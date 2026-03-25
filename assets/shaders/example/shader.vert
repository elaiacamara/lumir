#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} cameraData;

layout(std140,set = 0, binding = 1) readonly buffer storageBuffer{
    mat4 model[];
}ObjectData;

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec3 vertexColor;
layout(location = 2) in vec2 vertexTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    int instance_index = gl_InstanceIndex;
    vec2 pos = vertexPosition + vec2(0, instance_index * 1.0f); 
    gl_Position = cameraData.viewProjection * ObjectData.model[0] * vec4(pos, 0.0, 1.0);
    if (instance_index == 0)
        fragColor = vec4(1.0f, 1.0f,1.0f,0.1f);
    if (instance_index == 1)
        fragColor = vec4(0.0f, 1.0f,0.0f, 1.0f);
    if (instance_index == 2)
        fragColor = vec4(0.0f, 0.0f,1.0f, 1.0f);
    fragTexCoord = vertexTexCoord;
}
