#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 3) in vec4 a_Color;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform vec4 u_EmissiveColor;

out vec4 v_Color;

void main() {
    v_Color = u_EmissiveColor;
    gl_Position = u_ViewProjection * u_Model * vec4(a_Position, 1.0);
}
