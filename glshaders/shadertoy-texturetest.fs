iChannel0 = colored_rocks
iChannel1 = rusted_metal
iChannel2 = bark_and_lichen
iChannel3 = finished_wood

ScreenPercentage = 1.2

::::::

void main(void)
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
    vec3 bl_quad = texture2D(iChannel0, uv * 2. + vec2(.0, .0)).rgb;
    vec3 br_quad = texture2D(iChannel1, uv * 2. + vec2(.0, 1.)).rgb;
    vec3 tl_quad = texture2D(iChannel2, uv * 2. + vec2(1., .0)).rgb;
    vec3 tr_quad = texture2D(iChannel3, uv * 2. + vec2(1., 1.)).rgb;     
    
    vec3 scol = bl_quad * min(smoothstep(.5, .48, uv.y),
                              smoothstep(.5, .48, uv.x));

    scol += br_quad * min(smoothstep(.5, .48, uv.y),
                         smoothstep(.48, .5, uv.x));
    
    scol += tl_quad * min(smoothstep(.48, .5, uv.y),
                         smoothstep(.5, .48, uv.x));
    
    scol += tr_quad * min(smoothstep(.48, .5, uv.y),
                          smoothstep(.48, .5, uv.x));
    
	gl_FragColor = vec4(scol,1.0);

}
