#version 450 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

// Diffuse color
uniform sampler2D texture0;

layout(binding = 1, rgba16f) uniform image3D voxelColor;

uniform bool voxelize = false;
uniform bool normals = false;
uniform bool dominant_axis = false;

uniform float shine;

uniform vec3 eye;
uniform vec3 lightPos;
uniform vec3 lightInt;

out vec4 color;

ivec3 voxelIndex(vec3 pos) {
    const float minx = -20, maxx = 20,
        miny = -20, maxy = 20,
        minz = -20, maxz = 20;
    const int voxelDim = 64;

    float rangex = maxx - minx;
    float rangey = maxy - miny;
    float rangez = maxz - minz;

    float x = voxelDim * ((pos.x - minx) / rangex);
    float y = voxelDim * ((pos.y - miny) / rangey);
    float z = voxelDim * (1 - (pos.z - minz) / rangez);

    return ivec3(x, y, z);
}

void main() {
    color = texture(texture0, fragTexcoord);

    vec3 norm = normalize(fragNormal);
    vec3 light = normalize(lightPos - fragPosition);
    vec3 view = normalize(eye - fragPosition);

    vec3 h = normalize(0.5 * (view + light));
    float vdotr = max(dot(norm, h), 0);

    float ambient = 0.2;
    float diffuse = max(dot(norm, light), 0);
    float specular = pow(vdotr, shine);

    if (voxelize) {
        ivec3 i = voxelIndex(fragPosition);
        color = imageLoad(voxelColor, i).rgba;
		color.rgb = color.rgb / color.a;
		color.a = 1;
    }
    else {
        if (normals) {
            color = vec4(norm, 1);
        }
        else if (dominant_axis) {
            norm = abs(norm);
            color = vec4(step(vec3(max(max(norm.x, norm.y), norm.z)), norm.xyz), 1);
        }
        else {
            color = vec4((ambient + diffuse + specular) * lightInt * color.rgb, 1);
        }
    }
}
