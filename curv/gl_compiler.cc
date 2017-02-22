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

void gl_compile(const Shape2D& shape, std::ostream& out)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::Vec2);
    auto frame = GL_Frame::make(0, gl, nullptr, nullptr);

    out <<
        "#ifdef GLSLVIEWER\n"
        "uniform mat3 u_view2d;\n"
        "#endif\n"
        "float main_dist(vec2 " << dist_param << ", out vec4 colour)\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, *frame);

    if (shape.getfield("colour") != missing) {
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
        "    float d = main_dist(fragCoord*scale+offset, fragColour);\n"
        "    if (d > 0.0) {\n"
        "        vec2 uv = fragCoord.xy / iResolution.xy;\n"
        "        fragColour = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);\n"
        "    }\n"
        "}\n"
        ;
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
        "this operation is not supported by the Geometry Compiler");
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

void gl_put_as(GL_Frame& f, GL_Value val, const Phrase& src, GL_Type type)
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
            f.gl.out << ");\n";
            return;
        }
    }
    throw Exception(At_GL_Phrase(src, &f), stringify(
        "GL can't convert ",val.type," to ",type));
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
    gl_put_as(f, x, *xexpr.source_, rtype);
    f.gl.out << op;
    gl_put_as(f, y, *yexpr.source_, rtype);
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
// or return missing if it isn't a constant.
Value gl_eval_const(Operation& op, GL_Frame& f)
{
    if (auto c = dynamic_cast<Constant*>(&op))
        return c->value_;
    else if (auto dot = dynamic_cast<Dot_Expr*>(&op)) {
        if (auto ref = dynamic_shared_cast<Nonlocal_Ref>(dot->base_)) {
            auto base = (*f.nonlocal)[ref->slot_];
            if (base.is_ref())
                return base.get_ref_unsafe().getfield(dot->id_);
        }
    }
    else if (auto ref = dynamic_cast<Nonlocal_Ref*>(&op))
        return (*f.nonlocal)[ref->slot_];
    else if (auto fref = dynamic_cast<Nonlocal_Function_Ref*>(&op)) {
        return {make<Closure>(
            (Lambda&) (*f.nonlocal)[fref->lambda_slot_].get_ref_unsafe(),
            *f.nonlocal)};
    }
    return missing;
}

GL_Value Call_Expr::gl_eval(GL_Frame& f) const
{
    auto val = gl_eval_const(*fun_, f);
    if (auto fun = val.dycast<Function>()) {
        if (args_.size() != fun->nargs_) {
            throw Exception(At_GL_Phrase(*call_phrase()->args_, &f),
                "wrong number of arguments");
        }
        auto f2 = GL_Frame::make(fun->nslots_, f.gl, &f, call_phrase());
        for (size_t i = 0; i < args_.size(); ++i)
            (*f2)[i] = args_[i]->gl_eval(f);
        return fun->gl_call(*f2);
    }
    throw Exception(At_GL_Phrase(*fun_->source_, &f),
        "this function cannot be called by the Geometry Compiler");
}

GL_Value At_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Vec2);
    const char* arg2 = nullptr;
    auto k = gl_eval_const(*arg2_, f);
    auto num = k.get_num_or_nan();
    if (num == 0.0)
        arg2 = ".x";
    else if (num == 1.0)
        arg2 = ".y";
    if (arg2 == nullptr)
        throw Exception(At_GL_Phrase(*arg2_->source_, &f),
            "in Geometry Compiler, index must be 0 or 1");
    GL_Value result = f.gl.newvalue(GL_Type::Num);
    f.gl.out << "  float "<<result<<" = "<<arg1<<arg2<<";\n";
    return result;
}

GL_Value Arg_Ref::gl_eval(GL_Frame& f) const
{
    return f[slot_];
}

GL_Value Nonlocal_Ref::gl_eval(GL_Frame& f) const
{
    return gl_eval_const(f, (*f.nonlocal)[slot_], *source_);
}

GL_Value List_Expr::gl_eval(GL_Frame& f) const
{
    if (auto seq = dynamic_shared_cast<Sequence_Gen>(generator_)) {
        if (seq->size() == 2) {
            auto e1 = gl_eval_expr(f, *(*seq)[0], GL_Type::Num);
            auto e2 = gl_eval_expr(f, *(*seq)[1], GL_Type::Num);
            GL_Value result = f.gl.newvalue(GL_Type::Vec2);
            f.gl.out << "  vec2 "<<result<<" = vec2("<<e1<<","<<e2<<");\n";
            return result;
        }
        if (seq->size() == 3) {
            auto e1 = gl_eval_expr(f, *(*seq)[0], GL_Type::Num);
            auto e2 = gl_eval_expr(f, *(*seq)[1], GL_Type::Num);
            auto e3 = gl_eval_expr(f, *(*seq)[2], GL_Type::Num);
            GL_Value result = f.gl.newvalue(GL_Type::Vec3);
            f.gl.out << "  vec3 "<<result<<" = vec3("
                <<e1<<","<<e2<<","<<e3<<");\n";
            return result;
        }
        if (seq->size() == 4) {
            auto e1 = gl_eval_expr(f, *(*seq)[0], GL_Type::Num);
            auto e2 = gl_eval_expr(f, *(*seq)[1], GL_Type::Num);
            auto e3 = gl_eval_expr(f, *(*seq)[2], GL_Type::Num);
            auto e4 = gl_eval_expr(f, *(*seq)[3], GL_Type::Num);
            GL_Value result = f.gl.newvalue(GL_Type::Vec3);
            f.gl.out << "  vec4 "<<result<<" = vec4("
                <<e1<<","<<e2<<","<<e3<<","<<e4<<");\n";
            return result;
        }
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
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num);
    auto arg3 = gl_eval_expr(f, *arg3_, GL_Type::Num);
    GL_Value result = f.gl.newvalue(GL_Type::Num);
    f.gl.out <<"  float "<<result<<" =("<<arg1<<" ? "<<arg2<<" : "<<arg3<<");\n";
    return result;
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

} // namespace curv
