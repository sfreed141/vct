#version 430 core

layout(location = 0) out vec3 color;
layout(location = 1) out vec3 normal;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

struct Material {
                        // base		offset
    vec3 ambient;		// 16		0
    vec3 diffuse;		// 16		16
    vec3 specular;		// 16		32
    float shininess;	// 4		44

    bool hasAmbientMap;	// 4		48
    bool hasDiffuseMap;	// 4		52
    bool hasSpecularMap;// 4		56
    bool hasAlphaMap;	// 4		60
    bool hasNormalMap;	// 4		64
    bool hasRoughnessMap;//4        68
    bool hasMetallicMap; //4        72
};
uniform Material material;

// TODO Mesh::draw only binds for phong (and it queries binding location)
// layout(std140) uniform MaterialBlock {
//     Material material;
// };
layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 9) uniform sampler2D alphaMap;

void main() {
    if (material.hasAlphaMap && texture(alphaMap, fragTexcoord).r < 0.1) { discard; }

    color = material.hasDiffuseMap ? texture(diffuseMap, fragTexcoord).rgb : material.diffuse;
    normal = normalize(fragNormal) * 0.5 + 0.5;
}