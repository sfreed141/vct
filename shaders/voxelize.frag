#version 430 core

#define USE_RGBA16F 0

#if USE_RGBA16F
#extension GL_NV_gpu_shader5: enable
#extension GL_NV_shader_atomic_float: enable
#extension GL_NV_shader_atomic_fp16_vector: enable

#if GL_NV_shader_atomic_fp16_vector
layout(binding = 0, rgba16f) uniform image3D voxelColor;
layout(binding = 1, rgba16f) uniform image3D voxelNormal;
#endif // GL_NV_shader_atomic_fp16_vector
#else
layout(binding = 0, r32ui) uniform uimage3D voxelColor;
layout(binding = 1, r32ui) uniform uimage3D voxelNormal;
#endif // USE_RGBA16F

in GS_OUT {
    vec3 position;
    vec3 worldPosition;
    vec3 normal;
    vec2 texcoord;
    flat int axis;
} fs_in;

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
struct Light {
                        // base		offset
    vec3 position;		// 16		0
    vec3 direction;		// 16		16
    vec3 color;			// 16		32

    float range;		// 4		44
    float intensity;	// 4		48

    bool enabled;		// 4		52
    bool selected;		// 4		56
    bool shadowCaster;	// 4		60
    uint type;			// 4		64
};

layout(std140, binding = 3) buffer LightBlock {
    Light lights[];
};

layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 6) uniform sampler2D shadowmap;

struct VoxelizeInfo {
    uint totalVoxelFragments, uniqueVoxels, maxFragmentsPerVoxel;
};

layout(std140, binding = 4) buffer VoxelizeInfoBlock {
    VoxelizeInfo voxelizeInfo;
};

uniform bool voxelizeDilate = false;
uniform bool warpVoxels;
uniform bool voxelizeAtomicMax = false;
uniform bool voxelizeLighting;
uniform vec3 eye;
uniform vec3 voxelCenter, voxelMin, voxelMax;
uniform mat4 ls;

// Map [-1, 1] -> [0, 1]
vec3 ndcToUnit(vec3 p) { return (p + 1.0) * 0.5; }

#pragma include "common.glsl"

vec3 getVoxelPosition() {
    // get ndc
    vec3 ndc = vec3(fs_in.position);

    // map to unit
    vec3 unit = ndcToUnit(ndc);

    // swizzle based on projected axis
    if (fs_in.axis == 0) {
        // looking down x axis
        unit = vec3(1 - unit.z, unit.y, unit.x);
    }
    else if (fs_in.axis == 1) {
        // looking down y axis
        unit = vec3(unit.x, 1 - unit.z, unit.y);
    }
    else {
        // looking down z axis
    }

    if (warpVoxels) {
        unit = voxelWarp(unit, voxelLinearPosition(eye, voxelCenter, voxelMin, voxelMax));
    }

    return imageSize(voxelColor) * unit;
}

#if 1
vec4 convRGBA8ToVec4(uint val) {
    return vec4(
        float(val & 0x000000FF),
        float((val & 0x0000FF00) >> 8U),
        float((val & 0x00FF0000) >> 16U),
        float((val & 0xFF000000) >> 24U)
    );
}

uint convVec4ToRGBA8(vec4 val) {
    return (uint(val.w) & 0x000000FF) << 24U
        | (uint(val.z) & 0x000000FF) << 16U
        | (uint(val.y) & 0x000000FF) << 8U
        | (uint(val.x) & 0x000000FF);
}

void imageAtomicRGBA8Avg(layout(r32ui) coherent volatile uimage3D imgUI, ivec3 coords, vec4 val) {
    val.rgb *= 255.0;
    uint newVal = convVec4ToRGBA8(val);
    uint prevStoredVal = 0, curStoredVal;
    while ((curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal)) != prevStoredVal) {
        prevStoredVal = curStoredVal;
        vec4 rval = convRGBA8ToVec4(curStoredVal);
        rval.xyz *= rval.w;
        vec4 curValF = rval + val;
        curValF.xyz /= curValF.w;
        newVal = convVec4ToRGBA8(curValF);
    }
}
#else
// From OpenGL Insights + https://rauwendaal.net/2013/02/07/glslrunningaverage/
void imageAtomicRGBA8Avg(layout(r32ui) coherent volatile uimage3D imgUI, ivec3 coords, vec4 val) {
    val.a = 1.0 / 255.0;
    uint newVal = packUnorm4x8(val);
    uint prevStoredVal = 0, curStoredVal;

    // Spin wait while other threads modify
    while ((curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal)) != prevStoredVal) {
        prevStoredVal = curStoredVal;
        // Extract the moving average (current average in rgb, normalized count in w)
        vec4 avg = unpackUnorm4x8(curStoredVal);
        avg.rgb = (avg.rgb * avg.w + val.rgb * val.w) / (avg.w + val.w);
        avg.w = avg.w + val.w;
        newVal = packUnorm4x8(avg);
    }
}
#endif

// Evaluates how shadowed a point is using PCF with 5 samples
float calcShadowFactor(vec4 lsPosition) {
    vec3 shifted = (lsPosition.xyz / lsPosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.01;
    float fragDepth = shifted.z - bias;

    if (fragDepth > 1.0) {
        return 0;
    }

    const int numSamples = 5;
    const ivec2 offsets[numSamples] = ivec2[](
        ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(-1, 0), ivec2(0, -1)
    );

    for (int i = 0; i < numSamples; i++) {
        if (fragDepth > textureOffset(shadowmap, shifted.xy, offsets[i]).r) {
            shadowFactor += 1;
        }
    }
    shadowFactor /= numSamples;

    return shadowFactor;
}

void main() {
    atomicAdd(voxelizeInfo.totalVoxelFragments, 1);

    vec3 color = texture(diffuseMap, fs_in.texcoord).rgb;
    vec3 normal = (normalize(fs_in.normal) + 1) * 0.5; // map normal [-1, 1] -> [0, 1]

    if (voxelizeLighting) {
        vec3 finalLighting = vec3(0);
        for (int i = 0; i < lights.length(); i++) {
            if (!lights[i].enabled) continue;

            vec3 lighting = vec3(0);
            switch (lights[i].type) {
                case POINT_LIGHT:
                    lighting = color * lights[i].intensity * lights[i].color * max(0, dot(normalize(fs_in.normal), normalize(lights[i].position-fs_in.worldPosition)));
                    lighting *= 1.0 - smoothstep(0.75 * lights[i].range, lights[i].range, distance(lights[i].position, fs_in.worldPosition));
                    break;
                case DIRECTIONAL_LIGHT:
                    lighting = color * lights[i].color * lights[i].intensity * max(0, dot(normalize(fs_in.normal), normalize(-lights[i].direction)));
                    break;
            }

            // TODO this only works for the 'mainlight' right now
            if (lights[i].shadowCaster) {
                float shadowFactor = 1.0 - calcShadowFactor(ls * vec4(fs_in.worldPosition, 1));
                lighting *= shadowFactor;
            }

            finalLighting += lighting;
        }
        color = clamp(finalLighting, 0, 1);
    }

    // Store value (must be atomic, use alpha component as count)
    vec3 voxelPosition = getVoxelPosition();
    ivec3 voxelIndex = ivec3(voxelPosition);
    if (voxelizeDilate) {
        vec3 fractionalPosition = fract(voxelPosition);

        vec3 t;
        vec3 d = modf(fractionalPosition + 0.5, t);
        d = mix(d, 1 - d, t);

        const ivec3 offsets[] = ivec3[](
            ivec3(0, 0, 0), ivec3(0, 0, 1), ivec3(0, 1, 0), ivec3(0, 1, 1),
            ivec3(1, 0, 0), ivec3(1, 0, 1), ivec3(1, 1, 0), ivec3(1, 1, 1)
        );
        for (int i = 0; i < offsets.length(); i++) {
            ivec3 offset = offsets[i];
            vec3 weights = mix(d, 1 - d, offset);
            offset = ivec3(mix(vec3(-1), vec3(1), t) * offset);

            // Weighted average separately in each direction would be better but we only have one component :/
            // TODO seems to diminish total brightness: is this expected (and needs to be compensated for)?
            float w = length(weights);
#if USE_RGBA16F && GL_NV_shader_atomic_fp16_vector
            imageAtomicAdd(voxelColor, voxelIndex + offset, f16vec4(color * w, w));
            imageAtomicAdd(voxelNormal, voxelIndex + offset, f16vec4(normal * w, w));
        }
    }
    else if (voxelizeAtomicMax) {
        imageAtomicMax(voxelColor, voxelIndex, f16vec4(color, 1));
        imageAtomicMax(voxelNormal, voxelIndex, f16vec4(normal, 1));
    }
    else {
        imageAtomicAdd(voxelColor, voxelIndex, f16vec4(color, 1));
        imageAtomicAdd(voxelNormal, voxelIndex, f16vec4(normal, 1));
    }
#else
    // TODO BAD THINGS HAPPEN (only on desktop linux though...)
    // also can't accurately emulate a weighted average with r32ui
    //         w = clamp(w, 0, 1);
    //         imageAtomicRGBA8Avg(voxelColor, voxelIndex + offset, vec4(color, w));
    //         imageAtomicRGBA8Avg(voxelNormal, voxelIndex + offset, vec4(normal, w));
        }
    }
    // else
    if (voxelizeAtomicMax) {
        imageAtomicMax(voxelColor, voxelIndex, packUnorm4x8(vec4(color, 1)));
        imageAtomicMax(voxelNormal, voxelIndex, packUnorm4x8(vec4(normal, 1)));
    }
    else {
        imageAtomicRGBA8Avg(voxelColor, voxelIndex, vec4(color, 1));
        imageAtomicRGBA8Avg(voxelNormal, voxelIndex, vec4(normal, 1));
    }
#endif
}
