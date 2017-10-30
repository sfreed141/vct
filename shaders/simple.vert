#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexcoord;

void main() {
    fragPosition = vec3(model * vec4(position, 1.0f));
    fragNormal = vec3(model * vec4(normal, 0.0f));
    fragTexcoord = vec2(texcoord.x, 1.0f - texcoord.y);
    gl_Position = projection * view * model * vec4(position, 1.0f);
}
