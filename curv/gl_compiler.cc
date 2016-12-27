// Copyright Doug Moen 2016.
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

using namespace curv;
using aux::dfmt;

void curv::gl_compile(const Shape2D& shape, std::ostream& out)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::vec2);
    auto frame = GL_Frame::make(0, gl, nullptr, nullptr);

    out <<
        "float main_dist(vec2 " << dist_param << ")\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, *frame);

    out <<
        "  return " << result << ";\n"
        "}\n"
        "// minX, minY, maxX, maxY\n"
        "const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);\n"
        "void mainImage( out vec4 fragColor, in vec2 fragCoord )\n"
        "{\n"
        "    // transform `fragCoord` from viewport to world coordinates\n"
        "    float scale = (bbox.w-bbox.y)/iResolution.y;\n"
        "    float d = main_dist(fragCoord.xy*scale+bbox.xy);\n"
        "    if (d < 0.0)\n"
        "        fragColor = vec4(0,0,0,0);\n"
        "    else {\n"
        "        vec2 uv = fragCoord.xy / iResolution.xy;\n"
        "        fragColor = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);\n"
        "    }\n"
        "}\n"
        ;
}

std::ostream& curv::operator<<(std::ostream& out, GL_Type type)
{
    switch (type) {
    case GL_Type::num:
        out << "Num";
        break;
    case GL_Type::vec2:
        out << "Vec2";
        break;
    }
    return out;
}

GL_Value curv::gl_eval_expr(GL_Frame& f, const Operation& op, GL_Type type)
{
    GL_Value arg = op.gl_eval(f);
    if (arg.type != type)
        throw Exception(At_GL_Phrase(*op.source_, &f),
            stringify("argument is not a ",type));
    return arg;
}

GL_Value curv::gl_eval_const(GL_Frame& f, Value val, const Phrase& source)
{
    if (val.is_num()) {
        GL_Value result = f.gl.newvalue(GL_Type::num);
        double num = val.get_num_unsafe();
        f.gl.out << "  float " << result << " = "
            << dfmt(num, dfmt::EXPR) << ";\n";
        return result;
    }
    if (auto list = val.dycast<List>()) {
        if (list->size() == 2) {
        }
    }
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
    auto arg = gl_eval_expr(f, *arg_, GL_Type::num);
    GL_Value result = f.gl.newvalue(GL_Type::num);
    f.gl.out << "  float " << result << " = -" << arg << ";\n";
    return result;
}

GL_Value Add_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::num);
    GL_Value result = f.gl.newvalue(GL_Type::num);
    f.gl.out << "  float " << result << " = " << arg1 << " + " << arg2 << ";\n";
    return result;
}

GL_Value Subtract_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::num);
    GL_Value result = f.gl.newvalue(GL_Type::num);
    f.gl.out << "  float " << result << " = " << arg1 << " - " << arg2 << ";\n";
    return result;
}

GL_Value Multiply_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::num);
    GL_Value result = f.gl.newvalue(GL_Type::num);
    f.gl.out << "  float " << result << " = " << arg1 << " * " << arg2 << ";\n";
    return result;
}

GL_Value Divide_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::num);
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::num);
    GL_Value result = f.gl.newvalue(GL_Type::num);
    f.gl.out << "  float " << result << " = " << arg1 << " / " << arg2 << ";\n";
    return result;
}

// Evaluate an expression to a constant at GL compile time,
// or return missing if it isn't a constant.
namespace curv {
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
    return missing;
}
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
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::vec2);
    const char* arg2 = nullptr;
    if (auto k = dynamic_shared_cast<Constant>(arg2_)) {
        auto num = k->value_.get_num_or_nan();
        if (num == 0.0)
            arg2 = ".x";
        else if (num == 1.0)
            arg2 = ".y";
    }
    if (arg2 == nullptr)
        throw Exception(At_GL_Phrase(*arg2_->source_, &f),
            "in Geometry Compiler, index must be 0 or 1");
    GL_Value result = f.gl.newvalue(GL_Type::num);
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
            auto e1 = gl_eval_expr(f, *(*seq)[0], GL_Type::num);
            auto e2 = gl_eval_expr(f, *(*seq)[1], GL_Type::num);
            GL_Value result = f.gl.newvalue(GL_Type::vec2);
            f.gl.out << "  vec2 "<<result<<" = vec2("<<e1<<","<<e2<<");\n";
            return result;
        }
    }
    throw Exception(At_GL_Phrase(*source_, &f),
        "this list constructor does not support the Geometry Compiler");
}
