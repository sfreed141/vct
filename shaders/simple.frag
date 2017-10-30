#version 330 core

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexcoord;

uniform sampler2D texture0;

out vec4 color;

void main() {
    //color = vec4(vec3(.1, .1, fragPosition.y), 1.0f);
//    color = vec4(vec3(fragPosition.y), 1.0f);
//    color = vec4(fragNormal, 1.0f);

    color = texture(texture0, fragTexcoord);
    //color = vec4 (tex, 1.0f);
}
