#version 450

layout(location=0) in vec3 inPosition;
layout(location=3) in vec2 inTexture;

layout(location=0) out vec2 outTexture;

layout(set=0, binding=0)
uniform SceneConstants
{
    mat4 matWorld;
    mat4 matView;
    mat4 matProj;
};

void main()
{
    vec4 worldPosition = matWorld * vec4(inPosition, 1.0);
    gl_Position = matProj * matView * worldPosition;
    outTexture = inTexture;
}