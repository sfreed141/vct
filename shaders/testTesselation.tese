#version 430 core

layout(triangles, equal_spacing, ccw) in;

layout(binding = 0, rgba8) uniform image3D voxels;

in vec4 tcPosition[];

void main() {
    gl_Position = gl_TessCoord.x * tcPosition[0]//gl_in[0].gl_Position;
                + gl_TessCoord.y * tcPosition[1]//gl_in[1].gl_Position;
                + gl_TessCoord.z * tcPosition[2];//gl_in[2].gl_Position;
    imageStore(voxels, ivec3(0), vec4(1));
}