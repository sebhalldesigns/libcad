#version 300 es
precision highp float;

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
out vec2 vertex_world_size;      /* shape size in world units */
out vec2 vertex_screen_size;     /* shape size in screen pixels */
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
    /* Create tangent/bitangent basis from normal for proper 3D orientation */
    vec3 normal = normalize(instance_normal);

    /* Use a reference axis that is not parallel to the normal */
    vec3 ref_axis = (abs(normal.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(ref_axis, normal));
    vec3 bitangent = normalize(cross(normal, tangent));

    /* Apply rotation to tangent/bitangent basis (rotation in the plane) */
    if (instance_rotation != 0.0) {
        float c = cos(instance_rotation);
        float s = sin(instance_rotation);
        vec3 rotated_tangent = c * tangent + s * bitangent;
        vec3 rotated_bitangent = -s * tangent + c * bitangent;
        tangent = rotated_tangent;
        bitangent = rotated_bitangent;
    }

    /* Project center to screen space to calculate screen-space sizes for strokes */
    vec4 clip_center = projection * vec4(instance_center, 1.0);
    vec3 ndc_center = clip_center.xyz / clip_center.w;
    vec2 screen_center = (ndc_center.xy * 0.5 + 0.5) * viewport;

    /* Project size vectors to screen space to determine pixel scale */
    vec4 clip_offset_x = projection * vec4(instance_center + tangent * instance_size.x, 1.0);
    vec4 clip_offset_y = projection * vec4(instance_center + bitangent * instance_size.y, 1.0);

    vec2 screen_offset_x = (clip_offset_x.xy / clip_offset_x.w * 0.5 + 0.5) * viewport;
    vec2 screen_offset_y = (clip_offset_y.xy / clip_offset_y.w * 0.5 + 0.5) * viewport;

    /* Calculate screen-space size in pixels for stroke/antialiasing */
    float screen_width = length(screen_offset_x - screen_center);
    float screen_height = length(screen_offset_y - screen_center);
    vec2 screen_size = vec2(screen_width, screen_height);

    /* Handle degenerate cases - if shape is too thin in screen space, skip rendering */
    if (screen_width < 1.0 || screen_height < 1.0) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);  /* degenerate position */
        return;
    }

    /* Calculate world-space margin for stroke (convert pixels to world units) */
    /* Use min instead of average to prevent extreme values at edge-on views */
    float pixel_to_world_x = instance_size.x / max(screen_width, 1.0);
    float pixel_to_world_y = instance_size.y / max(screen_height, 1.0);
    float pixel_to_world = min(pixel_to_world_x, pixel_to_world_y);

    /* Account for stroke width, antialiasing, and corner radius */
    float margin = (instance_stroke_width * 2.0 + 2.0) * pixel_to_world;
    margin = max(margin, instance_corner_radius * 0.5);  /* Extra margin for rounded corners */

    /* For polygons, we need extra margin since vertices can extend beyond */
    /* the nominal size when calculating from bounding box */
    if (instance_sides >= 3.0 && instance_sides != 4.0) {
        margin += max(instance_size.x, instance_size.y) * 0.25;
    }

    /* Start with instance size plus margin */
    vec2 expanded_size = instance_size + vec2(margin * 2.0);

    /* For rotated shapes, expand to fit rotated bounding box */
    /* (rotation is in the plane, so axis-aligned bounds grow) */
    if (instance_rotation != 0.0) {
        float cos_r = abs(cos(instance_rotation));
        float sin_r = abs(sin(instance_rotation));
        float rotated_width = instance_size.x * cos_r + instance_size.y * sin_r;
        float rotated_height = instance_size.x * sin_r + instance_size.y * cos_r;
        expanded_size = vec2(rotated_width, rotated_height) + vec2(margin * 2.0);
    }

    /* Calculate world-space corner position on the plane */
    vec3 offset = tangent * quad_pos.x * expanded_size.x * 0.5 +
                  bitangent * quad_pos.y * expanded_size.y * 0.5;
    vec3 world_pos = instance_center + offset;

    /* Project to clip space */
    gl_Position = projection * vec4(world_pos, 1.0);

    /* pass through to fragment shader */
    vertex_pos = quad_pos;
    vertex_world_size = instance_size;     /* actual shape size in world units */
    vertex_screen_size = screen_size;      /* screen-space size for stroke calculations */
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
