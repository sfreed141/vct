#version 430 core

layout (location = 0) in vec3 vertPosition;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec2 vertTexcoord;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
} vs_out;

void main() {
    vs_out.position = vertPosition;
    vs_out.normal = vertNormal;
    vs_out.texcoord = vertTexcoord;

    gl_Position = vec4(vertPosition, 1);
}
