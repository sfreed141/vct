#version 330 core

#define NORMAL_MAP

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
#ifdef NORMAL_MAP
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform vec3 lightPos;
uniform vec3 eye;
#endif

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out VS_OUT {
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexcoord;

#ifdef NORMAL_MAP
    vec3 tangentLightPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
#endif
} vs_out;

void main() {
    vs_out.fragPosition = vec3(model * vec4(position, 1.0f));
    vs_out.fragNormal = vec3(model * vec4(normal, 0.0f));
    vs_out.fragTexcoord = vec2(texcoord.x, 1.0f - texcoord.y);
    gl_Position = projection * view * model * vec4(position, 1.0f);

#ifdef NORMAL_MAP
    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 T = normalize(vec3(model * vec4(tangent, 0)));
    vec3 B = normalize(vec3(model * vec4(bitangent, 0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0)));
    mat3 TBN = transpose(mat3(T, B, N));
    vs_out.tangentLightPos = TBN * lightPos;
    vs_out.tangentViewPos = TBN * eye;
    vs_out.tangentFragPos = TBN * vec3(model * vec4(position, 0));
#endif
}
