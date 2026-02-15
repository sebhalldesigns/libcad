#version 300 es
precision highp float;

#define PI 3.14159265359

/* input from vertex shader */
in vec2 vertex_pos;
in vec2 vertex_world_size;
in vec2 vertex_screen_size;
in float vertex_sides;
in float vertex_start_angle;
in float vertex_end_angle;
in float vertex_fill;
in float vertex_stroke_width;
in float vertex_corner_radius;
in float vertex_dash;
in float vertex_rotation;
flat in float vertex_entity_id_float;

/* output - entity ID encoded as color */
out vec4 pick_color;

/***************************************************************
** SDF FUNCTIONS (same as display shader)
***************************************************************/

float sdf_circle(vec2 p, float r)
{
    return length(p) - r;
}

float sdf_ellipse(vec2 p, vec2 r)
{
    float k0 = length(p / r);
    float k1 = length(p / (r * r));
    return k0 * (k0 - 1.0) / k1;
}

float sdf_rounded_box(vec2 p, vec2 size, float radius)
{
    vec2 d = abs(p) - size + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

float sdf_polygon(vec2 p, float r, int n)
{
    float min_dist = 1e10;
    bool inside = true;

    for (int i = 0; i < n; i++)
    {
        float a0 = 2.0 * PI * float(i) / float(n) - PI * 0.5 + PI;
        float a1 = 2.0 * PI * float(i + 1) / float(n) - PI * 0.5 + PI;

        vec2 v0 = r * vec2(cos(a0), sin(a0));
        vec2 v1 = r * vec2(cos(a1), sin(a1));

        float cross = (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
        inside = inside && (cross >= 0.0);

        vec2 edge = v1 - v0;
        vec2 pv = p - v0;
        float t = clamp(dot(pv, edge) / dot(edge, edge), 0.0, 1.0);
        vec2 closest = v0 + edge * t;
        float dist = length(p - closest);

        min_dist = min(min_dist, dist);
    }

    return inside ? -min_dist : min_dist;
}

bool angle_in_range(float angle, float start, float end)
{
    angle = mod(angle + PI, 2.0 * PI);
    start = mod(start + PI, 2.0 * PI);
    end = mod(end + PI, 2.0 * PI);

    if (start < end)
        return angle >= start && angle <= end;
    else
        return angle >= start || angle <= end;
}

/***************************************************************
** MAIN
***************************************************************/

void main()
{
    vec2 p = vertex_pos * vertex_world_size * 0.5;
    vec2 scale = vertex_screen_size / max(vertex_world_size, vec2(0.001));
    scale = max(scale, vec2(0.1));
    float avg_scale = (scale.x + scale.y) * 0.5;

    /* Calculate SDF */
    float dist;

    if (vertex_sides == 1.0)
    {
        if (abs(vertex_world_size.x - vertex_world_size.y) < 0.01)
            dist = sdf_circle(p, vertex_world_size.x * 0.5);
        else
            dist = sdf_ellipse(p, vertex_world_size * 0.5);
    }
    else if (vertex_sides == 4.0)
    {
        dist = sdf_rounded_box(p, vertex_world_size * 0.5, vertex_corner_radius);
    }
    else if (vertex_sides >= 3.0)
    {
        int n = int(vertex_sides);
        float r = (vertex_world_size.x + vertex_world_size.y) * 0.25;
        dist = sdf_polygon(p, r, n);
    }
    else
    {
        dist = sdf_rounded_box(p, vertex_world_size * 0.5, 0.0);
    }

    /* Handle arcs */
    if (vertex_start_angle != 0.0 || vertex_end_angle != 0.0)
    {
        float angle = atan(p.y, p.x);
        if (!angle_in_range(angle, vertex_start_angle, vertex_end_angle))
            discard;
    }

    /* For picking, make lines thicker (8px tolerance) */
    float edge_aa = 1.0 / avg_scale;
    float pick_tolerance = 8.0 / avg_scale;  /* 8 pixel picking tolerance */

    /* Check if we're within the shape or stroke */
    bool hit = false;

    if (vertex_stroke_width > 0.0)
    {
        /* Stroke with tolerance + interior fill */
        float half_stroke_world = (vertex_stroke_width * 0.5 + pick_tolerance) / avg_scale;
        bool stroke_hit = (dist >= -half_stroke_world && dist <= half_stroke_world);
        bool fill_hit = (dist <= 0.0);
        hit = stroke_hit || fill_hit;
    }
    else
    {
        /* Fill only */
        hit = (dist <= 0.0);
    }

    if (!hit)
        discard;

    /* DEBUG: Output solid color to verify rendering works */
    // pick_color = vec4(1.0, 0.0, 1.0, 1.0);  // Magenta test

    /* Encode entity ID into RGBA color */
    uint id = floatBitsToUint(vertex_entity_id_float);
    pick_color = vec4(
        float((id >>  0u) & 0xFFu) / 255.0,
        float((id >>  8u) & 0xFFu) / 255.0,
        float((id >> 16u) & 0xFFu) / 255.0,
        float((id >> 24u) & 0xFFu) / 255.0
    );
}
