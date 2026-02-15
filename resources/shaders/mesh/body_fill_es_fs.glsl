#version 300 es
precision highp float;

in vec3 v_normal;
in vec3 v_world_pos;

uniform vec4 u_fill_color;
uniform vec3 u_light_dir;

out vec4 frag_color;

void main()
{
    vec3 n = normalize(v_normal);
    float ndotl = max(dot(n, u_light_dir), 0.0);
    float ambient = 0.3;
    float diffuse = 0.7 * ndotl;
    float lighting = ambient + diffuse;
    frag_color = vec4(u_fill_color.rgb * lighting, u_fill_color.a);
}
