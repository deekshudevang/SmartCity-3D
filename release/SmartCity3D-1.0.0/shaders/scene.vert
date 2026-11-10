#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;
uniform mat3 u_NormalMatrix;
uniform int u_HasNormalMatrix;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec4 v_Color;
out vec4 v_FragPosLightSpace;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPos.xyz;

    vec3 normal;
    if (u_HasNormalMatrix == 1) {
        normal = normalize(u_NormalMatrix * a_Normal);
    } else {
        normal = normalize(mat3(transpose(inverse(u_Model))) * a_Normal);
    }
    v_Normal = normal;
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;

    // Normal-offset bias in light space to prevent shadow acne and self-shadowing
    vec4 biasedPos = worldPos + vec4(normal * 0.04, 0.0);
    v_FragPosLightSpace = u_LightSpaceMatrix * biasedPos;

    gl_Position = u_ViewProjection * worldPos;
}
