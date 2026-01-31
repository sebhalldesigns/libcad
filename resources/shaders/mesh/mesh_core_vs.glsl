#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 v_normal;
out vec3 v_world_pos;

void main()
{
    vec4 world_pos = model * vec4(a_pos, 1.0);
    v_world_pos = world_pos.xyz;
    v_normal = mat3(transpose(inverse(model))) * a_normal;
    gl_Position = projection * view * world_pos;
}
