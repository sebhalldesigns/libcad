#version 330 core

/* per-vertex attributes */
layout (location = 0) in vec2 quad_pos;

/* per-instance attributes */
layout (location = 1) in vec3 start;
layout (location = 2) in vec3 end;
layout (location = 3) in vec4 color;
layout (location = 4) in float stroke_width;
layout (location = 5) in float dash;

/* uniforms */
uniform mat4 projection;
uniform vec2 viewport;

/* output to fragment shader */
out vec2 vertex_pos;
out vec4 vertex_color;
out float vertex_stroke_width;
out float vertex_line_length;
out float vertex_depth01;
out vec2 vertex_center_ndc;
flat out vec4 axis_clip_origin;
flat out vec4 axis_clip_dir;
flat out float axis_ref_half_length;

void main()
{
    vec3 origin = (start + end) * 0.5;
    vec3 direction = end - start;
    float dir_len = length(direction);
    if (dir_len < 1e-6) {
        direction = vec3(1.0, 0.0, 0.0);
    } else {
        direction /= dir_len;
    }

    vec4 clip_origin = projection * vec4(origin, 1.0);
    const float min_w = 1e-3;
    if (clip_origin.w <= min_w)
    {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        vertex_pos = vec2(0.0);
        vertex_color = vec4(0.0);
        vertex_stroke_width = 0.0;
        vertex_line_length = 0.0;
        vertex_depth01 = 1.0;
        vertex_center_ndc = vec2(0.0);
        axis_clip_origin = vec4(0.0, 0.0, 0.0, 1.0);
        axis_clip_dir = vec4(1.0, 0.0, 0.0, 0.0);
        axis_ref_half_length = 1.0;
        return;
    }

    vec3 ndc_origin = clip_origin.xyz / clip_origin.w;
    vec2 dir_ndc = vec2(0.0);
    vec4 clip_dir = projection * vec4(direction, 0.0);

    vec4 clip_dir_pos = projection * vec4(origin + direction, 1.0);
    if (clip_dir_pos.w > min_w) {
        dir_ndc = clip_dir_pos.xy / clip_dir_pos.w - ndc_origin.xy;
    }
    if (length(dir_ndc) < 1e-5)
    {
        vec4 clip_dir_neg = projection * vec4(origin - direction, 1.0);
        if (clip_dir_neg.w > min_w) {
            dir_ndc = ndc_origin.xy - clip_dir_neg.xy / clip_dir_neg.w;
        }
    }
    if (length(dir_ndc) < 1e-5) {
        dir_ndc = vec2(1.0, 0.0);
    }

    vec2 line_dir_ndc = normalize(dir_ndc);
    float ndc_extent = 4.0;
    vec2 ndc_a = ndc_origin.xy - line_dir_ndc * ndc_extent;
    vec2 ndc_b = ndc_origin.xy + line_dir_ndc * ndc_extent;

    vec2 screen_start = (ndc_a * 0.5 + 0.5) * viewport;
    vec2 screen_end = (ndc_b * 0.5 + 0.5) * viewport;

    vec2 line_vec = screen_end - screen_start;
    float line_length = length(line_vec);
    if (line_length < 0.001)
    {
        line_length = 0.001;
        line_vec = vec2(1.0, 0.0);
    }

    vec2 line_dir = line_vec / line_length;
    vec2 perp_dir = vec2(-line_dir.y, line_dir.x);

    float half_width = stroke_width * 0.5;
    float extension = half_width;
    vec2 screen_center = mix(screen_start - line_dir * extension,
                             screen_end + line_dir * extension,
                             (quad_pos.x + 1.0) * 0.5);
    vec2 offset = perp_dir * quad_pos.y * half_width;
    vec2 screen_pos = screen_center + offset;

    vec2 ndc = (screen_pos / viewport) * 2.0 - 1.0;
    float depth_ndc = clamp(ndc_origin.z, -1.0, 1.0);
    gl_Position = vec4(ndc, depth_ndc, 1.0);

    vertex_pos = quad_pos;
    vertex_color = color;
    vertex_stroke_width = stroke_width;
    vertex_line_length = line_length;
    vertex_depth01 = depth_ndc * 0.5 + 0.5;
    vertex_center_ndc = (screen_center / viewport) * 2.0 - 1.0;
    axis_clip_origin = clip_origin;
    axis_clip_dir = clip_dir;
    axis_ref_half_length = max(length(end - start) * 0.5, 1e-3);
}
