#ifdef GLSLVIEWER
uniform mat3 u_view2d;
#endif
float main_dist(vec2 r0, out vec4 colour)
{
  float r1 = 1.0/0.0;
  float r2 = length(r0);
  float r3 = 1.0;
  float r4 = r2-r3;
  vec2 r5 = vec2(r1,r4);
  float r6 = min(r5.x,r5.y);
  vec2 r7 = vec2(3,0);
  vec2 r8 = r0-r7;
  float r9 = length(r8);
  float r10 = 1.0;
  float r11 = r9-r10;
  vec2 r12 = vec2(r6,r11);
  float r13 = min(r12.x,r12.y);
  float r14 = 1.0/0.0;
  float r15 = length(r0);
  float r16 = 1.0;
  float r17 = r15-r16;
  vec2 r18 = vec2(r14,r17);
  float r19 = min(r18.x,r18.y);
  float r20 = 0.0;
  bool r21 =(r19 <= r20);
  float r22 = 1.0/0.0;
  float r23 = 0.0;
  bool r24 =(r22 <= r23);
  vec3 r25 = vec3(0,0,0);
  vec3 r26 = vec3(1,1,1);
  vec3 r27 =(r24 ? r25 : r26);
  vec3 r28 = vec3(0,0,0);
  vec3 r29 =(r21 ? r27 : r28);
  colour = vec4(r29, 1.0);
  return r13;
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
