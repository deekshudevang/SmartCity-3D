#version 330 core

in vec3 v_TexCoords;
out vec4 o_Color;

uniform int u_HasCubemap;
uniform samplerCube u_Skybox;
uniform vec3 u_TopColor;
uniform vec3 u_HorizonColor;
uniform vec3 u_GroundColor;

void main() {
    if (u_HasCubemap == 1) {
        o_Color = texture(u_Skybox, v_TexCoords);
    } else {
        // Procedural atmospheric gradient fallback
        vec3 dir = normalize(v_TexCoords);
        float y = dir.y;

        vec3 color;
        if (y > 0.0) {
            float t = clamp(y, 0.0, 1.0);
            color = mix(u_HorizonColor, u_TopColor, t);
        } else {
            float t = clamp(-y, 0.0, 1.0);
            color = mix(u_HorizonColor, u_GroundColor, t);
        }
        o_Color = vec4(color, 1.0);
    }
}

