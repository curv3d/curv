// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <aux/dtostr.h>
#include <curv/gl_compiler.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/gl_context.h>
#include <curv/shape.h>
#include <curv/meaning.h>
#include <curv/function.h>

using aux::dfmt;
namespace curv {

void gl_compile_2d(const Shape2D&, std::ostream&, const Context&);
void gl_compile_3d(const Shape2D&, std::ostream&, const Context& );

void gl_compile(const Shape2D& shape, std::ostream& out, const Context& cx)
{
    if (shape.hasfield("is_3d")) {
        bool is3d = shape.getfield("is_3d", cx).to_bool(At_Field("is_3d", cx));
        if (is3d)
            return gl_compile_3d(shape, out, cx);
    }
    return gl_compile_2d(shape, out, cx);
}

void gl_compile_2d(const Shape2D& shape, std::ostream& out, const Context&)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::Vec4);
    auto frame = GL_Frame::make(0, gl, nullptr, nullptr);

    out <<
        "#ifdef GLSLVIEWER\n"
        "uniform mat3 u_view2d;\n"
        "#endif\n"
        "float main_dist(vec4 " << dist_param << ", out vec4 colour)\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, *frame);

    if (shape.hasfield("colour")) {
        GL_Value colour = shape.gl_colour(dist_param, *frame);
        out << "  colour = vec4(" << colour << ", 1.0);\n";
    } else {
        out << "  colour = vec4(0.4, 0.0, 0.0, 1.0);\n";
    }

    out <<
        "  return " << result << ";\n"
        "}\n";
    BBox bbox = shape.bbox(At_GL_Frame(&*frame));
    if (bbox.empty() || bbox.infinite()) {
        out <<
        "const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);\n";
    } else {
        out << "const vec4 bbox = vec4("
            << bbox.xmin << ","
            << bbox.ymin << ","
            << bbox.xmax << ","
            << bbox.ymax
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
        "#ifdef GLSLVIEWER\n"
        "    fragCoord = (u_view2d * vec3(fragCoord,1)).xy;\n"
        "#endif\n"
        "    float d = main_dist(vec4(fragCoord*scale+offset,0,iGlobalTime), fragColour);\n"
        "    if (d > 0.0) {\n"
        "        vec2 uv = fragCoord.xy / iResolution.xy;\n"
        "        fragColour = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);\n"
        "    }\n"
        "}\n"
        ;
}

void gl_compile_3d(const Shape2D& shape, std::ostream& out, const Context& cx)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::Vec4);
    auto frame = GL_Frame::make(0, gl, nullptr, nullptr);

    out <<
        "#ifdef GLSLVIEWER\n"
        "uniform mat3 u_view2d;\n"
        "#endif\n"
        "float main_dist(vec4 " << dist_param << ", out vec4 colour)\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, *frame);

    if (shape.hasfield("colour")) {
        GL_Value colour = shape.gl_colour(dist_param, *frame);
        out << "  colour = vec4(" << colour << ", 1.0);\n";
    } else {
        out << "  colour = vec4(0.8, 0.8, 0.5, 1.0);\n";
    }

    out <<
        "  return " << result << ";\n"
        "}\n";

#if 0
    BBox bbox = shape.bbox(At_GL_Frame(&*frame));
    if (bbox.empty() || bbox.infinite()) {
        out <<
        "const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);\n";
    } else {
        out << "const vec4 bbox = vec4("
            << bbox.xmin << ","
            << bbox.ymin << ","
            << bbox.xmax << ","
            << bbox.ymax
            << ");\n";
    }
#endif

    // Following code is based on code fragments written by Inigo Quilez,
    // with The MIT Licence.
    //    Copyright 2013 Inigo Quilez
    out <<
       "vec4 map(in vec3 pos)\n"
       "{\n"
       "    vec4 colour;\n"
       "    float dist = main_dist(vec4(pos, iGlobalTime), colour);\n"
       "    return vec4(dist, colour.rgb);\n"
       "}\n"

       "// ray marching. ro is ray origin, rd is ray direction (unit vector).\n"
       "// result is (t,r,g,b), where\n"
       "//  * t is the distance that we marched,\n"
       "//  * r,g,b is the colour of the distance field at the point we ended up at.\n"
       "//    (-1,-1,-1) means no object was hit.\n"
       "vec4 castRay( in vec3 ro, in vec3 rd )\n"
       "{\n"
       "    float tmin = 1.0;\n"
       "    float tmax = 20.0;\n"
       "   \n"
       "#if 0\n"
       "    // bounding volume\n"
       "    float tp1 = (0.0-ro.y)/rd.y; if( tp1>0.0 ) tmax = min( tmax, tp1 );\n"
       "    float tp2 = (1.6-ro.y)/rd.y; if( tp2>0.0 ) { if( ro.y>1.6 ) tmin = max( tmin, tp2 );\n"
       "                                                 else           tmax = min( tmax, tp2 ); }\n"
       "#endif\n"
       "    \n"
       "    float t = tmin;\n"
       "    vec3 c = vec3(-1.0,-1.0,-1.0);\n"
       "    for( int i=0; i<64; i++ )\n"
       "    {\n"
       "        float precis = 0.0005*t;\n"
       "        vec4 res = map( ro+rd*t );\n"
       "        if( res.x<precis || t>tmax ) break;\n"
       "        t += res.x;\n"
       "        c = res.yzw;\n"
       "    }\n"
       "\n"
       "    if( t>tmax ) c=vec3(-1.0,-1.0,-1.0);\n"
       "    return vec4( t, c );\n"
       "}\n"

       "vec3 calcNormal( in vec3 pos )\n"
       "{\n"
       "    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;\n"
       "    return normalize( e.xyy*map( pos + e.xyy ).x + \n"
       "                      e.yyx*map( pos + e.yyx ).x + \n"
       "                      e.yxy*map( pos + e.yxy ).x + \n"
       "                      e.xxx*map( pos + e.xxx ).x );\n"
       "    /*\n"
       "    vec3 eps = vec3( 0.0005, 0.0, 0.0 );\n"
       "    vec3 nor = vec3(\n"
       "        map(pos+eps.xyy).x - map(pos-eps.xyy).x,\n"
       "        map(pos+eps.yxy).x - map(pos-eps.yxy).x,\n"
       "        map(pos+eps.yyx).x - map(pos-eps.yyx).x );\n"
       "    return normalize(nor);\n"
       "    */\n"
       "}\n"

       "float calcAO( in vec3 pos, in vec3 nor )\n"
       "{\n"
       "    float occ = 0.0;\n"
       "    float sca = 1.0;\n"
       "    for( int i=0; i<5; i++ )\n"
       "    {\n"
       "        float hr = 0.01 + 0.12*float(i)/4.0;\n"
       "        vec3 aopos =  nor * hr + pos;\n"
       "        float dd = map( aopos ).x;\n"
       "        occ += -(dd-hr)*sca;\n"
       "        sca *= 0.95;\n"
       "    }\n"
       "    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 );    \n"
       "}\n"

       "// in ro: ray origin\n"
       "// in rd: ray direction\n"
       "// out: rgb colour\n"
       "vec3 render( in vec3 ro, in vec3 rd )\n"
       "{ \n"
       "    vec3 col = vec3(0.7, 0.9, 1.0) +rd.y*0.8;\n"
       "    vec4 res = castRay(ro,rd);\n"
       "    float t = res.x;\n"
       "    vec3 c = res.yzw;\n"
       "    if( c.x>=0.0 )\n"
       "    {\n"
       "        vec3 pos = ro + t*rd;\n"
       "        vec3 nor = calcNormal( pos );\n"
       "        vec3 ref = reflect( rd, nor );\n"
       "        \n"
       "        // material        \n"
       "        col = c;\n"
       "\n"
       "        // lighting        \n"
       "        float occ = calcAO( pos, nor );\n"
       "        vec3  lig = normalize( vec3(-0.4, 0.7, -0.6) );\n"
       "        float amb = clamp( 0.5+0.5*nor.y, 0.0, 1.0 );\n"
       "        float dif = clamp( dot( nor, lig ), 0.0, 1.0 );\n"
       "        float bac = clamp( dot( nor, normalize(vec3(-lig.x,0.0,-lig.z))), 0.0, 1.0 )*clamp( 1.0-pos.y,0.0,1.0);\n"
       "        float dom = smoothstep( -0.1, 0.1, ref.y );\n"
       "        float fre = pow( clamp(1.0+dot(nor,rd),0.0,1.0), 2.0 );\n"
       "        float spe = pow(clamp( dot( ref, lig ), 0.0, 1.0 ),16.0);\n"
       "        \n"
       "        vec3 lin = vec3(0.0);\n"
       "        lin += 1.30*dif*vec3(1.00,0.80,0.55);\n"
       "        lin += 2.00*spe*vec3(1.00,0.90,0.70)*dif;\n"
       "        lin += 0.40*amb*vec3(0.40,0.60,1.00)*occ;\n"
       "        lin += 0.50*dom*vec3(0.40,0.60,1.00)*occ;\n"
       "        lin += 0.50*bac*vec3(0.25,0.25,0.25)*occ;\n"
       "        lin += 0.25*fre*vec3(1.00,1.00,1.00)*occ;\n"
       "        col = col*lin;\n"
       "\n"
       "        col = mix( col, vec3(0.8,0.9,1.0), 1.0-exp( -0.0002*t*t*t ) );\n"
       "    }\n"
       "\n"
       "    return vec3( clamp(col,0.0,1.0) );\n"
       "}\n"

       "// Create a matrix to transform coordinates to look towards a given point.\n"
       "// * `origin` is the position of the camera.\n"
       "// * `target` is the position to look towards.\n"
       "// * `roll` is the rotation of the camera.\n"
       "mat3 look_at(vec3 origin, vec3 target, float roll)\n"
       "{\n"
       "    vec3 rr = vec3(sin(roll), cos(roll), 0.0);\n"
       "    vec3 ww = normalize(target - origin);\n"
       "    vec3 uu = normalize(cross(ww, rr));\n"
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

       "void mainImage( out vec4 fragColor, in vec2 fragCoord )\n"
       "{\n"
       "    vec2 p = -1.0 + 2.0 * fragCoord.xy / iResolution.xy;\n"
       "    p.x *= iResolution.x/iResolution.y;\n"
       "\n"
       "    float ctime = iGlobalTime;\n"
       "    // camera\n"
       "    vec3 ro = 1.1*vec3(2.5*sin(0.25*ctime),1.0+1.0*cos(ctime*.13),2.5*cos(0.25*ctime));\n"
       "    //vec3 ro = 1.1*vec3(0.0. 2.0, 2.5); // as above at t=0\n"
       "    mat3 camera = look_at(ro, vec3(0.0), 0.0);\n"
       "    vec3 rd = ray_direction(camera, p, 2.5);\n"
       "\n"
       "    vec3 col = render( ro, rd );\n"
       "    \n"
       "    fragColor = vec4(col,1.0);\n"
       "}\n"
       ;

#if 0
    // This is the 2D version of the renderer. Try lifting the u_view2d code.
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
        "#ifdef GLSLVIEWER\n"
        "    fragCoord = (u_view2d * vec3(fragCoord,1)).xy;\n"
        "#endif\n"
        "    float d = main_dist(vec4(fragCoord*scale+offset,0,iGlobalTime), fragColour);\n"
        "    if (d > 0.0) {\n"
        "        vec2 uv = fragCoord.xy / iResolution.xy;\n"
        "        fragColour = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);\n"
        "    }\n"
        "}\n"
        ;
#endif
}

GL_Type_Attr gl_types[] =
{
    {"bool", 1},
    {"float", 1},
    {"vec2", 2},
    {"vec3", 3},
    {"vec4", 4},
};

std::ostream& operator<<(std::ostream& out, GL_Type type)
{
    return out << gl_type_name(type);
}

GL_Value gl_call_unary_numeric(GL_Frame& f, const char* name)
{
    auto arg = f[0];
    if (!gl_type_numeric(arg.type))
        throw Exception(At_GL_Arg(0, f),
            stringify(name,": argument is not numeric"));
    auto result = f.gl.newvalue(arg.type);
    f.gl.out<<"  "<<arg.type<<" "<<result<<" = "<<name<<"("<<arg<<");\n";
    return result;
}

GL_Value gl_eval_expr(GL_Frame& f, const Operation& op, GL_Type type)
{
    GL_Value arg = op.gl_eval(f);
    if (arg.type != type) {
        throw Exception(At_GL_Phrase(*op.source_, &f),
            stringify("argument is not a ",type));
    }
    return arg;
}

GL_Value gl_eval_const(GL_Frame& f, Value val, const Phrase& source)
{
    if (val.is_num()) {
        GL_Value result = f.gl.newvalue(GL_Type::Num);
        double num = val.get_num_unsafe();
        f.gl.out << "  float " << result << " = "
            << dfmt(num, dfmt::EXPR) << ";\n";
        return result;
    }
    if (val.is_bool()) {
        GL_Value result = f.gl.newvalue(GL_Type::Bool);
        bool b = val.get_bool_unsafe();
        f.gl.out << "  bool " << result << " = "
            << (b ? "true" : "false") << ";\n";
        return result;
    }
    if (auto list = val.dycast<List>()) {
        if (list->size() >= 2 && list->size() <= 4) {
            static GL_Type types[5] = {
                {}, {}, GL_Type::Vec2, GL_Type::Vec3, GL_Type::Vec4
            };
            GL_Value result = f.gl.newvalue(types[list->size()]);
            f.gl.out
                << "  "
                << result.type
                << " "
                << result
                << " = "
                << result.type
                << "(";
            bool first = true;
            for (auto e : *list) {
                if (e.is_num()) {
                    if (!first) f.gl.out << ",";
                    first = false;
                    f.gl.out << e.get_num_unsafe();
                } else
                    goto error;
            }
            f.gl.out << ");\n";
            return result;
        }
    }
error:
    throw Exception(At_GL_Phrase(source, &f),
        stringify("value ",val," is not supported by the Geometry Compiler"));
}

GL_Value Operation::gl_eval(GL_Frame& f) const
{
    throw Exception(At_GL_Phrase(*source_, &f),
        "this expression is not supported by the Geometry Compiler");
}

void Operation::gl_exec(GL_Frame& f) const
{
    throw Exception(At_GL_Phrase(*source_, &f),
        "this action is not supported by the Geometry Compiler");
}

GL_Value Constant::gl_eval(GL_Frame& f) const
{
    return gl_eval_const(f, value_, *source_);
}

GL_Value Negative_Expr::gl_eval(GL_Frame& f) const
{
    auto x = arg_->gl_eval(f);
    if (!gl_type_numeric(x.type))
        throw Exception(At_GL_Phrase(*arg_->source_, &f),
            "argument not numeric");
    GL_Value result = f.gl.newvalue(x.type);
    f.gl.out<<"  "<<x.type<<" "<<result<<" = -"<<x<< ";\n";
    return result;
}

void gl_put_as(GL_Frame& f, GL_Value val, const Context& cx, GL_Type type)
{
    if (val.type == type) {
        f.gl.out << val;
        return;
    }
    if (val.type == GL_Type::Num) {
        if (gl_type_count(type) > 1) {
            f.gl.out << type << "(";
            bool first = true;
            for (int i = 0; i < gl_type_count(type); ++i) {
                if (!first) f.gl.out << ",";
                f.gl.out << val;
                first = false;
            }
            f.gl.out << ")";
            return;
        }
    }
    throw Exception(cx, stringify("GL can't convert ",val.type," to ",type));
}

GL_Value
gl_arith_expr(GL_Frame& f, const Phrase& source,
    const Operation& xexpr, const char* op, const Operation& yexpr)
{
    auto x = xexpr.gl_eval(f);
    auto y = yexpr.gl_eval(f);

    GL_Type rtype = GL_Type::Bool;
    if (x.type == y.type)
        rtype = x.type;
    else if (x.type == GL_Type::Num)
        rtype = y.type;
    else if (y.type == GL_Type::Num)
        rtype = x.type;
    if (rtype == GL_Type::Bool)
        throw Exception(At_GL_Phrase(source, &f), "GL domain error");

    GL_Value result = f.gl.newvalue(rtype);
    f.gl.out <<"  "<<rtype<<" "<<result<<" = ";
    gl_put_as(f, x, At_GL_Phrase(*xexpr.source_, &f), rtype);
    f.gl.out << op;
    gl_put_as(f, y, At_GL_Phrase(*yexpr.source_, &f), rtype);
    f.gl.out << ";\n";
    return result;
}

GL_Value Add_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *source_, *arg1_, "+", *arg2_);
}

GL_Value Subtract_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *source_, *arg1_, "-", *arg2_);
}

GL_Value Multiply_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *source_, *arg1_, "*", *arg2_);
}

GL_Value Divide_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *source_, *arg1_, "/", *arg2_);
}

// Evaluate an expression to a constant at GL compile time,
// or abort if it isn't a constant.
Value gl_constify(Operation& op, GL_Frame& f)
{
    if (auto c = dynamic_cast<Constant*>(&op))
        return c->value_;
    else if (auto dot = dynamic_cast<Dot_Expr*>(&op)) {
        if (auto ref = cast<Nonlocal_Strict_Ref>(dot->base_)) {
            Value base = (*f.nonlocal)[ref->slot_];
            return base.at(dot->id_, At_GL_Phrase(*op.source_, &f));
        }
    }
    else if (auto ref = dynamic_cast<Nonlocal_Strict_Ref*>(&op))
        return (*f.nonlocal)[ref->slot_];
    else if (auto ref = dynamic_cast<Nonlocal_Lazy_Ref*>(&op))
        return (*f.nonlocal)[ref->slot_];
    else if (auto fref = dynamic_cast<Nonlocal_Function_Ref*>(&op)) {
        return {make<Closure>(
            (Lambda&) (*f.nonlocal)[fref->lambda_slot_].get_ref_unsafe(),
            *f.nonlocal)};
    }
    else if (auto list = dynamic_cast<List_Expr*>(&op)) {
        Shared<List> listval = List::make(list->size());
        for (size_t i = 0; i < list->size(); ++i) {
            (*listval)[i] = gl_constify(*(*list)[i], f);
        }
        return {listval};
    }
    throw Exception(At_GL_Phrase(*op.source_, &f),
        "Geometry Compiler: not a constant");
}

GL_Value Call_Expr::gl_eval(GL_Frame& f) const
{
    auto val = gl_constify(*fun_, f);
    if (auto fun = val.dycast<Function>()) {
        return fun->gl_call_expr(*arg_, call_phrase(), f);
    }
    throw Exception(At_GL_Phrase(*fun_->source_, &f),
        stringify("Geometry Compiler: ",val," is not a function"));
}

GL_Value Block_Op::gl_eval(GL_Frame& f) const
{
    statements_.gl_exec(f);
    return body_->gl_eval(f);
}
void Block_Op::gl_exec(GL_Frame& f) const
{
    statements_.gl_exec(f);
    body_->gl_exec(f);
}

void Statements::gl_exec(GL_Frame& f) const
{
    for (auto action : actions_)
        action->gl_exec(f);
}

void
Let_Assign::gl_exec(GL_Frame& f) const
{
    GL_Value val = expr_->gl_eval(f);
    if (reassign_)
        f.gl.out << "  "<<f[slot_]<<"="<<val<<";\n";
    else {
        GL_Value var = f.gl.newvalue(val.type);
        f.gl.out << "  "<<var.type<<" "<<var<<"="<<val<<";\n";
        f[slot_] = var;
    }
}

char gl_index_letter(Value k, unsigned vecsize, const Context& cx)
{
    auto num = k.get_num_or_nan();
    if (num == 0.0)
        return 'x';
    if (num == 1.0)
        return 'y';
    if (num == 2.0 && vecsize > 2)
        return 'z';
    if (num == 3.0 && vecsize > 3)
        return 'w';
    throw Exception(cx,
        stringify("Geometry Compiler: got ",k,", expected 0..",vecsize-1));
}

GL_Value At_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = arg1_->gl_eval(f);
    if (gl_type_count(arg1.type) < 2)
        throw Exception(At_GL_Phrase(*arg1_->source_, &f), "not a vector");

    const char* arg2 = nullptr;
    auto k = gl_constify(*arg2_, f);
    if (auto list = k.dycast<List>()) {
        if (list->size() < 2 || list->size() > 4) {
            throw Exception(At_GL_Phrase(*arg2_->source_, &f),
                "list index vector must have between 2 and 4 elements");
        }
        char swizzle[5];
        memset(swizzle, 0, 5);
        for (size_t i = 0; i <list->size(); ++i) {
            swizzle[i] = gl_index_letter((*list)[i], gl_type_count(arg1.type),
                At_Index(i, At_GL_Phrase(*arg2_->source_, &f)));
        }
        GL_Value result = f.gl.newvalue(gl_vec_type(list->size()));
        f.gl.out << "  " << result.type << " "
            << result<<" = "<<arg1<<"."<<swizzle<<";\n";
        return result;
    }
    auto num = k.get_num_or_nan();
    if (num == 0.0)
        arg2 = ".x";
    else if (num == 1.0)
        arg2 = ".y";
    else if (num == 2.0 && gl_type_count(arg1.type) > 2)
        arg2 = ".z";
    else if (num == 3.0 && gl_type_count(arg1.type) > 3)
        arg2 = ".w";
    if (arg2 == nullptr)
        throw Exception(At_GL_Phrase(*arg2_->source_, &f),
            stringify("Geometry Compiler: got ",k,", expected 0..",
                gl_type_count(arg1.type)-1));

    GL_Value result = f.gl.newvalue(GL_Type::Num);
    f.gl.out << "  float "<<result<<" = "<<arg1<<arg2<<";\n";
    return result;
}

GL_Value Arg_Ref::gl_eval(GL_Frame& f) const
{
    return f[slot_];
}
GL_Value Let_Ref::gl_eval(GL_Frame& f) const
{
    return f[slot_];
}

GL_Value Nonlocal_Strict_Ref::gl_eval(GL_Frame& f) const
{
    return gl_eval_const(f, (*f.nonlocal)[slot_], *source_);
}

GL_Value List_Expr_Base::gl_eval(GL_Frame& f) const
{
    if (this->size() == 2) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num);
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num);
        GL_Value result = f.gl.newvalue(GL_Type::Vec2);
        f.gl.out << "  vec2 "<<result<<" = vec2("<<e1<<","<<e2<<");\n";
        return result;
    }
    if (this->size() == 3) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num);
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num);
        auto e3 = gl_eval_expr(f, *(*this)[2], GL_Type::Num);
        GL_Value result = f.gl.newvalue(GL_Type::Vec3);
        f.gl.out << "  vec3 "<<result<<" = vec3("
            <<e1<<","<<e2<<","<<e3<<");\n";
        return result;
    }
    if (this->size() == 4) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num);
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num);
        auto e3 = gl_eval_expr(f, *(*this)[2], GL_Type::Num);
        auto e4 = gl_eval_expr(f, *(*this)[3], GL_Type::Num);
        GL_Value result = f.gl.newvalue(GL_Type::Vec3);
        f.gl.out << "  vec4 "<<result<<" = vec4("
            <<e1<<","<<e2<<","<<e3<<","<<e4<<");\n";
        return result;
    }
    throw Exception(At_GL_Phrase(*source_, &f),
        "this list constructor does not support the Geometry Compiler");
}

GL_Value Not_Expr::gl_eval(GL_Frame& f) const
{
    auto arg = gl_eval_expr(f, *arg_, GL_Type::Bool);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" = !"<<arg<<";\n";
    return result;
}
GL_Value Or_Expr::gl_eval(GL_Frame& f) const
{
    // TODO: change GL Or to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Bool);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" || "<<arg2<<");\n";
    return result;
}
GL_Value And_Expr::gl_eval(GL_Frame& f) const
{
    // TODO: change GL And to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Bool);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" && "<<arg2<<");\n";
    return result;
}
GL_Value If_Else_Op::gl_eval(GL_Frame& f) const
{
    // TODO: change GL If to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool);
    auto arg2 = arg2_->gl_eval(f);
    auto arg3 = arg3_->gl_eval(f);
    if (arg2.type != arg3.type) {
        throw Exception(At_GL_Phrase(*source_, &f),
            "Geometry Compiler: if: type mismatch in 'then' and 'else' arms");
    }
    GL_Value result = f.gl.newvalue(arg2.type);
    f.gl.out <<"  "<<arg2.type<<" "<<result
             <<" =("<<arg1<<" ? "<<arg2<<" : "<<arg3<<");\n";
    return result;
}
void If_Else_Op::gl_exec(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool);
    f.gl.out << "  if ("<<arg1<<") {\n";
    arg2_->gl_exec(f);
    f.gl.out << "  } else {\n";
    arg3_->gl_exec(f);
    f.gl.out << "  }\n";
}
void If_Op::gl_exec(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool);
    f.gl.out << "  if ("<<arg1<<") {\n";
    arg2_->gl_exec(f);
    f.gl.out << "  }\n";
}
void While_Action::gl_exec(GL_Frame& f) const
{
    f.gl.out << "  for (;;) {\n";
    auto cond = gl_eval_expr(f, *cond_, GL_Type::Bool);
    f.gl.out << "  if (!"<<cond<<") break;\n";
    body_->gl_exec(f);
    f.gl.out << "  }\n";
}
GL_Value Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" == "<<arg2<<");\n";
    return result;
}
GL_Value Not_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" != "<<arg2<<");\n";
    return result;
}
GL_Value Less_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" < "<<arg2<<");\n";
    return result;
}
GL_Value Greater_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" > "<<arg2<<");\n";
    return result;
}
GL_Value Less_Or_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" <= "<<arg2<<");\n";
    return result;
}
GL_Value Greater_Or_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Bool);
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" >= "<<arg2<<");\n";
    return result;
}

Shared<List_Expr>
list_to_list_expr(const Phrase& source, const List& list)
{
    Shared<List_Expr> listx = List_Expr::make(list.size(), share(source));
    for (size_t i = 0; i < list.size(); ++i)
        listx->at(i) = make<Constant>(share(source), list.at(i));
    return listx;
}

Shared<List_Expr>
gl_expr_to_list(Operation& expr, int len, GL_Frame& f)
{
    if (auto list = cast<List_Expr>(share(expr))) {
        if (len < 0 || list->size() == (size_t)len)
            return list;
    }
    if (auto k = cast<Constant>(share(expr))) {
        if (auto list = k->value_.dycast<List>()) {
            if (len < 0 || list->size() == (size_t)len)
                return list_to_list_expr(*expr.source_, *list);
        }
    }
    throw Exception(At_GL_Phrase(*expr.source_, &f),
        stringify("GL: not a list of size ", len));
}

Shared<Lambda_Expr>
gl_expr_to_function(Operation& expr, GL_Frame& f)
{
    if (auto lambda = cast<Lambda_Expr>(share(expr))) {
        return lambda;
    }
    if (auto k = cast<Constant>(share(expr))) {
        if (auto lambda = k->value_.dycast<Closure>()) {
            return make<Lambda_Expr>(expr.source_,
                lambda->expr_,
                list_to_list_expr(*expr.source_, *lambda->nonlocal_),
                lambda->nargs_,
                lambda->nslots_);
        }
    }
    throw Exception(At_GL_Phrase(*expr.source_, &f),
        "GL: not a function");
}

} // namespace curv
