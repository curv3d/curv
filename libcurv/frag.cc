// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/frag.h>

#include <libcurv/glsl.h>
#include <libcurv/shape.h>

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/format.h>
#include <libcurv/function.h>

namespace curv {

void export_frag_2d(const Shape_Program&, const Render_Opts&, std::ostream&);
void export_frag_3d(const Shape_Program&, const Render_Opts&, std::ostream&);

void export_frag(
    const Shape_Program& shape, const Render_Opts& opts, std::ostream& out)
{
    if (shape.is_2d_)
        return export_frag_2d(shape, opts, out);
    if (shape.is_3d_)
        return export_frag_3d(shape, opts, out);
    die("export_frag: shape is not 2d or 3d");
}

void export_frag_2d(
    const Shape_Program& shape, const Render_Opts& opts, std::ostream& out)
{
    out <<
        "#define AA " << opts.aa_ << "\n"
        "#define TAA " << opts.taa_ << "\n"
        "#define FDUR " << opts.fdur_ << "\n"
        "const vec3 background_colour = vec3("
            << opts.bg_.x << ","
            << opts.bg_.y << ","
            << opts.bg_.z << ");\n"
        "uniform mat3 u_view2d;\n";

    glsl_function_export(shape, out);

    BBox bbox = shape.bbox_;
    if (bbox.empty2() || bbox.infinite2()) {
        out <<
        "const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);\n";
    } else {
        out << "const vec4 bbox = vec4("
            << bbox.min.x << ","
            << bbox.min.y << ","
            << bbox.max.x << ","
            << bbox.max.y
            << ");\n";
    }
    out <<
        "void mainImage( out vec4 fragColour, in vec2 fragCoord )\n"
        "{\n"
        "    vec2 size = bbox.zw - bbox.xy;\n"
        "    vec2 scale2 = size / iResolution.xy;\n"
        "    vec2 offset = bbox.xy;\n"
        "    float scale;\n"
        "    if (scale2.x > scale2.y) {\n"
        "        scale = scale2.x;\n"
        "        offset.y -= (iResolution.y*scale - size.y)/2.0;\n"
        "    } else {\n"
        "        scale = scale2.y;\n"
        "        offset.x -= (iResolution.x*scale - size.x)/2.0;\n"
        "    }\n"
        "    vec3 col = vec3(0.0);\n"
        "#if AA>1\n"
        "  for (int m=0; m<AA; ++m)\n"
        "  for (int n=0; n<AA; ++n) {\n"
        "    vec2 jitter = vec2(float(m),float(n)) / float(AA) - 0.5;\n"
        "#else\n"
        "    const vec2 jitter = vec2(0.0);\n"
        "#endif\n"
        "    vec2 xy = fragCoord + jitter;\n"
        "    xy = (u_view2d * vec3(xy,1)).xy;\n"
        "#if TAA>1\n"
        "  for (int t=0; t<TAA; ++t) {\n"
        "    float time = iTime + float(t)/float(TAA)*float(FDUR);\n"
        "#else\n"
        "    float time = iTime;\n"
        "#endif\n"
        "    vec4 p = vec4(xy*scale+offset,0,time);\n"
        "    float d = dist(p);\n"
        "    if (d > 0.0) {\n"
        "        col += background_colour;\n"
        "    } else {\n"
        "        col += colour(p);\n"
        "    }\n"
        "    \n"
        "#if TAA>1\n"
        "  }\n"
        "#endif\n"
        "#if AA>1\n"
        "  }\n"
        "#endif\n"
        "#if AA>1 || TAA>1\n"
        "    col /= float(AA*AA*TAA);\n"
        "#endif\n"
        "    // convert linear RGB to sRGB\n"
        "    fragColour = vec4(pow(col, vec3(0.454545454545454545)),1.0);\n"
        "}\n"
        ;
}

void export_sf1(
    const Shape_Program& shape, const Render_Opts& opts, std::ostream& out)
{
    SC_Compiler sc(out, SC_Target::glsl, shape.sstate_);
    sc.define_function(
        "sf1",
        std::vector<SC_Type>{
            SC_Type::Num(3),SC_Type::Num(3),SC_Type::Num(3),SC_Type::Num(3)},
        SC_Type::Num(3),
        opts.sf1_,
        At_Program(shape));
}

void export_frag_3d(
    const Shape_Program& shape, const Render_Opts& opts, std::ostream& out)
{
    out <<
        "#define AA " << opts.aa_ << "\n"
        "#define TAA " << opts.taa_ << "\n"
        "#define FDUR " << opts.fdur_ << "\n"
        "const vec3 background_colour = vec3("
            << opts.bg_.x << ","
            << opts.bg_.y << ","
            << opts.bg_.z << ");\n"
        "const int ray_max_iter = " << opts.ray_max_iter_ << ";\n"
        "const float ray_max_depth = " << dfmt(opts.ray_max_depth_, dfmt::EXPR) << ";\n"
        "uniform vec3 u_eye3d;\n"
        "uniform vec3 u_centre3d;\n"
        "uniform vec3 u_up3d;\n";

    glsl_function_export(shape, out);

    BBox bbox = shape.bbox_;
    if (bbox.empty3() || bbox.infinite3()) {
        out <<
        "const vec3 bbox_min = vec3(-10.0,-10.0,-10.0);\n"
        "const vec3 bbox_max = vec3(+10.0,+10.0,+10.0);\n";
    } else {
        out
        << "const vec3 bbox_min = vec3("
            << dfmt(bbox.min.x, dfmt::EXPR) << ","
            << dfmt(bbox.min.y, dfmt::EXPR) << ","
            << dfmt(bbox.min.z, dfmt::EXPR)
            << ");\n"
        << "const vec3 bbox_max = vec3("
            << dfmt(bbox.max.x, dfmt::EXPR) << ","
            << dfmt(bbox.max.y, dfmt::EXPR) << ","
            << dfmt(bbox.max.z, dfmt::EXPR)
            << ");\n";
    }

    // Following code is based on code fragments written by Inigo Quilez,
    // with The MIT Licence.
    //    Copyright 2013 Inigo Quilez
    out <<
       "// ray marching. ro is ray origin, rd is ray direction (unit vector).\n"
       "// result is (t,r,g,b), where\n"
       "//  * t is the distance that we marched,\n"
       "//  * r,g,b is the colour of the distance field at the point we ended up at.\n"
       "//    (-1,-1,-1) means no object was hit.\n"
       "vec4 castRay( in vec3 ro, in vec3 rd, float time )\n"
       "{\n"
       "    float tmin = 0.0;\n" // was 1.0
       "    float tmax = ray_max_depth;\n"
       "   \n"
       // TODO: implement bounding volume. If I remove the 'if(t>tmax)break'
       // check, then `tetrahedron` breaks. The hard coded tmax=200 fails for
       // some models.
       //"#if 0\n"
       //"    // bounding volume\n"
       //"    float tp1 = (0.0-ro.y)/rd.y; if( tp1>0.0 ) tmax = min( tmax, tp1 );\n"
       //"    float tp2 = (1.6-ro.y)/rd.y; if( tp2>0.0 ) { if( ro.y>1.6 ) tmin = max( tmin, tp2 );\n"
       //"                                                 else           tmax = min( tmax, tp2 ); }\n"
       //"#endif\n"
       //"    \n"
       "    float t = tmin;\n"
       "    vec3 c = vec3(-1.0,-1.0,-1.0);\n"
       "    for (int i=0; i<ray_max_iter; i++) {\n"
       "        float precis = 0.0005*t;\n"
       "        vec4 p = vec4(ro+rd*t,time);\n"
       "        float d = dist(p);\n"
       "        if (abs(d) < abs(precis)) {\n"
       "            c = colour(p);\n"
       "            break;\n"
       "        }\n"
       "        t += d;\n"
       "        if (t > tmax) break;\n"
       "    }\n"
       "    return vec4( t, c );\n"
       "}\n"

       "vec3 calcNormal( in vec3 pos, float time )\n"
       "{\n"
       "    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;\n"
       "    return normalize( e.xyy*dist( vec4(pos + e.xyy,time) ) + \n"
       "                      e.yyx*dist( vec4(pos + e.yyx,time) ) + \n"
       "                      e.yxy*dist( vec4(pos + e.yxy,time) ) + \n"
       "                      e.xxx*dist( vec4(pos + e.xxx,time) ) );\n"
       //"    /*\n"
       //"    vec3 eps = vec3( 0.0005, 0.0, 0.0 );\n"
       //"    vec3 nor = vec3(\n"
       //"        dist(pos+eps.xyy) - dist(pos-eps.xyy),\n"
       //"        dist(pos+eps.yxy) - dist(pos-eps.yxy),\n"
       //"        dist(pos+eps.yyx) - dist(pos-eps.yyx) );\n"
       //"    return normalize(nor);\n"
       //"    */\n"
       "}\n";

    if (opts.shader_ == Render_Opts::Shader::standard) {
       out <<
       // Compute an ambient occlusion factor.
       // pos: point on surface
       // nor: normal of the surface at pos
       // Yields a value clamped to [0,1] where 0 means no other surfaces
       // around the point, and 1 means the point is occluded by other surfaces.
       "float calcAO( in vec3 pos, in vec3 nor, float time )\n"
       "{\n"
       "    float occ = 0.0;\n"
       "    float sca = 1.0;\n"
       "    for( int i=0; i<5; i++ )\n"
       "    {\n"
       "        float hr = 0.01 + 0.12*float(i)/4.0;\n"
       "        vec3 aopos =  nor * hr + pos;\n"
       "        float dd = dist( vec4(aopos,time) );\n"
       "        occ += -(dd-hr)*sca;\n"
       "        sca *= 0.95;\n"
       "    }\n"
       "    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    \n"
       "}\n"

       "// in ro: ray origin\n"
       "// in rd: ray direction\n"
       "// out: rgb colour\n"
       "vec3 render( in vec3 ro, in vec3 rd, float time )\n"
       "{ \n"
       "    //vec3 col = vec3(0.7, 0.9, 1.0) +rd.z*0.8;\n"
       "    vec3 col = background_colour;\n"
       "    vec4 res = castRay(ro,rd, time);\n"
       "    float t = res.x;\n"
       "    vec3 c = res.yzw;\n"
       "    if( c.x>=0.0 )\n"
       "    {\n"
       "        vec3 pos = ro + t*rd;\n"
       "        vec3 nor = calcNormal( pos, time );\n"
       "        vec3 ref = reflect( rd, nor );\n"
       "        \n"
       "        // material        \n"
       "        col = c;\n"
       "\n"
       "        // lighting        \n"
       "        float occ = calcAO( pos, nor, time );\n"
       "        vec3  lig = normalize( vec3(-0.4, 0.6, 0.7) );\n"
       "        float amb = clamp( 0.5+0.5*nor.z, 0.0, 1.0 );\n"
       "        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );\n"
       "        float bac = clamp( dot( nor, normalize(vec3(-lig.x,lig.y,0.0))), 0.0, 1.0 )*clamp( 1.0-pos.z,0.0,1.0);\n"
       "        float dom = smoothstep( -0.1, 0.1, ref.z );\n"
       "        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );\n"
       "        float spe = pow(clamp( dot( ref, lig ), 0.0, 1.0 ),16.0);\n"
       "        \n"
       "        vec3 lin = vec3(0.0);\n"
       "        lin += 1.30*dif*vec3(1.00,0.80,0.55);\n"
       "        lin += 2.00*spe*vec3(1.00,0.90,0.70)*dif;\n"
       "        lin += 0.40*amb*vec3(0.40,0.60,1.00)*occ;\n"
       "        lin += 0.50*dom*vec3(0.40,0.60,1.00)*occ;\n"
       "        lin += 0.50*bac*vec3(0.35,0.35,0.35)*occ;\n"
       "        lin += 0.25*fre*vec3(1.00,1.00,1.00)*occ;\n"
       "        vec3 iqcol = col*lin;\n"
       "\n"
       "        //col = mix( col, vec3(0.8,0.9,1.0), 1.0-exp( -0.0002*t*t*t ) );\n"
       "        col = mix(col,iqcol, 0.5);\n" // adjust contrast
       "    }\n"
       "\n"
       "    return vec3( clamp(col,0.0,1.0) );\n"
       "}\n";
    }

    if (opts.shader_ == Render_Opts::Shader::sf1) {
       if (opts.sf1_) {
           export_sf1(shape, opts, out);
       } else {
       out <<
       // Only called if the ray struck a shape, returns pixel colour.
       "vec3 sf1(vec3 pos, vec3 nor, vec3 rd, vec3 col)\n"
       "{\n"
       "    vec3 ref = reflect( rd, nor );\n"
       "    \n"
       "    // lighting        \n"
       "    float occ = 1.0;\n"
       "    vec3  lig = normalize( vec3(-0.4, 0.6, 0.7) );\n"
       "    float amb = clamp( 0.5+0.5*nor.z, 0.0, 1.0 );\n"
       "    float dif = clamp( dot( nor, lig ), 0.0, 1.0 );\n"
       "    float bac = clamp( dot( nor, normalize(vec3(-lig.x,lig.y,0.0))), 0.0, 1.0 )*clamp( 1.0-pos.z,0.0,1.0);\n"
       "    float dom = smoothstep( -0.1, 0.1, ref.z );\n"
       "    float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );\n"
       "    float spe = pow(clamp( dot( ref, lig ), 0.0, 1.0 ),16.0);\n"
       "    \n"
       "    vec3 lin = vec3(0.0);\n"
       "    lin += 1.30*dif*vec3(1.00,0.80,0.55);\n"
       "    lin += 2.00*spe*vec3(1.00,0.90,0.70)*dif;\n"
       "    lin += 0.40*amb*vec3(0.40,0.60,1.00)*occ;\n"
       "    lin += 0.50*dom*vec3(0.40,0.60,1.00)*occ;\n"
       "    lin += 0.50*bac*vec3(0.35,0.35,0.35)*occ;\n"
       "    lin += 0.25*fre*vec3(1.00,1.00,1.00)*occ;\n"
       "    vec3 iqcol = col*lin;\n"
       "\n"
       "    return mix(col,iqcol, 0.5);\n" // adjust contrast
       "}\n";
       }
       out <<
       "// in ro: ray origin\n"
       "// in rd: ray direction\n"
       "// out: rgb colour\n"
       "vec3 render( in vec3 ro, in vec3 rd, float time )\n"
       "{ \n"
       "    //vec3 col = vec3(0.7, 0.9, 1.0) +rd.z*0.8;\n"
       "    vec3 col = background_colour;\n"
       "    vec4 res = castRay(ro,rd, time);\n"
       "    float t = res.x;\n"
       "    vec3 c = res.yzw;\n"
       "    if( c.x>=0.0 )\n"
       "    {\n"
       "        vec3 pos = ro + t*rd;\n"
       "        vec3 nor = calcNormal( pos, time );\n"
       "        col = sf1(pos, nor, rd, c);\n"
       "    }\n"
       "\n"
       "    return vec3( clamp(col,0.0,1.0) );\n"
       "}\n";
    }

    if (opts.shader_ == Render_Opts::Shader::pew) {
       // written by Philipp Emanuel Weidmann (@p-e-w on github)
       out <<
       "struct Light {\n"
       "    vec3 position;\n"
       "    // One component per color channel\n"
       "    vec3 specular_intensity;\n"
       "    vec3 diffuse_intensity;\n"
       "    vec3 ambient_intensity;\n"
       "};\n"

       "const Light lights[3] = Light[3](\n"
       "    Light(vec3(-10.0, -100.0,  100.0), vec3(1.5), vec3(1.5), vec3(0.25)),\n"
       "    Light(vec3(  0.0,  100.0,  100.0), vec3(2.0), vec3(2.0), vec3(0.25)),\n"
       "    Light(vec3( 20.0,  100.0, -100.0), vec3(1.5), vec3(1.5), vec3(0.5))\n"
       ");\n"

       "struct Material {\n"
       "    // One component per color channel\n"
       "    vec3 specular_reflectivity;\n"
       "    vec3 diffuse_reflectivity;\n"
       "    vec3 ambient_reflectivity;\n"
       "    vec3 shininess;\n"
       "};\n"

       "Material material(vec3 point, float time) {\n"
       "    return Material(vec3(1.5), vec3(1.2), vec3(0.5), vec3(15.0));\n"
       "}\n"

       "vec3 render(in vec3 ray_origin, in vec3 ray_direction, float time) {\n"
       "    vec4 result = castRay(ray_origin, ray_direction, time);\n"
       "    if (result.y < 0.0) {\n"
       "        return background_colour;\n"
       "    }\n"
       "\n"
       "    float distance = result.x;\n"
       "    vec3 point = ray_origin + distance*ray_direction;\n"
       "    vec3 normal = calcNormal(point, time);\n"
       "    vec3 viewer_direction = -ray_direction;\n"
       "\n"
       "    vec3 color = result.yzw;\n"
       "    Material material = material(point, time);\n"
       "\n"
       "    vec3 illumination = vec3(0.0);\n"
       "\n"
       "    // Implementation follows https://en.wikipedia.org/wiki/Phong_reflection_model\n"
       "    for (int i = 0; i < lights.length(); i++) {\n"
       "        Light light = lights[i];\n"
       "\n"
       "        illumination += material.ambient_reflectivity * light.ambient_intensity;\n"
       "\n"
       "        vec3 light_direction = normalize(light.position - point);\n"
       "\n"
       "        result = castRay(point, light_direction, time);\n"
       "        if (result.y < 0.0) {\n"
       "            // No part of the shape lies between the surface point\n"
       "            // and the light source, so directional light affects this point\n"
       "            vec3 reflection_direction = -reflect(light_direction, normal);\n"
       "\n"
       "            float diffuse_term = dot(light_direction, normal);\n"
       "            if (diffuse_term > 0.0) {\n"
       "                illumination +=\n"
       "                    material.diffuse_reflectivity * diffuse_term * light.diffuse_intensity;\n"
       "\n"
       "                float specular_term = dot(reflection_direction, viewer_direction);\n"
       "                if (specular_term > 0.0) {\n"
       "                    illumination +=\n"
       "                        material.specular_reflectivity *\n"
       "                        pow(vec3(specular_term), material.shininess) *\n"
       "                        light.specular_intensity;\n"
       "                }\n"
       "            }\n"
       "        }\n"
       "    }\n"
       "\n"
       "    return mix(color, clamp(color*illumination, 0.0, 1.0), 0.5);\n"
       "}\n";
    }

    out <<
       "// Create a matrix to transform coordinates to look towards a given point.\n"
       "// * `eye` is the position of the camera.\n"
       "// * `centre` is the position to look towards.\n"
       "// * `up` is the 'up' direction.\n"
       "mat3 look_at(vec3 eye, vec3 centre, vec3 up)\n"
       "{\n"
       "    vec3 ww = normalize(centre - eye);\n"
       "    vec3 uu = normalize(cross(ww, up));\n"
       "    vec3 vv = normalize(cross(uu, ww));\n"
       "    return mat3(uu, vv, ww);\n"
       "}\n"

       "// Generate a ray direction for ray-casting.\n"
       "// * `camera` is the camera look-at matrix.\n"
       "// * `pos` is the screen position, normally in the range -1..1\n"
       "// * `lens` is the lens length of the camera (encodes field-of-view).\n"
       "//   0 is very wide, and 2 is a good default.\n"
       "vec3 ray_direction(mat3 camera, vec2 pos, float lens)\n"
       "{\n"
       "    return normalize(camera * vec3(pos, lens));\n"
       "}\n"

       "void mainImage( out vec4 fragColour, in vec2 fragCoord )\n"
       "{\n"
       "    vec3 col = vec3(0.0);\n"
       "    const vec3 origin = (bbox_min + bbox_max) / 2.0;\n"
       "    const vec3 radius = (bbox_max - bbox_min) / 2.0;\n"
       "    float r = max(radius.x, max(radius.y, radius.z)) / 1.3;\n"
       "#if AA>1\n"
       "  for (int m=0; m<AA; ++m)\n"
       "  for (int n=0; n<AA; ++n) {\n"
       "    vec2 o = vec2(float(m),float(n)) / float(AA) - 0.5;\n"
       "#else\n"
       "    const vec2 o = vec2(0.0);\n"
       "#endif\n"
       "    vec2 p = -1.0 + 2.0 * (fragCoord+o) / iResolution.xy;\n"
       "    p.x *= iResolution.x/iResolution.y;\n"
       "\n"
       // convert from the OpenGL coordinate system to the Curv coord system.
       "    vec3 eye = vec3(u_eye3d.x, -u_eye3d.z, u_eye3d.y)*r + origin;\n"
       "    vec3 centre = vec3(u_centre3d.x, -u_centre3d.z, u_centre3d.y)*r + origin;\n"
       "    vec3 up = vec3(u_up3d.x, -u_up3d.z, u_up3d.y);\n"
       "    mat3 camera = look_at(eye, centre, up);\n"
       "    vec3 dir = ray_direction(camera, p, 2.5);\n"
       "\n"
       "#if TAA>1\n"
       "  for (int t=0; t<TAA; ++t) {\n"
       "    float time = iTime + float(t)/float(TAA)*float(FDUR);\n"
       "#else\n"
       "    float time = iTime;\n"
       "#endif\n"
       "    col += render( eye, dir, time );\n"
       "\n"
       "#if TAA>1\n"
       "  }\n"
       "#endif\n"
       "#if AA>1\n"
       "  }\n"
       "#endif\n"
       "#if AA>1 || TAA>1\n"
       "    col /= float(AA*AA*TAA);\n"
       "#endif\n"
       "\n"
       "    // convert linear RGB to sRGB\n"
       "    col = pow(col, vec3(0.454545454545454545));\n"
       "    fragColour = vec4(col,1.0);\n"
       "}\n"
       ;
}

} // namespaces
