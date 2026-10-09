#version 450

layout(location=0) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

layout(set=1, binding=0) uniform TextParameters {
    vec4 color;
} params;

layout(set=1, binding=1) uniform sampler2D fontTexture;

void main()
{
    // Glyph coverage is r * a: R8 atlases read a = 1, grayscale images loaded as RGBA have
    // r = coverage and a = 1, and white glyphs stored in alpha have r = 1
    vec4 texel = texture(fontTexture, inTexCoord);
    float coverage = texel.r * texel.a;

    outColor = vec4(params.color.rgb, params.color.a * coverage);
}
