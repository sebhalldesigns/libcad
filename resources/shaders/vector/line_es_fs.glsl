#version 300 es
precision highp float;

/* input from vertex shader */
in vec2 vertex_pos;             /* quad position (-1 to 1) */
in vec4 vertex_color;           /* line color */
in float vertex_stroke_width;   /* line width in pixels */
in float vertex_dash;           /* dash parameter */
in float vertex_line_length;    /* line length in screen pixels */

/* output */
out vec4 frag_color;

void main()
{
    /* calculate distance from line center in screen pixels */
    float half_width = vertex_stroke_width * 0.5;
    float dist_from_center = abs(vertex_pos.y) * half_width;

    /* position along line for caps and dashing (in screen pixels) */
    /* vertex_pos.x ranges from -1 to 1, we extend by half_width on each side */
    float pos_along_line = (vertex_pos.x + 1.0) * 0.5 * (vertex_line_length + 2.0 * half_width) - half_width;

    /* round caps - calculate actual distance from line segment */
    float dist_from_line = dist_from_center;

    if (pos_along_line < 0.0)
    {
        /* before start cap - distance to start point */
        dist_from_line = length(vec2(pos_along_line, dist_from_center));
    }
    else if (pos_along_line > vertex_line_length)
    {
        /* after end cap - distance to end point */
        dist_from_line = length(vec2(pos_along_line - vertex_line_length, dist_from_center));
    }

    /* anti-aliased edge (1 pixel smooth transition) */
    /* smoothstep provides smooth alpha falloff at the edge */
    float edge_aa = 1.0;
    float alpha = 1.0 - smoothstep(half_width - edge_aa, half_width + edge_aa, dist_from_line);

    /* early discard for fully transparent pixels */
    if (alpha <= 0.0)
    {
        discard;
    }

    /* dash pattern: unpack from float mantissa (12-bit period | 11-bit duty) */
    uint dash_bits = floatBitsToUint(vertex_dash);
    uint mantissa = dash_bits & 0x7FFFFFu;  /* extract 23-bit mantissa */
    float dash_period = float(mantissa >> 11);  /* upper 12 bits: 0-4095 */
    float dash_duty = float(mantissa & 0x7FFu) / 2047.0;  /* lower 11 bits: 0-1 */

    /* apply dashing to line body only */
    if (dash_period > 0.0 && pos_along_line >= 0.0 && pos_along_line <= vertex_line_length)
    {
        float dash_phase = mod(pos_along_line, dash_period) / dash_period;
        if (dash_phase > dash_duty)
        {
            discard;
        }
    }

    /* output final color with anti-aliased edges */
    frag_color = vec4(vertex_color.rgb, vertex_color.a * alpha);


    

}
