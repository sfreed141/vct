#version 430 core

#extension GL_NV_gpu_shader5: enable
#extension GL_NV_shader_atomic_float: enable
#extension GL_NV_shader_atomic_fp16_vector: enable

in GS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
    flat int axis;
} fs_in;

#if GL_NV_shader_atomic_fp16_vector
layout(binding = 0, rgba16f) uniform image3D voxelColor;
layout(binding = 1, rgba16f) uniform image3D voxelNormal;
#else
layout(binding = 0, r32ui) uniform uimage3D voxelColor;
layout(binding = 1, r32ui) uniform uimage3D voxelNormal;
#endif

uniform sampler2D diffuseTexture;

// Map [-1, 1] -> [0, 1]
vec3 ndcToUnit(vec3 p) { return (p + 1.0) * 0.5; }

ivec3 getVoxelPosition() {
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

	// map to voxel index
	vec3 voxelIndex = unit * imageSize(voxelColor).x;

	return ivec3(voxelIndex);
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

// void imageAtomicRGBA8Avg(layout(r32ui) coherent volatile uimage3D imgUI, ivec3 coords, vec4 val) {
// 	val.rgb *= 255.0;
// 	uint newVal = convVec4ToRGBA8(val);
// 	uint prevStoredVal = 0, curStoredVal;
// 	while ((curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal)) != prevStoredVal) {
// 		prevStoredVal = curStoredVal;
// 		vec4 rval = convRGBA8ToVec4(curStoredVal);
// 		rval.xyz *= rval.w;
// 		vec4 curValF = rval + val;
// 		curValF.xyz /= curValF.w;
// 		newVal = convVec4ToRGBA8(curValF);
// 	}
// }

void main() {
	ivec3 voxelIndex = getVoxelPosition();

    vec3 diffuseColor = texture(diffuseTexture, fs_in.texcoord).rgb;

    // Store value (must be atomic, use alpha component as count)
#if GL_NV_shader_atomic_fp16_vector
    imageAtomicAdd(voxelColor, voxelIndex, f16vec4(diffuseColor, 1));
    imageAtomicAdd(voxelNormal, voxelIndex, f16vec4(fs_in.normal, 1));
#else
	// imageAtomicRGBA8Avg(voxelColor, voxelIndex, vec4(diffuseColor, 1));
	// imageAtomicRGBA8Avg(voxelNormal, voxelIndex, vec4(fs_in.normal, 1));
	imageAtomicMax(voxelColor, voxelIndex, convVec4ToRGBA8(255 * vec4(diffuseColor, 1)));
	imageAtomicMax(voxelNormal, voxelIndex, convVec4ToRGBA8(255 * vec4(fs_in.normal, 1)));
#endif
}
