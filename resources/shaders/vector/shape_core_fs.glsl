#version 330 core

#define PI 3.14159265359

/* input from vertex shader */
in vec2 vertex_pos;             /* quad position (-1 to 1) */
in vec2 vertex_screen_size;     /* shape size in screen pixels */
in vec2 vertex_quad_size;       /* actual quad size (expanded for stroke/margins) */
in vec4 vertex_color;
in float vertex_sides;
in float vertex_start_angle;
in float vertex_end_angle;
in float vertex_fill;
in float vertex_stroke_width;
in float vertex_corner_radius;
in float vertex_dash;
in float vertex_rotation;

/* output */
out vec4 frag_color;

/***************************************************************
** SDF FUNCTIONS
***************************************************************/

/* SDF for circle */
float sdf_circle(vec2 p, float r)
{
    return length(p) - r;
}

/* SDF for ellipse */
float sdf_ellipse(vec2 p, vec2 r)
{
    float k0 = length(p / r);
    float k1 = length(p / (r * r));
    return k0 * (k0 - 1.0) / k1;
}

/* SDF for box with rounded corners */
float sdf_rounded_box(vec2 p, vec2 size, float radius)
{
    vec2 d = abs(p) - size + radius;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - radius;
}

/* SDF for regular polygon */
float sdf_polygon(vec2 p, float r, int n)
{
    /* compute vertices and find minimum distance to edges */
    float min_dist = 1e10;
    bool inside = true;

    for (int i = 0; i < n; i++)
    {
        /* current and next vertex angles (start at top, pointing up) */
        float a0 = 2.0 * PI * float(i) / float(n) - PI * 0.5 + PI;
        float a1 = 2.0 * PI * float(i + 1) / float(n) - PI * 0.5 + PI;

        /* vertices on circle */
        vec2 v0 = r * vec2(cos(a0), sin(a0));
        vec2 v1 = r * vec2(cos(a1), sin(a1));

        /* use cross product to test if point is on inside of edge */
        /* for CCW winding, cross > 0 means point is on left (inside) */
        float cross = (v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x);
        inside = inside && (cross >= 0.0);

        /* distance to edge segment */
        vec2 edge = v1 - v0;
        vec2 pv = p - v0;
        float t = clamp(dot(pv, edge) / dot(edge, edge), 0.0, 1.0);
        vec2 closest = v0 + edge * t;
        float dist = length(p - closest);

        min_dist = min(min_dist, dist);
    }

    return inside ? -min_dist : min_dist;
}

/* apply rotation to point */
vec2 rotate(vec2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return vec2(c * p.x - s * p.y, s * p.x + c * p.y);
}

/* angle in range check for arcs */
bool angle_in_range(float angle, float start, float end)
{
    /* normalize angles to 0-2PI */
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
    /* convert quad position to pixel space relative to shape center */
    vec2 p = vertex_pos * vertex_quad_size * 0.5;

    /* apply rotation */
    if (vertex_rotation != 0.0)
        p = rotate(p, -vertex_rotation);

    /* local color for debug modifications */
    vec4 color = vertex_color;

    /* calculate SDF distance based on shape type */
    float dist;

    if (vertex_sides == 1.0)
    {
        /* circle/ellipse */
        if (abs(vertex_screen_size.x - vertex_screen_size.y) < 0.01)
            dist = sdf_circle(p, vertex_screen_size.x * 0.5);
        else
            dist = sdf_ellipse(p, vertex_screen_size * 0.5);
    }
    else if (vertex_sides == 4.0)
    {
        /* rectangle (with optional rounded corners) */
        dist = sdf_rounded_box(p, vertex_screen_size * 0.5, vertex_corner_radius);
    }
    else if (vertex_sides >= 3.0)
    {
        /* regular polygon (triangle, pentagon, hexagon, etc.) */
        int n = int(vertex_sides);

        /* calculate bounding box for unit polygon (r=1) to find scale factor */
        float min_x = 1e10, max_x = -1e10;
        float min_y = 1e10, max_y = -1e10;

        for (int i = 0; i < n; i++)
        {
            float angle = 2.0 * PI * float(i) / float(n) - PI * 0.5 + PI;
            float vx = cos(angle);
            float vy = sin(angle);
            min_x = min(min_x, vx);
            max_x = max(max_x, vx);
            min_y = min(min_y, vy);
            max_y = max(max_y, vy);
        }

        float bbox_width = max_x - min_x;
        float bbox_height = max_y - min_y;

        /* scale radius so polygon fits in desired size */
        float r = min(vertex_screen_size.x / bbox_width, vertex_screen_size.y / bbox_height);

        dist = sdf_polygon(p, r, n);
    }
    else
    {
        /* default: rectangle */
        dist = sdf_rounded_box(p, vertex_screen_size * 0.5, 0.0);
    }

    /* handle arcs - cut out sections outside arc range */
    if (vertex_start_angle != 0.0 || vertex_end_angle != 0.0)
    {
        float angle = atan(p.y, p.x);
        if (!angle_in_range(angle, vertex_start_angle, vertex_end_angle))
            discard;
    }

    /* calculate alpha based on stroke/fill */
    float alpha = 0.0;
    float edge_aa = 1.0;  /* 1 pixel antialiasing */

    if (vertex_stroke_width > 0.0)
    {
        /* stroke mode - distance from edge */
        float half_stroke = vertex_stroke_width * 0.5;
        float stroke_dist = abs(dist) - half_stroke;
        alpha = 1.0 - smoothstep(-edge_aa, edge_aa, stroke_dist);
    }
    else
    {
        /* fill mode */
        alpha = 1.0 - smoothstep(-edge_aa, edge_aa, dist);
    }

    /* early discard for fully transparent pixels */
    if (alpha <= 0.0)
        discard;

    /* apply dashing for strokes */
    if (vertex_stroke_width > 0.0)
    {
        /* unpack dash parameters from mantissa */
        uint dash_bits = floatBitsToUint(vertex_dash);
        uint mantissa = dash_bits & 0x7FFFFFu;
        float dash_period = float(mantissa >> 11);
        float dash_duty = float(mantissa & 0x7FFu) / 2047.0;

        if (dash_period > 0.0)
        {
            /* calculate position along perimeter */
            float angle = atan(p.y, p.x);
            float r = (vertex_screen_size.x + vertex_screen_size.y) * 0.25;
            float perimeter = 2.0 * PI * r;
            float pos_along = (angle / (2.0 * PI) + 0.5) * perimeter;

            float dash_phase = mod(pos_along, dash_period) / dash_period;
            if (dash_phase > dash_duty)
                discard;
        }
    }

    /* output final color */
    frag_color = vec4(color.rgb, color.a * alpha);
}
