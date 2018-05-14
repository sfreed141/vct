#version 420 core

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

layout(binding = 9) uniform sampler2D alphaMap;

void main() {
    if (material.hasAlphaMap) {
        float alpha = texture(alphaMap, fragTexcoord).r;
        if (alpha < 0.1) {
            discard;
        }
    }
}
