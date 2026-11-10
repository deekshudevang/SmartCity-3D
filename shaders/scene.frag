#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec4 v_Color;
in vec4 v_FragPosLightSpace;

out vec4 o_Color;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    vec3 ambient;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

#define MAX_POINT_LIGHTS 16

uniform DirectionalLight u_DirLight;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];
uniform int u_NumPointLights;

uniform vec3 u_CameraPos;
uniform sampler2D u_ShadowMap;
uniform int u_HasTexture;
uniform sampler2D u_Texture;
uniform float u_SpecularStrength;
uniform float u_Shininess;
uniform vec4 u_ColorOverride;
uniform int u_UseColorOverride;

float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Keep shadows clear outside the far plane of the light frustum
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // Adaptive bias based on light angle to eliminate shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // 3x3 PCF (Percentage-Closer Filtering) for soft shadow edges
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    vec4 baseColor = (u_UseColorOverride == 1) ? u_ColorOverride : v_Color;
    if (u_HasTexture == 1) {
        baseColor *= texture(u_Texture, v_TexCoord);
    }

    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);

    // 1. Directional Light (Sun / Moon)
    vec3 lightDir = normalize(-u_DirLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), u_Shininess);

    float shadow = CalculateShadow(v_FragPosLightSpace, normal, lightDir);

    vec3 ambient = u_DirLight.ambient * baseColor.rgb;
    vec3 diffuse = u_DirLight.color * diff * u_DirLight.intensity * baseColor.rgb;
    vec3 specular = u_DirLight.color * spec * u_SpecularStrength * u_DirLight.intensity;

    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);

    // 2. Point Lights (Streetlights, Headlights)
    for (int i = 0; i < u_NumPointLights && i < MAX_POINT_LIGHTS; ++i) {
        vec3 pLightDir = u_PointLights[i].position - v_FragPos;
        float distance = length(pLightDir);
        if (distance < u_PointLights[i].radius) {
            pLightDir = normalize(pLightDir);
            float pDiff = max(dot(normal, pLightDir), 0.0);

            // Attenuation: smooth falloff to 0 at radius
            float atten = clamp(1.0 - (distance / u_PointLights[i].radius), 0.0, 1.0);
            atten *= atten; // quadratic falloff

            vec3 pHalfway = normalize(pLightDir + viewDir);
            float pSpec = pow(max(dot(normal, pHalfway), 0.0), u_Shininess);

            result += u_PointLights[i].color * (pDiff * baseColor.rgb + pSpec * u_SpecularStrength) * u_PointLights[i].intensity * atten;
        }
    }

    o_Color = vec4(result, baseColor.a);
}
