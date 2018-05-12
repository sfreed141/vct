#version 450 core

#define NORMAL_MAP

in VS_OUT {
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexcoord;

    vec4 lightFragPos;

#ifdef NORMAL_MAP
    mat3 TBN;
    mat3 inverseTBN;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
#endif
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

struct Material {
                        // base		offset
    vec3 ambient;		// 16		0
    vec3 diffuse;		// 16		16
    vec3 specular;		// 16		32
    float shininess;	// 4		44

    bool hasAmbientMap;	// 4		48
    bool hasDiffuseMap;	// 4		52
    bool hasSpecularMap;// 4		56
    bool hasAlphaMap;	// 4		60
    bool hasNormalMap;	// 4		64
};

layout(std140) uniform MaterialBlock {
    Material material;
};

// uniform sampler2D ambientMap;
// uniform sampler2D alphaMap;

layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D specularMap;

#ifdef NORMAL_MAP
layout(binding = 5) uniform sampler2D normalMap;
#endif

layout(binding = 6) uniform sampler2D shadowmap;

layout(binding = 2) uniform sampler3D voxelColor;
layout(binding = 3) uniform sampler3D voxelNormal;
layout(binding = 4) uniform sampler3D voxelRadiance;

uniform bool voxelize = false;
uniform bool normals = false;
uniform bool dominant_axis = false;
uniform bool radiance = false;
uniform bool drawWarpSlope = false;

uniform bool enablePostprocess = false;
uniform bool enableShadows = true;
uniform bool enableNormalMap = true;
uniform bool enableIndirect = false;
uniform bool enableDiffuse = true;
uniform bool enableSpecular = true;
uniform bool enableReflections = false;
uniform float ambientScale = 0.2;
uniform float reflectScale = 0.5;

uniform vec3 eye;
uniform mat4 ls;

uniform int miplevel = 0;
uniform int voxelDim;
uniform vec3 voxelMin, voxelMax;
uniform vec3 voxelCenter;
uniform bool voxelWarp;

uniform int vctSteps;
uniform float vctConeAngle;
uniform float vctBias;
uniform float vctConeInitialHeight;
uniform float vctLodOffset;
uniform int vctSpecularSteps;
uniform float vctSpecularConeAngle;
uniform float vctSpecularBias;
uniform float vctSpecularConeInitialHeight;
uniform float vctSpecularLodOffset;

out vec4 color;

#pragma include "common.glsl"

// based on https://github.com/godotengine/godot/blob/master/drivers/gles3/shaders/scene.glsl
vec3 traceCone(sampler3D voxelTexture, vec3 position, vec3 direction, int steps, float bias, float coneAngle, float coneHeight, float lodOffset) {
    direction = normalize(direction);
    direction.z = -direction.z;
    direction /= voxelDim;
    vec3 start = position + bias * direction;

    vec3 color = vec3(0);
    float alpha = 0;

    // TODO incorporate voxel size (need to support warping too). one axis for now (assume extents symmetric)
    // float stepScale = linearVoxelSize(voxelDim, voxelMin, voxelMax).x;
    // direction *= stepScale;
    for (int i = 0; i < steps && alpha < 0.95; i++) {
        float coneRadius = coneHeight * tan(coneAngle / 2.0);
        float lod = log2(max(1.0, 2 * coneRadius));
        vec3 samplePosition = start + coneHeight * direction;
        vec4 sampleColor = textureLod(voxelTexture, samplePosition, lod + lodOffset);
        float a = 1 - alpha;
        color += sampleColor.rgb * a;
        alpha += a * sampleColor.a;
        // color += sampleColor.rgb * sampleColor.a;
        coneHeight += coneRadius;
    }

    return color;
}

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

vec3 postprocess(vec3 color) {
    const float gamma = 2.2;
    // tone map
    color = color / (color + vec3(1));
    // gamma correction
    color = pow(color, vec3(1.0 / gamma));

    return color;
}

// lighting model adapted from https://www.3dgep.com/forward-plus/
float calculateDiffuse(vec3 N, vec3 L) {
    return max(dot(N, L), 0);
}

float calculateSpecular(vec3 N, vec3 L, vec3 V, float shininess) {
    vec3 H = normalize(V + L);
    return pow(max(dot(N, H), 0), shininess);
}

float calculateAttenuation(float range, float d) {
    return 1.0 - smoothstep(0.75 * range, range, d);
}

struct LightingResult {
    vec3 diffuse, specular;
};

LightingResult calculatePointLight(Light light, vec3 P, vec3 N, vec3 V, float shininess) {
    float dist = distance(light.position, P);
    if (dist > light.range) return LightingResult(vec3(0), vec3(0));

    float attenuation = calculateAttenuation(light.range, dist);
    vec3 L = normalize(light.position - P);
    vec3 diffuse = calculateDiffuse(N, L) * attenuation * light.color * light.intensity;
    vec3 specular = calculateSpecular(N, L, V, shininess) * attenuation * light.color * light.intensity;

    return LightingResult(diffuse, specular);
}

LightingResult calculateDirectionalLight(Light light, vec3 N, vec3 V, float shininess) {
    vec3 L = normalize(-light.direction);
    vec3 diffuse = calculateDiffuse(N, L) * light.color * light.intensity;
    vec3 specular = calculateSpecular(N, L, V, shininess) * light.color * light.intensity;

    return LightingResult(diffuse, specular);
}

// Lighting calculations are done in world space; N assumed normalized
LightingResult calculateDirectLighting(Material m, vec3 eye, vec3 P, vec3 N) {
    vec3 V = normalize(eye - P);

    LightingResult finalLighting = LightingResult(vec3(0), vec3(0));
    for (int i = 0; i < lights.length(); i++) {
        if (!lights[i].enabled) continue;

        LightingResult lighting = LightingResult(vec3(0), vec3(0));
        switch (lights[i].type) {
            case POINT_LIGHT:
                lighting = calculatePointLight(lights[i], P, N, V, m.shininess);
                break;
            case DIRECTIONAL_LIGHT:
                lighting = calculateDirectionalLight(lights[i], N, V, m.shininess);
                break;
        }

        // TODO this only works for the 'mainlight' right now
        if (lights[i].shadowCaster) {
            float shadowFactor = 1.0 - calcShadowFactor(fs_in.lightFragPos);
            lighting.diffuse *= shadowFactor;
            lighting.specular *= shadowFactor;
        }

        finalLighting.diffuse += lighting.diffuse;
        finalLighting.specular += lighting.specular;
    }

    return finalLighting;
}

void main() {
    if (voxelize) {
        vec3 i = voxelIndex(fs_in.fragPosition, voxelDim, voxelCenter, voxelMin, voxelMax, voxelWarp) / voxelDim;

        if (normals) {
            vec3 normal = normalize(textureLod(voxelNormal, i, miplevel).rgb);
            color = vec4(normal, 1);
        }
        else if (drawWarpSlope) {
            vec3 range = voxelMax - voxelMin;
            vec3 pos = fs_in.fragPosition - voxelCenter;

            vec3 unit = (pos - voxelMin) / range;
            unit.z = 1 - unit.z;

            float slope = voxelWarpFnGradient(unit).x;
            unit = voxelWarpFn(unit);

            const vec3 gradient[] = vec3[](
                vec3(1, 0, 0),
                vec3(0, 1, 0),
                vec3(0, 0, 1)
            );
            color.rgb = mix(gradient[int(max(floor(slope), 0))], gradient[int(min(ceil(slope), gradient.length()))], fract(slope));
        }
        else if (radiance) {
            color = textureLod(voxelRadiance, i, miplevel).rgba;
        }
        else {
            color = textureLod(voxelColor, i, miplevel).rgba;
        }


        return;
    }

    vec4 diffuseColor = material.hasDiffuseMap ? texture(diffuseMap, fs_in.fragTexcoord) : vec4(material.diffuse, 1);
    vec4 specularColor;
    if (material.hasSpecularMap) {
        specularColor = texture(specularMap, fs_in.fragTexcoord);
    }
    else if (material.hasDiffuseMap) {
        specularColor = diffuseColor;
    }
    else {
        specularColor = vec4(material.specular, 1);
    }

    vec3 lightInt = lights[0].color;

    vec3 normal;
#ifdef NORMAL_MAP
    if (enableNormalMap && material.hasNormalMap) {

        normal = texture(normalMap, fs_in.fragTexcoord).rgb;
        normal = normalize(normal * 2.0 - 1.0);
        normal = normalize(fs_in.TBN * normal);
    }
    else
#endif
    {
        normal = normalize(fs_in.fragNormal);
    }

    if (normals) {
        color = vec4(normal, 1);
    }
    else if (dominant_axis) {
        normal = abs(normal);
        color = vec4(step(vec3(max(max(normal.x, normal.y), normal.z)), normal.xyz), 1);
    }
    else {
        LightingResult lighting = calculateDirectLighting(material, eye, fs_in.fragPosition, normal);

        vec3 diffuseLighting = diffuseColor.rgb * (enableDiffuse ? lighting.diffuse : vec3(0));
        vec3 specularLighting = specularColor.rgb * (enableSpecular ? lighting.specular : vec3(0));

        if (enableIndirect) {
            vec3 voxelPosition = voxelIndex(fs_in.fragPosition, voxelDim, voxelCenter, voxelMin, voxelMax, voxelWarp) / voxelDim;
            vec3 indirect = vec3(0);
            indirect += traceCone(radiance ? voxelRadiance : voxelColor, voxelPosition, normal, vctSteps, vctBias, vctConeAngle, vctConeInitialHeight, vctLodOffset);

            vec3 coneDirs[4] = vec3[] (
                vec3(0.707, 0.707, 0),
                vec3(0, 0.707, 0.707),
                vec3(-0.707, 0.707, 0),
                vec3(0, 0.707, -0.707)
            );
            float coneWeights[4] = float[](0.25, 0.25, 0.25, 0.25);
            for (int i = 0; i < 4; i++) {
                vec3 dir = normalize(fs_in.TBN * coneDirs[i]);
                indirect += coneWeights[i] * traceCone(radiance ? voxelRadiance : voxelColor, voxelPosition, dir, vctSteps, vctBias, vctConeAngle, vctConeInitialHeight, vctLodOffset);
            }

            indirect *= ambientScale;
            indirect *= diffuseColor.rgb;
            color = vec4(indirect + diffuseLighting + specularLighting, 1);

            if (enableReflections) {
                vec3 reflectColor = traceCone(
                    radiance ? voxelRadiance : voxelColor,
                    voxelPosition,
                    reflect(fs_in.fragPosition - eye, normalize(fs_in.fragNormal)),
                    vctSpecularSteps, vctSpecularBias, vctSpecularConeAngle, vctSpecularConeInitialHeight, vctSpecularLodOffset
                );
                // indirect += 0.5 * reflectColor;
                color.rgb = mix(color.rgb, reflectColor, reflectScale);
            }
        }
        else {
            color.rgb = ambientScale * diffuseColor.rgb + diffuseLighting + specularLighting;
            color.a = 1;
        }

        if (enablePostprocess) {
            color.rgb = postprocess(color.rgb);
        }
    }
}
