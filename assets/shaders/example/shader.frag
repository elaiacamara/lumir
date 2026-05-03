#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D material;
layout(set = 1, binding = 1) uniform MaterialData {
    vec3 color;
} u_material;


void main(){
    outColor = texture(material, fragTexCoord);
    //outColor = vec4(u_material.color,  1.0f);
}
