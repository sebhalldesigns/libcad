#version 330 core

in vec2 fragPos;
in vec4 color;
in float radius;

out vec4 fragColor;

void main() {
    // SDF for circle in local space
    float dist = length(fragPos) - 1.0; // Circle has radius 1 in local space
    
    // Anti-aliasing with smoothstep
    float alpha = 1.0 - smoothstep(-0.02, 0.02, dist);
    
    fragColor = vec4(color.rgb, color.a * alpha);
}