#version 330 core

layout(location = 0) in vec2 quadVertex; // (-1,-1) to (1,1)
layout(location = 1) in vec2 instancePos;
layout(location = 2) in float instanceRadius;
layout(location = 3) in vec4 instanceColor;

out vec2 fragPos;
out vec4 color;
out float radius;

uniform mat4 projection;

void main() {
    // Scale quad by radius and translate to instance position
    vec2 worldPos = instancePos + quadVertex * instanceRadius;
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);
    
    fragPos = quadVertex; // Local space [-1, 1]
    color = instanceColor;
    radius = instanceRadius;
}