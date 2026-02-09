#version 330 core

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

    /* dash pattern (if enabled) */
    if (vertex_dash > 0.0)
    {
        /* only apply dashing to the line body, not the caps */
        if (pos_along_line >= 0.0 && pos_along_line <= vertex_line_length)
        {
            /* decode dash parameter */
            /* for now: vertex_dash is the dash period in pixels */
            /* you could encode dash pattern more sophisticatedly later */
            float dash_period = vertex_dash;
            float dash_ratio = 0.5; /* 50% on, 50% off */

            /* calculate position within current dash cycle */
            float dash_phase = mod(pos_along_line, dash_period) / dash_period;

            /* discard if we're in the "off" part of the dash */
            if (dash_phase > dash_ratio)
            {
                discard;
            }

            /* optional: smooth dash transitions (uncomment for soft dashes) */
            /*
            float dash_aa = 2.0 / dash_period;
            float dash_alpha = smoothstep(dash_ratio - dash_aa, dash_ratio, dash_phase);
            dash_alpha = min(dash_alpha, smoothstep(1.0, 1.0 - dash_aa, dash_phase));
            alpha *= dash_alpha;
            */
        }
    }

    /* output final color with anti-aliased edges */
    frag_color = vec4(vertex_color.rgb, vertex_color.a * alpha);
}
