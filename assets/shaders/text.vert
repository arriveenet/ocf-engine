#version 450

layout(location=0) in vec3 inPosition;
layout(location=4) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;

layout(set=0, binding=0) uniform UBO {
    mat4 projection;
    mat4 view;
    mat4 model;
    vec4 lightDirection;
    vec3 eyePosition;
    float exposure;
} ubo;

void main()
{
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
}
