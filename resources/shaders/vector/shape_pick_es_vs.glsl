#version 300 es
precision highp float;

/* per-vertex attributes (quad) */
layout(location = 0) in vec2 quad_pos;  /* -1 to 1 */

/* per-instance attributes (shape data) */
layout(location = 1) in vec3 instance_center;
layout(location = 2) in vec3 instance_normal;
layout(location = 3) in vec2 instance_size;
layout(location = 4) in vec4 instance_color;       /* unused in picking, but keep layout */
layout(location = 5) in float instance_rotation;
layout(location = 6) in float instance_sides;
layout(location = 7) in float instance_start_angle;
layout(location = 8) in float instance_end_angle;
layout(location = 9) in float instance_fill;
layout(location = 10) in float instance_stroke_width;
layout(location = 11) in float instance_corner_radius;
layout(location = 12) in float instance_dash;
layout(location = 13) in float instance_entity_id;  /* entity ID for picking */

/* uniforms */
uniform mat4 projection;
uniform vec2 viewport;

/* outputs to fragment shader */
out vec2 vertex_pos;
out vec2 vertex_world_size;
out vec2 vertex_screen_size;
out float vertex_sides;
out float vertex_start_angle;
out float vertex_end_angle;
out float vertex_fill;
out float vertex_stroke_width;
out float vertex_corner_radius;
out float vertex_dash;
out float vertex_rotation;
flat out float vertex_entity_id_float;

void main()
{
    /* Create tangent/bitangent basis from normal */
    vec3 normal = normalize(instance_normal);
    vec3 ref_axis = (abs(normal.y) < 0.999) ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(ref_axis, normal));
    vec3 bitangent = normalize(cross(normal, tangent));

    /* Apply rotation */
    if (instance_rotation != 0.0) {
        float c = cos(instance_rotation);
        float s = sin(instance_rotation);
        vec3 rotated_tangent = c * tangent + s * bitangent;
        vec3 rotated_bitangent = -s * tangent + c * bitangent;
        tangent = rotated_tangent;
        bitangent = rotated_bitangent;
    }

    /* Project to screen space */
    vec4 clip_center = projection * vec4(instance_center, 1.0);
    vec3 ndc_center = clip_center.xyz / clip_center.w;
    vec2 screen_center = (ndc_center.xy * 0.5 + 0.5) * viewport;

    vec4 clip_offset_x = projection * vec4(instance_center + tangent * instance_size.x, 1.0);
    vec4 clip_offset_y = projection * vec4(instance_center + bitangent * instance_size.y, 1.0);

    vec2 screen_offset_x = (clip_offset_x.xy / clip_offset_x.w * 0.5 + 0.5) * viewport;
    vec2 screen_offset_y = (clip_offset_y.xy / clip_offset_y.w * 0.5 + 0.5) * viewport;

    float screen_width = length(screen_offset_x - screen_center);
    float screen_height = length(screen_offset_y - screen_center);
    vec2 screen_size = vec2(screen_width, screen_height);

    if (screen_width < 1.0 || screen_height < 1.0) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    /* Calculate margins (same as display shader) */
    float pixel_to_world_x = instance_size.x / max(screen_width, 1.0);
    float pixel_to_world_y = instance_size.y / max(screen_height, 1.0);
    float pixel_to_world = min(pixel_to_world_x, pixel_to_world_y);

    float margin = (instance_stroke_width * 2.0 + 2.0) * pixel_to_world;
    margin = max(margin, instance_corner_radius * 0.5);

    if (instance_sides >= 3.0 && instance_sides != 4.0) {
        margin += max(instance_size.x, instance_size.y) * 0.25;
    }

    vec2 expanded_size = instance_size + vec2(margin * 2.0);

    if (instance_rotation != 0.0) {
        float cos_r = abs(cos(instance_rotation));
        float sin_r = abs(sin(instance_rotation));
        float rotated_width = instance_size.x * cos_r + instance_size.y * sin_r;
        float rotated_height = instance_size.x * sin_r + instance_size.y * cos_r;
        expanded_size = vec2(rotated_width, rotated_height) + vec2(margin * 2.0);
    }

    vec3 offset = tangent * quad_pos.x * expanded_size.x * 0.5 +
                  bitangent * quad_pos.y * expanded_size.y * 0.5;
    vec3 world_pos = instance_center + offset;

    gl_Position = projection * vec4(world_pos, 1.0);

    /* Pass to fragment shader */
    vertex_pos = quad_pos;
    vertex_world_size = instance_size;
    vertex_screen_size = screen_size;
    vertex_sides = instance_sides;
    vertex_start_angle = instance_start_angle;
    vertex_end_angle = instance_end_angle;
    vertex_fill = instance_fill;
    vertex_stroke_width = instance_stroke_width;
    vertex_corner_radius = instance_corner_radius;
    vertex_dash = instance_dash;
    vertex_rotation = instance_rotation;
    vertex_entity_id_float = instance_entity_id;
}
