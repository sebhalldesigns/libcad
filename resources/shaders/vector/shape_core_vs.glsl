#version 330 core

/* per-vertex attributes (quad) */
layout(location = 0) in vec2 quad_pos;  /* -1 to 1 */

/* per-instance attributes (shape data) */
layout(location = 1) in vec3 instance_center;
layout(location = 2) in vec3 instance_normal;
layout(location = 3) in vec2 instance_size;
layout(location = 4) in vec4 instance_color;
layout(location = 5) in float instance_rotation;
layout(location = 6) in float instance_sides;
layout(location = 7) in float instance_start_angle;
layout(location = 8) in float instance_end_angle;
layout(location = 9) in float instance_fill;
layout(location = 10) in float instance_stroke_width;
layout(location = 11) in float instance_corner_radius;
layout(location = 12) in float instance_dash;

/* uniforms */
uniform mat4 projection;
uniform vec2 viewport;

/* outputs to fragment shader */
out vec2 vertex_pos;             /* quad position (-1 to 1) */
out vec2 vertex_screen_size;     /* shape size in screen pixels */
out vec2 vertex_quad_size;       /* actual quad size in screen pixels (expanded) */
out vec4 vertex_color;
out float vertex_sides;
out float vertex_start_angle;
out float vertex_end_angle;
out float vertex_fill;
out float vertex_stroke_width;
out float vertex_corner_radius;
out float vertex_dash;
out float vertex_rotation;

void main()
{
    /* transform center to clip space */
    vec4 clip_center = projection * vec4(instance_center, 1.0);

    /* perspective divide to get NDC */
    vec3 ndc_center = clip_center.xyz / clip_center.w;

    /* convert to screen space (pixels) */
    vec2 screen_center = (ndc_center.xy * 0.5 + 0.5) * viewport;

    /* calculate world-space size at the center's depth */
    /* project a point offset by instance_size to see how big it is in screen space */
    vec4 clip_offset_x = projection * vec4(instance_center + vec3(instance_size.x, 0.0, 0.0), 1.0);
    vec4 clip_offset_y = projection * vec4(instance_center + vec3(0.0, instance_size.y, 0.0), 1.0);

    vec2 screen_offset_x = (clip_offset_x.xy / clip_offset_x.w * 0.5 + 0.5) * viewport;
    vec2 screen_offset_y = (clip_offset_y.xy / clip_offset_y.w * 0.5 + 0.5) * viewport;

    /* calculate screen-space size */
    float screen_width = length(screen_offset_x - screen_center);
    float screen_height = length(screen_offset_y - screen_center);
    vec2 screen_size = vec2(screen_width, screen_height);

    /* expand for stroke width and extra margin for polygons */
    /* for polygons, vertices extend to circumradius which may exceed size */
    float margin = instance_stroke_width * 2.0 + 2.0;  /* stroke + 2px antialiasing */

    /* for polygons (sides >= 3), add extra margin since circumradius > half-size */
    if (instance_sides >= 3.0 && instance_sides != 4.0)
        margin += max(screen_size.x, screen_size.y) * 0.5;

    vec2 expanded_size = screen_size + vec2(margin);

    /* for rotated shapes, expand to fit the rotated bounding box */
    if (instance_rotation != 0.0) {
        float cos_r = abs(cos(instance_rotation));
        float sin_r = abs(sin(instance_rotation));
        float new_width = screen_size.x * cos_r + screen_size.y * sin_r;
        float new_height = screen_size.x * sin_r + screen_size.y * cos_r;
        expanded_size = vec2(new_width, new_height) + vec2(margin);
    }

    /* create screen-space quad */
    vec2 screen_pos = screen_center + quad_pos * expanded_size * 0.5;

    /* convert back to NDC */
    vec2 ndc = (screen_pos / viewport) * 2.0 - 1.0;

    /* output final position with preserved depth */
    gl_Position = vec4(ndc * clip_center.w, ndc_center.z * clip_center.w, clip_center.w);

    /* pass through to fragment shader */
    vertex_pos = quad_pos;
    vertex_screen_size = screen_size;
    vertex_quad_size = expanded_size;
    vertex_color = instance_color;
    vertex_sides = instance_sides;
    vertex_start_angle = instance_start_angle;
    vertex_end_angle = instance_end_angle;
    vertex_fill = instance_fill;
    vertex_stroke_width = instance_stroke_width;
    vertex_corner_radius = instance_corner_radius;
    vertex_dash = instance_dash;
    vertex_rotation = instance_rotation;
}
