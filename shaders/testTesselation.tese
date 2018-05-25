#version 430 core

layout(triangles, equal_spacing, ccw, point_mode) in;

#pragma include "use_rgba16f.glsl"

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

layout(binding = 0) uniform sampler2D diffuseMap;

in vec4 tcPosition[];
in vec3 tcNormal[];
in vec2 tcTexcoord[];
in vec3 tcVoxelPosition[];

out TE_OUT {
    vec3 worldPosition;
    vec3 normal;
    vec2 texcoord;
    vec4 voxelColor;
} te_out;

uniform mat4 projection;
uniform mat4 view;

uniform bool voxelizeAtomicMax = false;
// TODO uniform bool voxelizeLighting;
uniform bool voxelizeTesselationWarp;

uniform float voxelDim = 256;
uniform vec3 voxelMin = vec3(-20), voxelMax = vec3(20), voxelCenter = vec3(0);

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

void voxelStore(ivec3 voxelIndex, vec4 color, vec3 normal) {
    normal = normalize(normal) * 0.5 + 0.5;

#if USE_RGBA16F
    if (voxelizeAtomicMax) {
        imageAtomicMax(voxelColor, voxelIndex, f16vec4(color.rgb, 1));
        imageAtomicMax(voxelNormal, voxelIndex, f16vec4(normal, 1));
    }
    else {
        imageAtomicAdd(voxelColor, voxelIndex, f16vec4(color.rgb, 1));
        imageAtomicAdd(voxelNormal, voxelIndex, f16vec4(normal, 1));
    }
#else
    // imageStore(voxelColor, voxelIndex, uvec4(packedColor, 0, 0, 0));
    // imageStore(voxelNormal, voxelIndex, uvec4(packedNormal, 0, 0, 0));
    if (voxelizeAtomicMax) {
        uint packedColor = packUnorm4x8(color);
        uint packedNormal = packUnorm4x8(vec4(normal, 1));

        imageAtomicMax(voxelColor, voxelIndex, packedColor);
        imageAtomicMax(voxelNormal, voxelIndex, packedNormal);
    }
    else {
        imageAtomicRGBA8Avg(voxelColor, voxelIndex, vec4(color.rgb, 1));
        imageAtomicRGBA8Avg(voxelNormal, voxelIndex, vec4(normal, 1));
    }
#endif
}

void main() {
    vec4 position = gl_TessCoord.x * tcPosition[0]
                    + gl_TessCoord.y * tcPosition[1]
                    + gl_TessCoord.z * tcPosition[2];
    vec3 normal = gl_TessCoord.x * tcNormal[0]
                    + gl_TessCoord.y * tcNormal[1]
                    + gl_TessCoord.z * tcNormal[2];
    vec2 texcoord = gl_TessCoord.x * tcTexcoord[0]
                    + gl_TessCoord.y * tcTexcoord[1]
                    + gl_TessCoord.z * tcTexcoord[2];

    vec4 color = texture(diffuseMap, texcoord);

    ivec3 voxelA = ivec3(voxelDim * tcVoxelPosition[0]), voxelB = ivec3(voxelDim * tcVoxelPosition[1]), voxelC = ivec3(voxelDim * tcVoxelPosition[2]);
    if (all(equal(voxelA, voxelB)) && all(equal(voxelA, voxelC)) && gl_TessCoord.x == 1.0) {
        voxelStore(voxelA, color, normal);
    }
    else {
        vec3 voxelPosition = (position.xyz - voxelCenter - voxelMin) / (voxelMax - voxelMin);
        ivec3 voxelIndex = ivec3(voxelPosition * imageSize(voxelColor).xyz);
        voxelStore(voxelIndex, color, normal);
    }

    te_out.worldPosition = position.xyz;
    te_out.normal = normal;
    te_out.texcoord = texcoord;
    te_out.voxelColor = color;

    // if (gl_TessLevelInner[0] > 32.0) te_out.voxelColor = vec4(1, 0, 0, 1);
    // if (gl_TessLevelInner[0] > 48.0) te_out.voxelColor = vec4(0, 1, 0, 1);
    // if (gl_TessLevelInner[0] >= 64.0) te_out.voxelColor = vec4(0, 0, 1, 1);
    // TODO fill adjacent voxels if tess level >= some threshold?


    // gl_Position = position;
    gl_Position = projection * view * position;
}