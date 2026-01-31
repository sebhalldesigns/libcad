#version 330 core

in vec3 v_normal;
in vec3 v_world_pos;

uniform vec4 u_color;

out vec4 FragColor;

void main()
{
    vec3 light_dir = normalize(vec3(0.3, 0.8, 0.5));
    vec3 n = normalize(v_normal);

    float ambient = 0.2;
    float diffuse = max(dot(n, light_dir), 0.0);
    float lighting = ambient + diffuse * 0.8;

    FragColor = vec4(u_color.rgb * lighting, u_color.a);
}
