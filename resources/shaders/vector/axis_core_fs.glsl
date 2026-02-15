#version 330 core

in vec2 vertex_pos;
in vec4 vertex_color;
in float vertex_stroke_width;
in float vertex_line_length;
in float vertex_depth01;
in vec2 vertex_center_ndc;
flat in vec4 axis_clip_origin;
flat in vec4 axis_clip_dir;
flat in float axis_ref_half_length;

out vec4 frag_color;

static float solve_axis_param(float ndc_component, float clip_origin_component, float clip_dir_component, float clip_origin_w, float clip_dir_w)
{
    float denom = ndc_component * clip_dir_w - clip_dir_component;
    if (abs(denom) < 1e-6) {
        return 0.0;
    }
    return (clip_origin_component - ndc_component * clip_origin_w) / denom;
}

void main()
{
    float half_width = vertex_stroke_width * 0.5;
    float dist_from_center = abs(vertex_pos.y) * half_width;
    float pos_along_line = (vertex_pos.x + 1.0) * 0.5 * (vertex_line_length + 2.0 * half_width) - half_width;

    float dist_from_line = dist_from_center;
    if (pos_along_line < 0.0) {
        dist_from_line = length(vec2(pos_along_line, dist_from_center));
    } else if (pos_along_line > vertex_line_length) {
        dist_from_line = length(vec2(pos_along_line - vertex_line_length, dist_from_center));
    }

    float edge_aa = 1.0;
    float alpha = 1.0 - smoothstep(half_width - edge_aa, half_width + edge_aa, dist_from_line);
    if (alpha <= 0.0) {
        discard;
    }

    float denom_x = abs(vertex_center_ndc.x * axis_clip_dir.w - axis_clip_dir.x);
    float denom_y = abs(vertex_center_ndc.y * axis_clip_dir.w - axis_clip_dir.y);
    float axis_t_x = solve_axis_param(vertex_center_ndc.x, axis_clip_origin.x, axis_clip_dir.x, axis_clip_origin.w, axis_clip_dir.w);
    float axis_t_y = solve_axis_param(vertex_center_ndc.y, axis_clip_origin.y, axis_clip_dir.y, axis_clip_origin.w, axis_clip_dir.w);
    float denom_sum = denom_x + denom_y;
    float axis_t_world = (denom_sum > 1e-6)
        ? ((axis_t_x * denom_x + axis_t_y * denom_y) / denom_sum)
        : 0.0;

    float axis_clip_w = axis_clip_origin.w + axis_t_world * axis_clip_dir.w;
    if (axis_clip_w <= 1e-4) {
        discard;
    }

    float world_dist = abs(axis_t_world);
    float haze_start_world = axis_ref_half_length * 0.6;
    float haze_end_world = axis_ref_half_length * 2.4;
    float fog = smoothstep(haze_start_world, haze_end_world, world_dist);
    float haze_alpha = 1.0 - fog;
    frag_color = vec4(vertex_color.rgb, vertex_color.a * alpha * haze_alpha);
}
