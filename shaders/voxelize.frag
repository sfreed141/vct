#version 430 core

// #extension GL_NV_shader_atomic_fp16_vector: require

in GS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
    flat int axis;
} fs_in;

layout(location = 0, rgba16f) uniform image3D voxelColor;
layout(location = 1, rgba16f) uniform image3D voxelNormal;

uniform sampler2D diffuseTexture;

void main() {
    ivec3 voxelSize = imageSize(voxelColor);
    float voxelDim = voxelSize.x;

    // Get voxel index
    vec3 voxelIndex = vec3(gl_FragCoord.xy, gl_FragCoord.z * voxelDim);
    // Swap components based on which axis it was projected on
    if (fs_in.axis == 0) {
        // looking down x axis
        voxelIndex = vec3(voxelIndex.z, voxelIndex.y, voxelDim - voxelIndex.x);
    }
    else if (fs_in.axis == 1) {
        // looking down y axis
        voxelIndex = voxelIndex.yzx;
    }
    else {
        // looking down z axis (no change needed)
    }

    vec3 diffuseColor = texture(diffuseTexture, fs_in.texcoord).rgb;

    // TODO
    // Store value (must be atomic, use alpha component of normal as count)
//    imageAtomicAdd(voxelColor, voxelIndex, vec4(diffuseColor, 0));
//    imageAtomicAdd(voxelNormal, voxelIndex, vec4(fs_in.normal, 1));
}
