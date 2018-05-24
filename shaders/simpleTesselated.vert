#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

// uniform mat4 projection;
// uniform mat4 view;
uniform mat4 model;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexcoord;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    fragPosition = vec3(model * vec4(position, 1));
    fragNormal = normalMatrix * normal;
    fragTexcoord = texcoord;
    gl_Position = model * vec4(position, 1);
}
