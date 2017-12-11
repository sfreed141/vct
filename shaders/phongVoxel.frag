#version 450 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

// Diffuse color
uniform sampler2D texture0;

layout(binding = 1, rgba16f) uniform image3D voxelColor;

uniform float shine;

uniform vec3 eye;
uniform vec3 lightPos;
uniform vec3 lightInt;

ivec3 voxelIndex(vec3 pos) {
    const float minx = -20, maxx = 20,
        miny = -2, maxy = 15,
        minz = -12, maxz = 12;
    const int voxelDim = 64;

    float rangex = maxx - minx;
    float rangey = maxy - miny;
    float rangez = maxz - minz;

    float x = voxelDim * ((pos.x - minx) / rangex);
    float y = voxelDim * ((pos.y - miny) / rangey);
    float z = voxelDim * ((pos.z - minz) / rangez);

    return ivec3(x, y, z);
}

void main() {
    vec4 color = texture(texture0, fragTexcoord);

    vec3 norm = normalize(fragNormal);
    vec3 light = normalize(lightPos - fragPosition);
    vec3 view = normalize(eye - fragPosition);

    vec3 h = normalize(0.5 * (view + light));
    float vdotr = max(dot(norm, h), 0);

    float ambient = 0.2;
    float diffuse = max(dot(norm, light), 0);
    float specular = pow(vdotr, shine);

    color = vec4((ambient + diffuse + specular) * lightInt * color.rgb, 1);

    ivec3 i = voxelIndex(fragPosition);
    imageStore(voxelColor, i, color);
}
