#version 330 core

in vec2 v_quad;
in float v_line_length;

uniform vec4 u_edge_color;
uniform float u_line_width;

out vec4 frag_color;

void main()
{
    float half_width = u_line_width * 0.5;
    float dist_from_center = abs(v_quad.y) * half_width;

    float alpha = 1.0 - smoothstep(half_width - 1.0, half_width + 1.0, dist_from_center);

    if (alpha <= 0.0)
    {
        discard;
    }

    frag_color = vec4(u_edge_color.rgb, u_edge_color.a * alpha);
}
