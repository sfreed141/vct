#version 430 core

layout (location = 0) in vec3 vertPosition;
layout (location = 1) in vec3 vertNormal;
layout (location = 2) in vec2 vertTexcoord;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
} vs_out;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(vertPosition, 1.0);

    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vs_out.position = vec3(gl_Position);
    vs_out.normal = normalMatrix * vertNormal;
    vs_out.texcoord = vertTexcoord;
}
