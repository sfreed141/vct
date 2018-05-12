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
    vec3 normal;
    vec2 texcoord;
    flat int axis;
} fs_in;

layout(binding = 0) uniform sampler2D diffuseTexture;

uniform bool voxelizeDilate = false;
uniform bool voxelWarp;
uniform bool voxelizeAtomicMax = false;

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

    if (voxelWarp) {
        unit = voxelWarpFn(unit);
    }

	return imageSize(voxelColor) * unit;
}

// From OpenGL Insights
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

void main() {
    vec3 color = texture(diffuseTexture, fs_in.texcoord).rgb;
	vec3 normal = (normalize(fs_in.normal) + 1) * 0.5; // map normal [-1, 1] -> [0, 1]

    // Store value (must be atomic, use alpha component as count)
#if USE_RGBA16F && GL_NV_shader_atomic_fp16_vector
	if (voxelizeDilate) {
		vec3 voxelPosition = getVoxelPosition();
		ivec3 voxelIndex = ivec3(voxelPosition);
		vec3 fractionalPosition = fract(voxelPosition);
		const ivec3 offsets[] = ivec3[](
			ivec3(0, 0, 0), ivec3(0, 0, 1), ivec3(0, 1, 0), ivec3(0, 1, 1),
			ivec3(1, 0, 0), ivec3(1, 0, 1), ivec3(1, 1, 0), ivec3(1, 1, 1)
		);

		for (int i = 0; i < 8; i++) {
			vec3 weight = vec3(
				offsets[i].x == 0 ? 1 -  fractionalPosition.x : fractionalPosition.x,
				offsets[i].y == 0 ? 1 -  fractionalPosition.y : fractionalPosition.y,
				offsets[i].z == 0 ? 1 -  fractionalPosition.z : fractionalPosition.z
			);
			imageAtomicAdd(voxelColor, voxelIndex + offsets[i], f16vec4(color, weight.x + weight.y + weight.z));
			imageAtomicAdd(voxelNormal, voxelIndex + offsets[i], f16vec4(normal, weight.x + weight.y + weight.z));
		}
	}
	else {
		ivec3 voxelIndex = ivec3(getVoxelPosition());
		imageAtomicAdd(voxelColor, voxelIndex, f16vec4(color, 1));
		imageAtomicAdd(voxelNormal, voxelIndex, f16vec4(normal, 1));
	}
#else
	ivec3 voxelIndex = ivec3(getVoxelPosition());
	if (voxelizeAtomicMax) {
		imageAtomicMax(voxelColor, voxelIndex, convVec4ToRGBA8(255 * vec4(color, 1)));
		imageAtomicMax(voxelNormal, voxelIndex, convVec4ToRGBA8(255 * vec4(normal, 1)));
	}
	else {
		imageAtomicRGBA8Avg(voxelColor, voxelIndex, vec4(color, 1));
		imageAtomicRGBA8Avg(voxelNormal, voxelIndex, vec4(normal, 1));
	}
#endif
}
