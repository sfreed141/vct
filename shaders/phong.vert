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
uniform mat4 ls;

out VS_OUT {
    vec3 fragPosition;
    vec3 fragNormal;
    vec2 fragTexcoord;

    vec4 lightFragPos;

#ifdef NORMAL_MAP
    vec3 tangentLightPos;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
#endif
} vs_out;

void main() {
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vs_out.fragPosition = vec3(model * vec4(position, 1));
    vs_out.fragNormal = normalMatrix * normal;
    vs_out.fragTexcoord = texcoord;
    gl_Position = projection * view * model * vec4(position, 1);

    vs_out.lightFragPos = ls * model * vec4(position, 1);

#ifdef NORMAL_MAP
    // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    vec3 T = normalMatrix * tangent;
    vec3 B = normalMatrix * bitangent;
    vec3 N = normalMatrix * normal;
    // re-orthogonalize
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));
    vs_out.tangentLightPos = TBN * lightPos;
    vs_out.tangentViewPos = TBN * eye;
    vs_out.tangentFragPos = TBN * vec3(model * vec4(position, 0));
#endif
}
