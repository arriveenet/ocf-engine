#version 450

layout(location=0) in vec3 inPosition;
layout(location=4) in vec2 inTexCoord;

layout(location=0) out vec2 outTexCoord;

layout(set=0, binding=0) uniform UBO {
    mat4 projection;
    mat4 view;
    vec4 lightDirection;
    vec3 eyePosition;
    float exposure;
} ubo;

layout(push_constant) uniform Object {
    mat4 model;
} object;

void main()
{
    gl_Position = ubo.projection * ubo.view * object.model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
}
