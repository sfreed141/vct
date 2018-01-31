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

void main() {
    ivec3 voxelSize = imageSize(voxelColor);
    float voxelDim = voxelSize.x;

    // Get voxel index
    vec3 voxelIndex = vec3(gl_FragCoord.xy, gl_FragCoord.z * voxelDim);

    // Swap components based on which axis it was projected on
    if (fs_in.axis == 0) {
        // looking down x axis
		voxelIndex = vec3(voxelDim - voxelIndex.z, voxelIndex.y, voxelIndex.x);
    }
    else if (fs_in.axis == 1) {
        // looking down y axis
		voxelIndex = vec3(voxelIndex.x, voxelIndex.z, voxelDim - voxelIndex.y);
    }
    else {
        // looking down z axis
		voxelIndex.z = voxelDim - voxelIndex.z;
    }

    vec3 diffuseColor = texture(diffuseTexture, fs_in.texcoord).rgb;

    // Store value (must be atomic, use alpha component as count)
#if GL_NV_shader_atomic_fp16_vector
    imageAtomicAdd(voxelColor, ivec3(voxelIndex), f16vec4(diffuseColor, 1));
    imageAtomicAdd(voxelNormal, ivec3(voxelIndex), f16vec4(fs_in.normal, 1));
#else
	// TODO
    imageStore(voxelColor, ivec3(voxelIndex), f16vec4(diffuseColor, 1));
    imageStore(voxelNormal, ivec3(voxelIndex), f16vec4(fs_in.normal, 1));
#endif
}
