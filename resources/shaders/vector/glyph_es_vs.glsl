#version 300 es
precision highp float;

/* per-vertex attributes (quad) */
layout(location = 0) in vec2 quad_pos;  /* -1 to 1 */

/* per-instance attributes (glyph data) */
layout(location = 1) in vec2 instance_position;
layout(location = 2) in vec2 instance_size;
layout(location = 3) in vec2 instance_uv_min;
layout(location = 4) in vec2 instance_uv_max;
layout(location = 5) in vec4 instance_color;

/* uniforms */
uniform mat4 projection;

/* outputs to fragment shader */
out vec2 vertex_uv;
out vec4 vertex_color;

void main()
{
    /* calculate world position of this vertex */
    vec2 world_pos = instance_position + quad_pos * instance_size * 0.5;

    /* transform to clip space */
    gl_Position = projection * vec4(world_pos, 0.0, 1.0);

    /* interpolate UV coordinates from quad_pos (-1 to 1) to (uv_min to uv_max) */
    vertex_uv = mix(instance_uv_min, instance_uv_max, (quad_pos + 1.0) * 0.5);

    /* pass through color */
    vertex_color = instance_color;
}
