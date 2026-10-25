#version 450

layout(location=0) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

layout(set=1, binding=0) uniform SpriteParameters {
    vec4 color;
} params;

layout(set=1, binding=1) uniform sampler2D spriteTexture;

void main()
{
    outColor = texture(spriteTexture, inTexCoord) * params.color;
}
