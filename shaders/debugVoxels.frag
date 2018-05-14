#version 420 core

in GS_OUT {
    vec4 voxelColor;
} fs_in;

out vec4 color;

void main() {
    color = fs_in.voxelColor;
}
