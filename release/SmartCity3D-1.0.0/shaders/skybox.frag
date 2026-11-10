#version 330 core

in vec3 v_TexCoords;
out vec4 o_Color;

uniform int u_HasCubemap;
uniform samplerCube u_Skybox;
uniform vec3 u_TopColor;
uniform vec3 u_HorizonColor;
uniform vec3 u_GroundColor;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;
uniform float u_NightFactor; // 0 = day, 1 = deep night

// High quality pseudo-random hash for procedural night stars
float Hash31(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

void main() {
    if (u_HasCubemap == 1) {
        o_Color = texture(u_Skybox, v_TexCoords);
    } else {
        vec3 dir = normalize(v_TexCoords);
        float y = dir.y;

        vec3 skyColor;
        if (y > 0.0) {
            float t = clamp(pow(y, 0.65), 0.0, 1.0);
            skyColor = mix(u_HorizonColor, u_TopColor, t);
        } else {
            float t = clamp(pow(-y, 0.8), 0.0, 1.0);
            skyColor = mix(u_HorizonColor, u_GroundColor, t);
        }

        // 1. Sun Disc & Corona Glow
        vec3 sunDirNorm = normalize(-u_SunDir);
        if (sunDirNorm.y > -0.05) {
            float sunDot = max(dot(dir, sunDirNorm), 0.0);
            // Sharp solar disc (angular diameter ~ 0.53 deg, cos ~ 0.9998)
            float sunDisc = smoothstep(0.9985, 0.9997, sunDot);
            // Atmospheric solar corona / halo glow
            float sunGlow = pow(sunDot, 128.0) * 0.45 + pow(sunDot, 32.0) * 0.15;

            skyColor += (sunDisc * 3.5 + sunGlow) * u_SunColor;
        }

        // 2. Procedural Stars at Night
        if (u_NightFactor > 0.05 && y > 0.05) {
            vec3 starCell = floor(dir * 140.0);
            float starHash = Hash31(starCell);
            if (starHash > 0.982) {
                float brightness = (starHash - 0.982) / 0.018;
                float starFade = smoothstep(0.05, 0.35, y) * u_NightFactor;
                skyColor += vec3(0.85, 0.92, 1.0) * brightness * starFade * 1.5;
            }
        }

        o_Color = vec4(skyColor, 1.0);
    }
}
