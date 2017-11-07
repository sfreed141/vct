#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
} gs_in[];

out GS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
    flat int axis;
} gs_out;

// projection matrices along each axis
uniform mat4 mvp_x, mvp_y, mvp_z;

void main() {
    // find dominant axis (using face normal)
    vec3 faceNormal = normalize(cross(gs_in[1].position - gs_in[0].position, gs_in[2].position - gs_in[0].position));

    // since projecting onto std basis just find max component
    mat4 mvp;
    int axis;
    if (faceNormal.x > faceNormal.y && faceNormal.x > faceNormal.z) {
        mvp = mvp_x;
        axis = 0;
    }
    else if (faceNormal.y > faceNormal.x && faceNormal.y > faceNormal.z) {
        mvp = mvp_y;
        axis = 1;
    }
    else {
        mvp = mvp_z;
        axis = 2;
    }

    // project vertices and emit vertices
    for (int i = 0; i < 3; i++) {
        gl_Position = mvp * gl_in[i].gl_Position;

        gs_out.position = gl_Position.xyz;
        gs_out.normal = gs_in[i].normal;
        gs_out.texcoord = gs_in[i].texcoord;
        gs_out.axis = axis;
        EmitVertex();
    }

    EndPrimitive();
}
