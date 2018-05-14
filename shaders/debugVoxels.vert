#version 420 core

layout(location = 0) in vec3 position;

out VS_OUT {
    vec3 worldPosition;
    vec4 voxelColor;
} vs_out;

layout(binding = 0) uniform sampler3D voxels;

uniform mat4 mvp;

uniform int level = 0;
uniform float voxelDim;
uniform vec3 voxelMin, voxelMax, voxelCenter;

void main() {
    float instance = float(gl_InstanceID);
    int x = int(mod(instance, voxelDim));
    int y = int(mod(instance / voxelDim, voxelDim));
    int z = int(instance / (voxelDim * voxelDim));
    vec3 voxelPosition = vec3(x, y, z) / voxelDim;

    vs_out.voxelColor = textureLod(voxels, voxelPosition, level);
    vs_out.worldPosition = position + voxelCenter + mix(voxelMin, voxelMax, voxelPosition);

    gl_Position = vec4(vs_out.worldPosition, 1);
}
