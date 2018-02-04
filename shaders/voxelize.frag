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

layout(binding = 0, rgba16f) uniform image3D voxelColor;
layout(binding = 1, rgba16f) uniform image3D voxelNormal;

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

void main() {
	ivec3 voxelIndex = getVoxelPosition();

    vec3 diffuseColor = texture(diffuseTexture, fs_in.texcoord).rgb;

    // Store value (must be atomic, use alpha component as count)
#if GL_NV_shader_atomic_fp16_vector
    imageAtomicAdd(voxelColor, voxelIndex, f16vec4(diffuseColor, 1));
    imageAtomicAdd(voxelNormal, voxelIndex, f16vec4(fs_in.normal, 1));
#else
	// TODO
    imageStore(voxelColor, voxelIndex, f16vec4(diffuseColor, 1));
    imageStore(voxelNormal, voxelIndex, f16vec4(fs_in.normal, 1));
#endif
}
