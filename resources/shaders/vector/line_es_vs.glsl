#version 300 es
precision highp float;

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
out float vertex_dash;
out float vertex_line_length;

void main()
{
    /* transform both endpoints to clip space */
    vec4 clip_start = projection * vec4(start, 1.0);
    vec4 clip_end = projection * vec4(end, 1.0);

    /* perspective divide to get NDC coordinates */
    vec3 ndc_start = clip_start.xyz / clip_start.w;
    vec3 ndc_end = clip_end.xyz / clip_end.w;

    /* convert to screen space (pixels) */
    vec2 screen_start = (ndc_start.xy * 0.5 + 0.5) * viewport;
    vec2 screen_end = (ndc_end.xy * 0.5 + 0.5) * viewport;

    /* calculate screen-space line vector */
    vec2 line_vec = screen_end - screen_start;
    float line_length = length(line_vec);

    /* handle degenerate lines (start == end) */
    if (line_length < 0.001)
    {
        line_length = 0.001;
        line_vec = vec2(1.0, 0.0);
    }

    vec2 line_dir = line_vec / line_length;
    vec2 perp_dir = vec2(-line_dir.y, line_dir.x);

    /* extend line for round caps (in screen space pixels) */
    float half_width = stroke_width * 0.5;
    float extension = half_width;

    /* calculate screen-space position of this vertex */
    /* quad_pos.x ranges from -1 to 1 along the line */
    /* quad_pos.y ranges from -1 to 1 perpendicular to line */
    vec2 screen_center = mix(screen_start - line_dir * extension,
                            screen_end + line_dir * extension,
                            (quad_pos.x + 1.0) * 0.5);

    vec2 offset = perp_dir * quad_pos.y * half_width;
    vec2 screen_pos = screen_center + offset;

    /* convert back to NDC space */
    vec2 ndc = (screen_pos / viewport) * 2.0 - 1.0;

    /* interpolate depth from start/end points based on position along line */
    float t = (quad_pos.x + 1.0) * 0.5;
    float depth = mix(ndc_start.z, ndc_end.z, t);

    /* interpolate w component for proper perspective-correct interpolation */
    float w = mix(clip_start.w, clip_end.w, t);

    /* output final clip-space position */
    /* multiply NDC by w to get clip space, and set w component */
    gl_Position = vec4(ndc * w, depth * w, w);

    /* pass data to fragment shader */
    vertex_pos = quad_pos;
    vertex_color = color;
    vertex_stroke_width = stroke_width;
    vertex_dash = dash;
    vertex_line_length = line_length;
}

