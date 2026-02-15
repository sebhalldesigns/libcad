#version 330 core

/* per-vertex attributes */
layout (location = 0) in vec2 a_quad;

/* per-instance attributes */
layout (location = 1) in vec3 a_start;
layout (location = 2) in vec3 a_end;

/* uniforms */
uniform mat4 u_mvp;
uniform vec2 u_viewport;
uniform float u_line_width;

/* output to fragment shader */
out vec2 v_quad;
out float v_line_length;

void main()
{
    vec4 clip_start = u_mvp * vec4(a_start, 1.0);
    vec4 clip_end = u_mvp * vec4(a_end, 1.0);

    vec3 ndc_start = clip_start.xyz / clip_start.w;
    vec3 ndc_end = clip_end.xyz / clip_end.w;

    vec2 screen_start = (ndc_start.xy * 0.5 + 0.5) * u_viewport;
    vec2 screen_end = (ndc_end.xy * 0.5 + 0.5) * u_viewport;

    vec2 line_vec = screen_end - screen_start;
    float line_length = length(line_vec);

    if (line_length < 0.001)
    {
        line_length = 0.001;
        line_vec = vec2(1.0, 0.0);
    }

    vec2 line_dir = line_vec / line_length;
    vec2 perp_dir = vec2(-line_dir.y, line_dir.x);

    float half_width = u_line_width * 0.5;

    vec2 screen_center = mix(screen_start, screen_end, (a_quad.x + 1.0) * 0.5);
    vec2 offset = perp_dir * a_quad.y * half_width;
    vec2 screen_pos = screen_center + offset;

    vec2 ndc = (screen_pos / u_viewport) * 2.0 - 1.0;

    float t = (a_quad.x + 1.0) * 0.5;
    float depth = mix(ndc_start.z, ndc_end.z, t);
    float w = mix(clip_start.w, clip_end.w, t);

    gl_Position = vec4(ndc * w, depth * w, w);

    v_quad = a_quad;
    v_line_length = line_length;
}
