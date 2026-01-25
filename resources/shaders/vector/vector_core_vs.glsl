#version 330 core

layout(location = 0) in vec2 v_pos; /* per-vertex */

// Per-instance attributes
layout(location = 1) in vec3 i_center;
layout(location = 2) in float i_type;

layout(location = 3) in vec3 i_axis_x;
layout(location = 4) in float i_radius;

layout(location = 5) in vec3 i_axis_y;
layout(location = 6) in float i_corner_radius;

layout(location = 7) in vec2 i_half_size;
layout(location = 8) in float i_thickness_px;
layout(location = 9) in float i_filled;

layout(location = 10) in vec4 i_color;

uniform mat4 u_view_projection;

out vec2 v_plane;              
out vec2 v_half_size;
out float v_radius;
out float v_corner_radius;
out float v_thickness_px;
out float v_filled;
out vec4 v_color;
flat out int v_type;

void main() {
    vec3 world_pos = i_center + v_pos.x * i_axis_x + v_pos.y * i_axis_y;
    gl_Position = u_view_projection * vec4(world_pos, 1.0);

    // plane-local p. If axis lengths already represent half-extents,
    // then vP should be in those same plane units.
    v_plane = vec2(v_pos.x, v_pos.y) * i_half_size;

    v_half_size = i_half_size;
    v_radius = i_radius;
    v_corner_radius = i_corner_radius;
    v_thickness_px = i_thickness_px;
    v_filled = i_filled;
    v_color = i_color;
    v_type = int(i_type);
}