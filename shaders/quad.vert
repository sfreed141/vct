#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tc;

out VS_OUT {
    vec2 tc;
} vs_out;

void main() {
    vs_out.tc = tc;
    gl_Position = vec4(position * 0.8, 1);
}
