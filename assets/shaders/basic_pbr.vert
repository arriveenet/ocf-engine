#version 450

layout(location=0) in vec3 inPosition;
layout(location=1) in vec3 inNormal;
layout(location=2) in vec4 inTangent;
layout(location=4) in vec2 inTexCoord;

layout(location=0) out vec3 outWorldPosition;
layout(location=1) out vec3 outNormal;
layout(location=2) out vec3 outTangent;
layout(location=3) out vec3 outBitangent;
layout(location=4) out vec2 outTexCoord;

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
	vec4 worldPosition = ubo.model * vec4(inPosition, 1.0);
	gl_Position = ubo.projection * ubo.view * worldPosition;
	outWorldPosition = worldPosition.xyz;
	outTexCoord = inTexCoord;

    mat3 mat3World = mat3(ubo.model);
    vec3 N = normalize(mat3World * inNormal);
    vec3 T = normalize(mat3World * inTangent.xyz);
    vec3 B = cross(N, T) * inTangent.w;

    outNormal = N;
    outTangent = T;
    outBitangent = B;
}
