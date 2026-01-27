#version 330 core

in vec2 v_plane;
in vec2 v_half_size;
in float v_radius;
in float v_corner_radius;
in float v_thickness_px;
in float v_filled;
in vec4 v_color;
flat in int v_type;

out vec4 o_color;

/* signed distance functions in plane units */
float sd_circle(vec2 p, float r) {
    return length(p) - r;
}

float sd_round_rect(vec2 p, vec2 b, float r) {
    /* b = half extents */
    vec2 q = abs(p) - (b - vec2(r));
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

/* AA fill alpha for shape interior (dist < 0) */
float alpha_fill(float dist) {
    float aa = fwidth(dist);
    return 1.0 - smoothstep(0.0 - aa, 0.0 + aa, dist);
}

/* pixel-constant stroke alpha around dist=0 */
float alpha_stroke_px(float dist, float thickness) {
    /* dist is in plane units; fwidth(dist) ~= planeUnits per pixel near the edge */
    float aa = fwidth(dist);
    float t = thickness * aa;  /* convert px thickness to plane units */
    float a = 1.0 - smoothstep(t - aa, t + aa, abs(dist));
    return a;
}

void main() {

    float dist;
    if (v_type == 0) {  /* circle */
        dist = sd_circle(v_plane, v_radius);
    } else {  /* rounded rect */
        dist = sd_round_rect(v_plane, v_half_size, v_corner_radius);
    }

    float a_fill = alpha_fill(dist);
    float a_stroke = alpha_stroke_px(dist, v_thickness_px);

    float a = mix(a_stroke, max(a_fill, a_stroke), clamp(v_filled, 0.0, 1.0));

    /* discard when fully transparent (helps fillrate) */
    if (a <= 0.0) discard;

    /* straight alpha output */
    o_color = vec4(v_color.rgb, v_color.a * a);
}
