#version 430 core

layout (points) in;
// layout(points, max_vertices = 1) out;
layout (triangle_strip, max_vertices = 36) out;

in TE_OUT {
    vec3 worldPosition;
    vec3 normal;
    vec2 texcoord;
    vec4 voxelColor;
} gs_in[];

out GS_OUT {
    vec4 voxelColor;
} gs_out;

const vec3 cube_vertices[] = vec3[](
    vec3(-0.5,  0.5, -0.5),
    vec3( 0.5,  0.5, -0.5),
    vec3( 0.5,  0.5,  0.5),
    vec3(-0.5,  0.5,  0.5),

    vec3(-0.5, -0.5, -0.5),
    vec3( 0.5, -0.5, -0.5),
    vec3( 0.5, -0.5,  0.5),
    vec3(-0.5, -0.5,  0.5)
);

const int cube_indices[] = int[](
    5, 4, 1, 0, 0, 0,
    0, 3, 1, 2,
    5, 6, 4, 7,
    0, 3, 3, 3,
    2, 7, 6

);

uniform mat4 projection, view;
uniform float voxelDim = 256;
uniform vec3 voxelMin = vec3(-20), voxelMax = vec3(20), voxelCenter = vec3(0);

vec3 voxelWorldSize() {
    return (voxelMax - voxelMin) / voxelDim;
}

void main() {
    if (gs_in[0].voxelColor.a > 0) {
        gs_out.voxelColor = gs_in[0].voxelColor;

        // gl_Position = projection * view * vec4(gs_in[0].worldPosition, 1);
        // EmitVertex();
        // EndPrimitive();

        // project and emit vertices
        for (int i = 0; i < cube_indices.length(); i++) {
            vec3 position = voxelWorldSize() * cube_vertices[cube_indices[i]] + gs_in[0].worldPosition;
            gl_Position = projection * view * vec4(position, 1);
            EmitVertex();
        }

        EndPrimitive();
    }
}
