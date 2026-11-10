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
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float innerCutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    int hasDiffuseMap;
    int hasSpecularMap;
    int hasNormalMap;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

uniform DirectionalLight u_DirLight;
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];
uniform int u_NumPointLights;
uniform SpotLight u_SpotLights[MAX_SPOT_LIGHTS];
uniform int u_NumSpotLights;

uniform Material u_Material;
uniform int u_UseMaterial;

uniform vec3 u_CameraPos;
uniform sampler2D u_ShadowMap;
uniform int u_HasTexture;
uniform sampler2D u_Texture;
uniform float u_SpecularStrength;
uniform float u_Shininess;
uniform vec4 u_ColorOverride;
uniform int u_UseColorOverride;
uniform int u_UseBlinnPhong; // 1 = Blinn-Phong (default), 0 = Phong

float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
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

float CalculateSpecular(vec3 normal, vec3 lightDir, vec3 viewDir, float shininess) {
    if (u_UseBlinnPhong == 1) {
        // Blinn-Phong halfway vector
        vec3 halfwayDir = normalize(lightDir + viewDir);
        return pow(max(dot(normal, halfwayDir), 0.0), shininess);
    } else {
        // Classical Phong reflection vector
        vec3 reflectDir = reflect(-lightDir, normal);
        return pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    }
}

void main() {
    vec4 baseColor = (u_UseColorOverride == 1) ? u_ColorOverride : v_Color;
    if (u_UseMaterial == 1 && u_Material.hasDiffuseMap == 1) {
        baseColor *= texture(u_Material.diffuseMap, v_TexCoord);
    } else if (u_HasTexture == 1) {
        baseColor *= texture(u_Texture, v_TexCoord);
    }

    float shininess = (u_UseMaterial == 1) ? max(u_Material.shininess, 1.0) : max(u_Shininess, 1.0);
    float specStrength = (u_UseMaterial == 1) ? 1.0 : u_SpecularStrength;

    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);

    // 1. Directional Light
    vec3 lightDir = normalize(-u_DirLight.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = CalculateSpecular(normal, lightDir, viewDir, shininess);
    float shadow = CalculateShadow(v_FragPosLightSpace, normal, lightDir);

    vec3 ambient = u_DirLight.ambient * baseColor.rgb;
    vec3 diffuse = u_DirLight.color * diff * u_DirLight.intensity * baseColor.rgb;
    vec3 specular = u_DirLight.color * spec * specStrength * u_DirLight.intensity;

    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);

    // 2. Point Lights (up to 16)
    for (int i = 0; i < u_NumPointLights && i < MAX_POINT_LIGHTS; ++i) {
        vec3 pLightDir = u_PointLights[i].position - v_FragPos;
        float distance = length(pLightDir);
        if (distance < u_PointLights[i].radius && distance > 0.001) {
            pLightDir = normalize(pLightDir);
            float pDiff = max(dot(normal, pLightDir), 0.0);
            float pSpec = CalculateSpecular(normal, pLightDir, viewDir, shininess);

            float constAtten = max(u_PointLights[i].constant, 0.1);
            float atten = 1.0 / (constAtten + u_PointLights[i].linear * distance + u_PointLights[i].quadratic * distance * distance);
            atten *= clamp(1.0 - (distance / u_PointLights[i].radius), 0.0, 1.0);

            vec3 pDiffuse = u_PointLights[i].color * pDiff * u_PointLights[i].intensity * baseColor.rgb * atten;
            vec3 pSpecular = u_PointLights[i].color * pSpec * specStrength * u_PointLights[i].intensity * atten;
            result += (pDiffuse + pSpecular);
        }
    }

    // 3. Spot Lights (up to 8)
    for (int j = 0; j < u_NumSpotLights && j < MAX_SPOT_LIGHTS; ++j) {
        vec3 sLightDir = u_SpotLights[j].position - v_FragPos;
        float sDist = length(sLightDir);
        if (sDist > 0.001) {
            sLightDir = normalize(sLightDir);
            float theta = dot(sLightDir, normalize(-u_SpotLights[j].direction));
            float epsilon = u_SpotLights[j].innerCutoff - u_SpotLights[j].outerCutoff;
            float spotIntensity = clamp((theta - u_SpotLights[j].outerCutoff) / max(epsilon, 0.0001), 0.0, 1.0);

            if (spotIntensity > 0.0) {
                float sDiff = max(dot(normal, sLightDir), 0.0);
                float sSpec = CalculateSpecular(normal, sLightDir, viewDir, shininess);

                float sAtten = 1.0 / (u_SpotLights[j].constant + u_SpotLights[j].linear * sDist + u_SpotLights[j].quadratic * sDist * sDist);
                sAtten *= spotIntensity;

                vec3 sDiffuse = u_SpotLights[j].color * sDiff * u_SpotLights[j].intensity * baseColor.rgb * sAtten;
                vec3 sSpecular = u_SpotLights[j].color * sSpec * specStrength * u_SpotLights[j].intensity * sAtten;
                result += (sDiffuse + sSpecular);
            }
        }
    }

    o_Color = vec4(result, baseColor.a);
}
