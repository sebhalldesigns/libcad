#version 330 core

in vec2 v_plane;
in vec2 v_half_size;
in float v_radius;
in float v_corner_radius;
in float v_thickness_px;
in float v_filled;
flat in int v_type;
flat in int v_object_id;

out int o_pick_id;

/* simplified SDF functions for picking */
float sd_circle(vec2 p, float r) {
    return length(p) - r;
}

float sd_box(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sd_round_box(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main() {
    float dist = 0.0;

    switch (v_type) {
        case 0:
            dist = sd_circle(v_plane, v_radius);
            break;
        case 1:
            dist = sd_round_box(v_plane, v_half_size, v_corner_radius);
            break;
        default:
            dist = sd_box(v_plane, v_half_size);
            break;
    }

    /* for picking, use a larger hit area (stroke + fill) */
    float aa = fwidth(dist);
    float threshold = v_thickness_px * aa + aa * 2.0;

    if (v_filled > 0.5) {
        if (dist > threshold) discard;
    } else {
        if (abs(dist) > threshold) discard;
    }

    o_pick_id = v_object_id;
}
