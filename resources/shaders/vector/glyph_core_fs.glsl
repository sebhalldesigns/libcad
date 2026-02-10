#version 330 core

/* input from vertex shader */
in vec2 vertex_uv;
in vec4 vertex_color;

/* uniforms */
uniform sampler2D atlas_texture;

/* output */
out vec4 frag_color;

void main()
{
    /* sample SDF from atlas texture (stored in red channel) */
    float sdf_value = texture(atlas_texture, vertex_uv).r;

    /* convert SDF value to alpha with antialiasing */
    /* stb_truetype typically stores: 0 = far outside, 128 = on edge, 255 = far inside */
    /* normalize to 0-1 range */
    float normalized_sdf = sdf_value;

    /* apply smoothstep for antialiasing */
    /* adjust the range based on screen-space derivatives for scale-independent rendering */
    float edge_distance = 0.5;  /* edge is at 0.5 after normalization */
    float smoothing = length(vec2(dFdx(normalized_sdf), dFdy(normalized_sdf)));

    /* clamp smoothing to avoid artifacts */
    smoothing = clamp(smoothing, 0.001, 0.2);

    float alpha = smoothstep(edge_distance - smoothing, edge_distance + smoothing, normalized_sdf);

    /* early discard for fully transparent pixels */
    if (alpha <= 0.0)
        discard;

    /* output final color */
    frag_color = vec4(vertex_color.rgb, vertex_color.a * alpha);
}
