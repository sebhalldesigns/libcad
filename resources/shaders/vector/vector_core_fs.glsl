#version 330 core

in vec2 v_plane;
in vec2 v_half_size;
in float v_radius;
in float v_corner_radius;
in float v_thickness_px;
in float v_filled;
in vec4 v_color;
flat in int v_type;
flat in int v_object_id;

layout(location = 0) out vec4 o_color;
layout(location = 1) out int o_pick_id;

/* ========== SDF FUNCTIONS ========== */

/* circle SDF */
float sd_circle(vec2 p, float r) {
    return length(p) - r;
}

/* box SDF (axis-aligned rectangle) */
float sd_box(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

/* rounded box SDF */
float sd_round_box(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

/* line segment SDF */
float sd_segment(vec2 p, vec2 a, vec2 b) {
    vec2 pa = p - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

/* arc SDF - partial circle from angle a1 to a2 */
float sd_arc(vec2 p, float r, float a1, float a2) {
    float angle = atan(p.y, p.x);

    /* normalize angles */
    float mid = (a1 + a2) * 0.5;
    float half_span = abs(a2 - a1) * 0.5;

    float ang_diff = angle - mid;
    /* wrap to [-PI, PI] */
    ang_diff = mod(ang_diff + 3.141592653589793, 6.283185307179586) - 3.141592653589793;

    if (abs(ang_diff) <= half_span) {
        /* on the arc */
        return abs(length(p) - r);
    } else {
        /* closest to endpoints */
        vec2 p1 = r * vec2(cos(a1), sin(a1));
        vec2 p2 = r * vec2(cos(a2), sin(a2));
        return min(length(p - p1), length(p - p2));
    }
}

/* triangle SDF */
float sd_triangle(vec2 p, vec2 p0, vec2 p1, vec2 p2) {
    vec2 e0 = p1 - p0, e1 = p2 - p1, e2 = p0 - p2;
    vec2 v0 = p - p0, v1 = p - p1, v2 = p - p2;

    vec2 pq0 = v0 - e0 * clamp(dot(v0, e0) / dot(e0, e0), 0.0, 1.0);
    vec2 pq1 = v1 - e1 * clamp(dot(v1, e1) / dot(e1, e1), 0.0, 1.0);
    vec2 pq2 = v2 - e2 * clamp(dot(v2, e2) / dot(e2, e2), 0.0, 1.0);

    float s = sign(e0.x * e2.y - e0.y * e2.x);
    vec2 d = min(min(
        vec2(dot(pq0, pq0), s * (v0.x * e0.y - v0.y * e0.x)),
        vec2(dot(pq1, pq1), s * (v1.x * e1.y - v1.y * e1.x))),
        vec2(dot(pq2, pq2), s * (v2.x * e2.y - v2.y * e2.x)));

    return -sqrt(d.x) * sign(d.y);
}

/* regular polygon SDF */
float sd_polygon(vec2 p, float r, int n) {
    float an = 3.141592653589793 / float(n);
    vec2 acs = vec2(cos(an), sin(an));

    /* reduce to first sector */
    float bn = mod(atan(p.y, p.x), 2.0 * an) - an;
    p = length(p) * vec2(cos(bn), abs(sin(bn)));

    p -= r * acs;
    p.y += clamp(-p.y, 0.0, r * acs.y);

    return length(p) * sign(p.x);
}

/* ellipse SDF (approximate) */
float sd_ellipse(vec2 p, vec2 ab) {
    p = abs(p);
    if (p.x > p.y) { p = p.yx; ab = ab.yx; }
    float l = ab.y * ab.y - ab.x * ab.x;
    float m = ab.x * p.x / l;
    float m2 = m * m;
    float n = ab.y * p.y / l;
    float n2 = n * n;
    float c = (m2 + n2 - 1.0) / 3.0;
    float c3 = c * c * c;
    float q = c3 + m2 * n2 * 2.0;
    float d = c3 + m2 * n2;
    float g = m + m * n2;
    float co;
    if (d < 0.0) {
        float h = acos(q / c3) / 3.0;
        float s = cos(h);
        float t = sin(h) * sqrt(3.0);
        float rx = sqrt(-c * (s + t + 2.0) + m2);
        float ry = sqrt(-c * (s - t + 2.0) + m2);
        co = (ry + sign(l) * rx + abs(g) / (rx * ry) - m) / 2.0;
    } else {
        float h = 2.0 * m * n * sqrt(d);
        float s = sign(q + h) * pow(abs(q + h), 1.0 / 3.0);
        float u = sign(q - h) * pow(abs(q - h), 1.0 / 3.0);
        float rx = -s - u - c * 4.0 + 2.0 * m2;
        float ry = (s - u) * sqrt(3.0);
        float rm = sqrt(rx * rx + ry * ry);
        co = (ry / sqrt(rm - rx) + 2.0 * g / rm - m) / 2.0;
    }
    vec2 r = ab * vec2(co, sqrt(1.0 - co * co));
    return length(r - p) * sign(p.y - r.y);
}

/* ========== ALPHA FUNCTIONS ========== */

/* anti-aliased fill */
float alpha_fill(float dist) {
    float aa = fwidth(dist) * 1.5;
    return 1.0 - smoothstep(-aa, aa, dist);
}

/* stroke with consistent thickness */
float alpha_stroke(float dist, float thickness_px) {
    float aa = fwidth(dist);
    float half_t = thickness_px * aa * 0.5;
    return 1.0 - smoothstep(half_t - aa, half_t + aa, abs(dist));
}

/* ========== MAIN ========== */

void main() {
    float dist = 0.0;

    /* shape type dispatch */
    switch (v_type) {
        case 0:  /* circle */
            dist = sd_circle(v_plane, v_radius);
            break;
        case 1:  /* rounded rectangle */
            dist = sd_round_box(v_plane, v_half_size, v_corner_radius);
            break;
        case 2:  /* line segment - endpoints encoded in half_size and radius/corner_radius */
            {
                vec2 a = vec2(-v_half_size.x, -v_half_size.y);
                vec2 b = vec2(v_radius, v_corner_radius);
                dist = sd_segment(v_plane, a, b);
            }
            break;
        case 3:  /* arc - radius in v_radius, angles in half_size */
            dist = sd_arc(v_plane, v_radius, v_half_size.x, v_half_size.y);
            break;
        case 4:  /* triangle - vertices encoded */
            {
                vec2 p0 = vec2(-v_half_size.x, -v_half_size.y * 0.577);
                vec2 p1 = vec2(v_half_size.x, -v_half_size.y * 0.577);
                vec2 p2 = vec2(0.0, v_half_size.y * 1.155);
                dist = sd_triangle(v_plane, p0, p1, p2);
            }
            break;
        case 5:  /* regular polygon - n sides in corner_radius */
            dist = sd_polygon(v_plane, v_radius, int(v_corner_radius));
            break;
        case 6:  /* ellipse */
            dist = sd_ellipse(v_plane, v_half_size);
            break;
        case 7:  /* rectangle (non-rounded) */
            dist = sd_box(v_plane, v_half_size);
            break;
        default:
            dist = sd_circle(v_plane, v_radius);
            break;
    }

    /* compute alpha */
    float a_fill = alpha_fill(dist);
    float a_stroke = alpha_stroke(dist, v_thickness_px);
    float a = mix(a_stroke, max(a_fill, a_stroke), clamp(v_filled, 0.0, 1.0));

    /* discard fully transparent pixels */
    if (a <= 0.001) discard;

    /* output color */
    o_color = vec4(v_color.rgb, v_color.a * a);

    /* output pick ID */
    o_pick_id = v_object_id;
}
