#version 330 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

// Diffuse color
uniform sampler2D texture0;

uniform vec3 eye;
uniform vec3 lightPos;
uniform vec3 lightInt;

out vec4 color;

void main() {
    color = texture(texture0, fragTexcoord);

    vec3 norm = normalize(fragNormal);
    vec3 light = normalize(lightPos - fragPosition);
    vec3 view = normalize(eye - fragPosition);

    vec3 h = normalize(0.5 * (view + light));
    float vdotr = max(dot(norm, h), 0);

    float shine = 30;
    float ambient = 0.2;
    float diffuse = max(dot(norm, light), 0);
    float specular = pow(vdotr, shine);

    color = vec4((ambient + diffuse + specular) * lightInt * color.rgb, 1);
}
