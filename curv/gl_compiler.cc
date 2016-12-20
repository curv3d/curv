// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/gl_compiler.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/shape.h>
#include <curv/meaning.h>
#include <curv/function.h>

using namespace curv;

void curv::gl_compile(const Shape2D& shape, std::ostream& out)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::vec2);

    out <<
        "float main_dist(vec2 " << dist_param << ")\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, gl);

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

GL_Value Operation::gl_eval(GL_Compiler&) const
{
    throw Exception(At_Phrase(*source_, nullptr),
        "this operation is not supported by the Geometry Compiler");
}

GL_Value Constant::gl_eval(GL_Compiler& gl) const
{
    if (value_.is_num()) {
        GL_Value result = gl.newvalue(GL_Type::num);
        double num = value_.get_num_unsafe();
        gl.out << "  float " << result << " = ";
        if (num == 1.0/0.0) // infinity
            gl.out << "1e999";
        else if (num == -1.0/0.0) // -infinity
            gl.out << "-1e999";
        else
            gl.out << value_;
        gl.out << ";\n";
        return result;
    }
    throw Exception(At_Phrase(*source_, nullptr),
        stringify("value ",value_," is not supported by the Geometry Compiler"));
}

GL_Value GL_Compiler::eval_expr(Operation& op, GL_Type type)
{
    GL_Value arg = op.gl_eval(*this);
    if (arg.type != type)
        throw Exception(At_Phrase(*op.source_, nullptr),
            stringify("argument is not a ",type));
    return arg;
}

GL_Value Negative_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg = gl.eval_expr(*arg_, GL_Type::num);
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float " << result << " = -" << arg << ";\n";
    return result;
}

GL_Value Add_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg1 = gl.eval_expr(*arg1_, GL_Type::num);
    auto arg2 = gl.eval_expr(*arg2_, GL_Type::num);
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float " << result << " = " << arg1 << " + " << arg2 << ";\n";
    return result;
}

GL_Value Subtract_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg1 = gl.eval_expr(*arg1_, GL_Type::num);
    auto arg2 = gl.eval_expr(*arg2_, GL_Type::num);
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float " << result << " = " << arg1 << " - " << arg2 << ";\n";
    return result;
}

GL_Value Multiply_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg1 = gl.eval_expr(*arg1_, GL_Type::num);
    auto arg2 = gl.eval_expr(*arg2_, GL_Type::num);
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float " << result << " = " << arg1 << " * " << arg2 << ";\n";
    return result;
}

GL_Value Divide_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg1 = gl.eval_expr(*arg1_, GL_Type::num);
    auto arg2 = gl.eval_expr(*arg2_, GL_Type::num);
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float " << result << " = " << arg1 << " / " << arg2 << ";\n";
    return result;
}

GL_Value Call_Expr::gl_eval(GL_Compiler& gl) const
{
    if (auto func = dynamic_shared_cast<Constant>(fun_)) {
        if (auto funv = func->value_.dycast<Function>()) {
            if (args_.size() != funv->nargs_) {
                throw Exception(At_Phrase(*call_phrase()->args_, nullptr),
                    "wrong number of arguments");
            }
            GL_Args args;
            for (size_t i = 0; i < args_.size(); ++i)
                args.push_back(args_[i]->gl_eval(gl));
            return funv->gl_call(args, gl);
        }
    }
    throw Exception(At_Phrase(*source_, nullptr),
        "this function call does not support Geometry Compiler");
}

GL_Value At_Expr::gl_eval(GL_Compiler& gl) const
{
    auto arg1 = gl.eval_expr(*arg1_, GL_Type::vec2);
    const char* arg2 = nullptr;
    if (auto k = dynamic_shared_cast<Constant>(arg2_)) {
        auto num = k->value_.get_num_or_nan();
        if (num == 0.0)
            arg2 = ".x";
        else if (num == 1.0)
            arg2 = ".y";
    }
    if (arg2 == nullptr)
        throw Exception(At_Phrase(*arg2_->source_, nullptr),
            "in Geometry Compiler, index must be 0 or 1");
    GL_Value result = gl.newvalue(GL_Type::num);
    gl.out << "  float "<<result<<" = "<<arg1<<arg2<<";\n";
    return result;
}

GL_Value Arg_Ref::gl_eval(GL_Compiler& gl) const
{
    if (slot_ == 0)
        return gl.arg0;
    throw Exception(At_Phrase(*source_, nullptr),
        "in Geometry Compiler, only 1 function argument supported");
}
