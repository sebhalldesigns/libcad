#version 300 es
precision highp float;

/* per-vertex attributes */
layout (location = 0) in vec2 quad_pos;

/* per-instance attributes */
layout (location = 1) in vec3 start;
layout (location = 2) in vec3 end;
layout (location = 3) in vec4 color;          /* unused in picking */
layout (location = 4) in float stroke_width;
layout (location = 5) in float dash;          /* unused in picking */
layout (location = 6) in float entity_id;     /* entity ID for picking */

/* uniforms */
uniform mat4 projection;
uniform vec2 viewport;

/* output to fragment shader */
out vec2 vertex_pos;
out float vertex_stroke_width;
out float vertex_line_length;
flat out float vertex_entity_id_float;

void main()
{
    /* Transform endpoints to clip space */
    vec4 clip_start = projection * vec4(start, 1.0);
    vec4 clip_end = projection * vec4(end, 1.0);

    /* Perspective divide */
    vec3 ndc_start = clip_start.xyz / clip_start.w;
    vec3 ndc_end = clip_end.xyz / clip_end.w;

    /* Convert to screen space */
    vec2 screen_start = (ndc_start.xy * 0.5 + 0.5) * viewport;
    vec2 screen_end = (ndc_end.xy * 0.5 + 0.5) * viewport;

    /* Calculate line vector */
    vec2 line_vec = screen_end - screen_start;
    float line_length = length(line_vec);

    if (line_length < 0.001)
    {
        line_length = 0.001;
        line_vec = vec2(1.0, 0.0);
    }

    vec2 line_dir = line_vec / line_length;
    vec2 perp_dir = vec2(-line_dir.y, line_dir.x);

    /* Make lines slightly thicker for picking */
    float pick_width = stroke_width + 2.0;
    float half_width = pick_width * 0.5;
    float extension = half_width;

    /* Calculate vertex position */
    vec2 screen_center = mix(screen_start - line_dir * extension,
                            screen_end + line_dir * extension,
                            (quad_pos.x + 1.0) * 0.5);

    vec2 offset = perp_dir * quad_pos.y * half_width;
    vec2 screen_pos = screen_center + offset;

    /* Convert back to NDC */
    vec2 ndc = (screen_pos / viewport) * 2.0 - 1.0;

    /* Interpolate depth */
    float t = (quad_pos.x + 1.0) * 0.5;
    float depth = mix(ndc_start.z, ndc_end.z, t);
    float w = mix(clip_start.w, clip_end.w, t);

    gl_Position = vec4(ndc * w, depth * w, w);

    /* Pass to fragment shader */
    vertex_pos = quad_pos;
    vertex_stroke_width = pick_width;
    vertex_line_length = line_length;
    vertex_entity_id_float = entity_id;
}
