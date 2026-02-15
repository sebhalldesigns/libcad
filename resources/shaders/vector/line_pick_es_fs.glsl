#version 300 es
precision highp float;

/* input from vertex shader */
in vec2 vertex_pos;
in float vertex_stroke_width;
in float vertex_line_length;
flat in float vertex_entity_id_float;

/* output - entity ID encoded as color */
out vec4 pick_color;

void main()
{
    /* Calculate distance from line (same logic as display shader) */
    float half_width = vertex_stroke_width * 0.5;
    float dist_from_center = abs(vertex_pos.y) * half_width;
    float pos_along_line = (vertex_pos.x + 1.0) * 0.5 * (vertex_line_length + 2.0 * half_width) - half_width;

    float dist_from_line = dist_from_center;

    if (pos_along_line < 0.0)
    {
        dist_from_line = length(vec2(pos_along_line, dist_from_center));
    }
    else if (pos_along_line > vertex_line_length)
    {
        dist_from_line = length(vec2(pos_along_line - vertex_line_length, dist_from_center));
    }

    /* Discard if outside line (no antialiasing needed for picking) */
    if (dist_from_line > half_width)
    {
        discard;
    }

    /* Encode entity ID into RGBA color */
    uint id = floatBitsToUint(vertex_entity_id_float);
    pick_color = vec4(
        float((id >>  0u) & 0xFFu) / 255.0,
        float((id >>  8u) & 0xFFu) / 255.0,
        float((id >> 16u) & 0xFFu) / 255.0,
        float((id >> 24u) & 0xFFu) / 255.0
    );
}
