#ifdef GLSLVIEWER
uniform mat3 u_view2d;
#endif
float main_dist(vec2 r0, out vec4 colour)
{
  colour = vec4(1.0, 0.0, 0.0, 1.0);
  return 0.0;
}
const vec4 bbox = vec4(-1,-1,4,1);
void mainImage( out vec4 fragColour, in vec2 fragCoord )
{
    vec2 size = bbox.zw - bbox.xy;
    vec2 scale2 = size / iResolution.xy;
    vec2 offset = bbox.xy;
    float scale;
    if (scale2.x > scale2.y) {
        scale = scale2.x;
        offset.y -= (iResolution.y*scale - size.y)/2.0;
    } else {
        scale = scale2.y;
        offset.x -= (iResolution.x*scale - size.x)/2.0;
    }
#ifdef GLSLVIEWER
    fragCoord = (u_view2d * vec3(fragCoord,1)).xy;
#endif
    float d = main_dist(fragCoord*scale+offset, fragColour);
    if (d > 0.0) {
        vec2 uv = fragCoord.xy / iResolution.xy;
        fragColour = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);
    }
}
