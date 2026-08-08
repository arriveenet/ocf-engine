#version 450
#define M_PI    (3.1415926535897932384626433832795)
#define M_INV_PI (1.0 / M_PI)

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec3 inTangent;
layout(location=3) in vec3 inBitangent;
layout(location=4) in vec2 inTexCoord;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform UBO {
    mat4 projection;
    mat4 view;
    mat4 model;
    vec4 lightDirection;
    vec3 eyePosition;
    float exposure;
} ubo;

layout(set=1, binding=0) uniform MaterialParameters {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float alphaCutoff;
    uint alphaMode;

    uint hasNormalMap;
    uint padding0;
    uint padding1;
    uint padding2;
} material;

layout(set=1, binding=1) uniform sampler2D baseColorTexture;

layout(set=1, binding=2) uniform sampler2D metallicRoughnessTexture;

layout(set=1, binding=3) uniform sampler2D normalMapTexture;

vec4 convertSRGBToLinear(vec4 srgbColor)
{
    vec3 linearColor = pow(srgbColor.rgb, vec3(2.2));
    return vec4(linearColor, srgbColor.w);
}

vec4 convertLinearToSRGB(vec4 linearColor)
{
    vec3 srgbColor = pow(linearColor.rgb, vec3(1.0 / 2.2));
    return vec4(srgbColor, linearColor.w);
}

vec4 getBaseColor()
{
    vec4 baseColor = material.baseColorFactor;
    vec4 fetchedColor = texture(baseColorTexture, inTexCoord);
    baseColor*= convertSRGBToLinear(fetchedColor);
    return baseColor;
}

vec3 getNormalVector()
{
    if (material.hasNormalMap > 0) {
        vec4 normalMap = texture(normalMapTexture, inTexCoord);
        mat3 TBN = mat3(normalize(inTangent), normalize(inBitangent), normalize(inNormal));
        normalMap = normalMap * 2.0 - 1.0;
        return normalize(TBN * normalMap.xyz);
    }
    return normalize(inNormal);
}

vec2 getMetallicRoughness()
{
    vec4 metallicRoughness = texture(metallicRoughnessTexture, inTexCoord);
    float metallic = metallicRoughness.b * material.metallicFactor;
    float roughness = metallicRoughness.g * material.roughnessFactor;
    return vec2(metallic, roughness);
}

vec3 specularReflection(vec3 reflectance0, float dotVH)
{
    float x = clamp(1.0 - dotVH, 0.0, 1.0);
    return reflectance0 + (vec3(1.0) - reflectance0) * pow(x, 5.0);
}

float microfactorDistribution(float alphaRoughness, float dotNH)
{
    float roughnessSquared = alphaRoughness * alphaRoughness;
    float factor = (dotNH * roughnessSquared - dotNH) * dotNH + 1.0;
    return roughnessSquared / (M_PI * factor * factor);
}

float geometricOcclusion(float alphaRoughness, float dotNL, float dotNV)
{
    float r = alphaRoughness;
    // Add a small value to prevent the denominator from becoming zero.
    float epsilon = 1e-5;
    float attenualtionL = 2.0 * dotNL / (dotNL + sqrt(r * r + (1.0 - r * r) * (dotNL * dotNL)) + epsilon);
    float attenualtionV = 2.0 * dotNV / (dotNV + sqrt(r * r + (1.0 - r * r) * (dotNV * dotNV)) + epsilon);
    return attenualtionL * attenualtionV;
}

vec3 pbrNeutralToneMapping(vec3 color)
{
    // Reference: https://github.com/KhronosGroup/ToneMapping

    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d -startCompression);
    color *= newPeak / peak;

    float g = 1.0 -1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, newPeak * vec3(1,1,1), g);
}

void main()
{
    vec3 toLightDir = normalize(ubo.lightDirection.xyz);
    vec4 baseColor = getBaseColor();

    vec2 metallicRoughness = getMetallicRoughness();
    float metallic = metallicRoughness.x;
    float roughness = metallicRoughness.y;

    vec3 normal = getNormalVector();
    vec3 viewDir = normalize(ubo.eyePosition - inPosition);
    vec3 halfVector = normalize(toLightDir + viewDir);

    float dotNL = clamp(dot(toLightDir, normal), 0.0, 1.0);
    float dotNV = clamp(dot(normal, viewDir), 0.0, 1.0);
    float dotNH = clamp(dot(normal, halfVector), 0.0, 1.0);
    float dotVH = clamp(dot(viewDir, halfVector), 0.0, 1.0);
    const vec3 F0 = vec3(0.04);
    const vec3 lightColor = vec3(1.0);
    const float alphaRoughness = roughness * roughness;

    vec3 diffuse = baseColor.rgb * (1.0 - metallic);
    vec3 specular = mix(F0, baseColor.rgb, metallic);

    float D = microfactorDistribution(alphaRoughness, dotNH);
    vec3 F = specularReflection(specular, dotVH);
    float G = geometricOcclusion(alphaRoughness, dotNL, dotNV);

    vec3 diffuseContribution = (1.0 - F) * diffuse * M_PI;
    vec3 specularContribution = (D * F * G) / max(4.0 * dotNL * dotNV, 0.0001);

    vec3 color = dotNL * lightColor * (diffuseContribution + specularContribution);
    outColor = vec4(color, baseColor.a);

    // Alpha masking
    if (material.alphaMode == 1 && outColor.a < material.alphaCutoff) {
        discard;
    }

    // Tone mapping and exposure adjustment
    outColor.xyz *= ubo.exposure;
    outColor.xyz = pbrNeutralToneMapping(outColor.xyz);

    // Convert linear color to sRGB for output
    outColor = convertLinearToSRGB(outColor);
}
