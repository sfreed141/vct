#version 420

in vec2 fragTexcoord;

out vec4 color;

layout(binding = 0) uniform sampler3D voxelColor;
layout(binding = 1) uniform sampler3D voxelNormal;
layout(binding = 2) uniform sampler3D voxelRadiance;

uniform vec3 eye = vec3(0);
uniform vec3 viewForward = vec3(0, 0, -1);
uniform vec3 viewRight = vec3(1, 0, 0);
uniform vec3 viewUp = vec3(0, 1, 0);

uniform int width = 1280, height = 720;

uniform float near = 0.1, far = 100.0;

uniform int voxelDim = 128;

uniform int lod = 0;

uniform bool radiance = true;

ivec3 voxelIndex(vec3 pos) {
    const float minx = -20, maxx = 20,
        miny = -20, maxy = 20,
        minz = -20, maxz = 20;

    float rangex = maxx - minx;
    float rangey = maxy - miny;
    float rangez = maxz - minz;

    float x = voxelDim * ((pos.x - minx) / rangex);
    float y = voxelDim * ((pos.y - miny) / rangey);
    float z = voxelDim * (1 - (pos.z - minz) / rangez);

    return ivec3(x, y, z);
}

void main() {
    vec3 rayStart = eye;
    vec2 offset = (gl_FragCoord.xy / vec2(width, height)) * 2 - 1;
    vec3 rayDir = normalize(viewForward + offset.x * viewRight + offset.y * viewUp);

    vec4 value = vec4(0, 0, 0, 0);
    float scale = 1.0;
    while (value.a < 1 && scale < far) {
        vec3 voxelCoords = voxelIndex(rayStart + scale * rayDir) / float(voxelDim);
        vec4 sampleColor = textureLod(radiance ? voxelRadiance : voxelColor, voxelCoords, lod);
        float alpha = 1 - value.a;
        value.rgb += sampleColor.rgb * alpha;
        value.a += sampleColor.a * alpha;
        scale += 0.2;
    }

    color = value;
}
