#version 420 core

layout(location = 0) in vec3 position;

out VS_OUT {
    vec3 worldPosition;
    vec4 voxelColor;
} vs_out;

layout(binding = 0) uniform sampler3D voxels;

uniform mat4 mvp;

uniform float level = 0.0;
uniform float voxelDim;
uniform vec3 voxelMin, voxelMax, voxelCenter;

void main() {
    float instance = float(gl_InstanceID);
    float x = modf(instance / voxelDim, instance);
    float y = modf(instance / voxelDim, instance);
    float z = instance / voxelDim;
    vec3 voxelPosition = vec3(x,y,z) + 0.5 / voxelDim;

    vec3 tc = vec3(voxelPosition.x, voxelPosition.y, 1 - voxelPosition.z);
    vs_out.voxelColor = textureLod(voxels, tc, level);

    // TODO doesn't account for warping (will need to scale cubes differently too)
    vs_out.worldPosition = position + voxelCenter + mix(voxelMin, voxelMax, voxelPosition);

    gl_Position = vec4(vs_out.worldPosition, 1);
}
