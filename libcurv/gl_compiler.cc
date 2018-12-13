// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <cctype>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/dtostr.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/gl_context.h>
#include <libcurv/math.h>
#include <libcurv/meaning.h>
#include <libcurv/picker.h>
#include <libcurv/reactive.h>

namespace curv {

GL_Value gl_call_unary_numeric(GL_Frame& f, const char* name)
{
    auto arg = f[0];
    if (!arg.type.is_numeric())
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
        throw Exception(At_GL_Phrase(op.syntax_, f),
            stringify("wrong argument type: expected ",type,", got ",arg.type));
    }
    return arg;
}

bool
get_mat(List& list, int i, int j, double& elem)
{
    if (auto row = list[i].dycast<List>()) {
        if (row->size() == list.size()) {
            Value e = row->at(j);
            if (e.is_num()) {
                elem = e.get_num_unsafe();
                return true;
            }
        }
    }
    return false;
}

GL_Value gl_eval_const(GL_Frame& f, Value val, const Phrase& syntax)
{
    if (val.is_num()) {
        GL_Value result = f.gl.newvalue(GL_Type::Num());
        double num = val.get_num_unsafe();
        f.gl.out << "  float " << result << " = "
            << dfmt(num, dfmt::EXPR) << ";\n";
        return result;
    }
    if (val.is_bool()) {
        GL_Value result = f.gl.newvalue(GL_Type::Bool());
        bool b = val.get_bool_unsafe();
        f.gl.out << "  bool " << result << " = "
            << (b ? "true" : "false") << ";\n";
        return result;
    }
    if (auto list = val.dycast<List>()) {
        if (list->size() >= 2 && list->size() <= 4) {
            if (isnum(list->front())) {
                // vector
                GL_Value values[4];
                for (unsigned i = 0; i < list->size(); ++i) {
                    values[i] = gl_eval_const(f, list->at(i), syntax);
                    if (values[i].type != GL_Type::Num())
                        goto error;
                }
                static GL_Type types[5] = {
                    {}, {}, GL_Type::Vec(2), GL_Type::Vec(3), GL_Type::Vec(4)
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
                for (unsigned i = 0; i < list->size(); ++i) {
                    if (!first) f.gl.out << ",";
                    first = false;
                    f.gl.out << values[i];
                }
                f.gl.out << ");\n";
                return result;
            } else {
                // matrix
                static GL_Type types[5] = {
                    {}, {}, GL_Type::Mat(2), GL_Type::Mat(3), GL_Type::Mat(4)
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
                for (size_t i = 0; i < list->size(); ++i) {
                    for (size_t j = 0; j < list->size(); ++j) {
                        double elem;
                        if (get_mat(*list, j, i, elem)) {
                            if (!first) f.gl.out << ",";
                            first = false;
                            f.gl.out << dfmt(elem, dfmt::EXPR);
                        } else
                            goto error;
                    }
                }
                f.gl.out << ");\n";
                return result;
            }
        }
        goto error;
    }
    if (auto re = val.dycast<Reactive_Expression>()) {
        auto f2 = GL_Frame::make(0, f.gl, nullptr, &f, &syntax);
        return re->expr_->gl_eval(*f2);
    }
    if (auto uv = val.dycast<Uniform_Variable>()) {
        GL_Value result = f.gl.newvalue(uv->gltype_);
        f.gl.out << "  " << uv->gltype_
            << " " << result << " = rv_" << uv->name_ << ";\n";
        return result;
    }
error:
    throw Exception(At_GL_Phrase(share(syntax), f),
        stringify("value ",val," is not supported by the Geometry Compiler"));
}

GL_Value Operation::gl_eval(GL_Frame& f) const
{
    throw Exception(At_GL_Phrase(syntax_, f), stringify(
        "this expression is not supported by the Geometry Compiler: ",
        boost::core::demangle(typeid(*this).name())));
}

void Operation::gl_exec(GL_Frame& f) const
{
    throw Exception(At_GL_Phrase(syntax_, f), stringify(
        "this action is not supported by the Geometry Compiler: ",
        boost::core::demangle(typeid(*this).name())));
}

GL_Value Constant::gl_eval(GL_Frame& f) const
{
    return gl_eval_const(f, value_, *syntax_);
}

GL_Value Negative_Expr::gl_eval(GL_Frame& f) const
{
    auto x = arg_->gl_eval(f);
    if (!x.type.is_numeric())
        throw Exception(At_GL_Phrase(arg_->syntax_, f),
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
    if (val.type == GL_Type::Num()) {
        if (gl_type_count(type) > 1) {
            f.gl.out << type << "(";
            bool first = true;
            for (unsigned i = 0; i < gl_type_count(type); ++i) {
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
gl_arith_expr(GL_Frame& f, const Phrase& syntax,
    const Operation& xexpr, const char* op, const Operation& yexpr)
{
    auto x = xexpr.gl_eval(f);
    auto y = yexpr.gl_eval(f);

    GL_Type rtype = GL_Type::Bool();
    if (x.type == y.type)
        rtype = x.type;
    else if (x.type == GL_Type::Num())
        rtype = y.type;
    else if (y.type == GL_Type::Num())
        rtype = x.type;
    if (rtype == GL_Type::Bool())
        throw Exception(At_GL_Phrase(share(syntax), f),
            stringify("GL domain error: ",x.type,op,y.type));

    GL_Value result = f.gl.newvalue(rtype);
    f.gl.out <<"  "<<rtype<<" "<<result<<" = ";
    if (isalpha(*op)) {
        f.gl.out << op << "(";
        gl_put_as(f, x, At_GL_Phrase(xexpr.syntax_, f), rtype);
        f.gl.out << ",";
        gl_put_as(f, y, At_GL_Phrase(yexpr.syntax_, f), rtype);
        f.gl.out << ")";
    } else {
        gl_put_as(f, x, At_GL_Phrase(xexpr.syntax_, f), rtype);
        f.gl.out << op;
        gl_put_as(f, y, At_GL_Phrase(yexpr.syntax_, f), rtype);
    }
    f.gl.out << ";\n";
    return result;
}

GL_Value Add_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *syntax_, *arg1_, "+", *arg2_);
}

GL_Value Subtract_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *syntax_, *arg1_, "-", *arg2_);
}

GL_Value Multiply_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *syntax_, *arg1_, "*", *arg2_);
}

GL_Value Divide_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *syntax_, *arg1_, "/", *arg2_);
}

GL_Value Power_Expr::gl_eval(GL_Frame& f) const
{
    return gl_arith_expr(f, *syntax_, *arg1_, "pow", *arg2_);
}

// Evaluate an expression to a constant at GL compile time,
// or abort if it isn't a constant.
Value gl_constify(Operation& op, GL_Frame& f)
{
    if (auto c = dynamic_cast<Constant*>(&op))
        return c->value_;
    else if (auto dot = dynamic_cast<Dot_Expr*>(&op)) {
        Value base = gl_constify(*dot->base_, f);
        if (dot->selector_.id_ != nullptr)
            return base.at(dot->selector_.id_->symbol_,
                At_GL_Phrase(op.syntax_, f));
        else
            throw Exception(At_GL_Phrase(dot->selector_.string_->syntax_, f),
                "Geometry Compiler: not an identifier");
    }
    else if (auto ref = dynamic_cast<Nonlocal_Data_Ref*>(&op))
        return f.nonlocals_->at(ref->slot_);
    else if (auto ref = dynamic_cast<Symbolic_Ref*>(&op)) {
        auto b = f.nonlocals_->dictionary_->find(ref->name_);
        assert(b != f.nonlocals_->dictionary_->end());
        return f.nonlocals_->get(b->second);
    }
    else if (auto list = dynamic_cast<List_Expr*>(&op)) {
        Shared<List> listval = List::make(list->size());
        for (size_t i = 0; i < list->size(); ++i) {
            (*listval)[i] = gl_constify(*(*list)[i], f);
        }
        return {listval};
    } else if (auto neg = dynamic_cast<Negative_Expr*>(&op)) {
        Value arg = gl_constify(*neg->arg_, f);
        if (arg.is_num())
            return Value(-arg.get_num_unsafe());
    }
    throw Exception(At_GL_Phrase(op.syntax_, f),
        "Geometry Compiler: not a constant");
}

bool gl_try_eval(Operation& op, GL_Frame& f, GL_Value& val)
{
    try {
        val = op.gl_eval(f);
        return true;
    } catch (Exception&) {
        return false;
    }
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

GL_Value Preaction_Op::gl_eval(GL_Frame& f) const
{
    actions_->gl_exec(f);
    return body_->gl_eval(f);
}
void Preaction_Op::gl_exec(GL_Frame& f) const
{
    actions_->gl_exec(f);
    body_->gl_exec(f);
}

void Compound_Op_Base::gl_exec(GL_Frame& f) const
{
    for (auto s : *this)
        s->gl_exec(f);
}

void Scope_Executable::gl_exec(GL_Frame& f) const
{
    for (auto action : actions_)
        action->gl_exec(f);
}
void Null_Action::gl_exec(GL_Frame&) const
{
}

void
Data_Setter::gl_exec(GL_Frame& f) const
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
void
Pattern_Setter::gl_exec(GL_Frame& f) const
{
    assert(module_slot_ == (slot_t)(-1));
    pattern_->gl_exec(*definiens_, f, f);
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

GL_Value gl_eval_index_expr(
    GL_Value arg1, const Phrase& src1, Operation& index, GL_Frame& f)
{
    if (!arg1.type.is_vec())
        throw Exception(At_GL_Phrase(share(src1), f), "not a vector");

    auto k = gl_constify(index, f);
    if (auto list = k.dycast<List>()) {
        if (list->size() < 2 || list->size() > 4) {
            throw Exception(At_GL_Phrase(index.syntax_, f),
                "list index vector must have between 2 and 4 elements");
        }
        char swizzle[5];
        memset(swizzle, 0, 5);
        for (size_t i = 0; i <list->size(); ++i) {
            swizzle[i] = gl_index_letter((*list)[i],
                arg1.type.count(),
                At_Index(i, At_GL_Phrase(index.syntax_, f)));
        }
        GL_Value result = f.gl.newvalue(GL_Type::Vec(list->size()));
        f.gl.out << "  " << result.type << " "<< result<<" = ";
        if (f.gl.target == GL_Target::glsl) {
            // use GLSL swizzle syntax: v.xyz
            f.gl.out <<arg1<<"."<<swizzle;
        } else {
            // fall back to a vector constructor: vec3(v.x,v.y,v.z)
            f.gl.out << result.type << "(";
            bool first = true;
            for (size_t i = 0; i < list->size(); ++i) {
                if (!first)
                    f.gl.out << ",";
                first = false;
                f.gl.out << arg1 << "." << swizzle[i];
            }
            f.gl.out << ")";
        }
        f.gl.out <<";\n";
        return result;
    }
    const char* arg2 = nullptr;
    auto num = k.get_num_or_nan();
    if (num == 0.0)
        arg2 = ".x";
    else if (num == 1.0)
        arg2 = ".y";
    else if (num == 2.0 && arg1.type.count() > 2)
        arg2 = ".z";
    else if (num == 3.0 && arg1.type.count() > 3)
        arg2 = ".w";
    if (arg2 == nullptr)
        throw Exception(At_GL_Phrase(index.syntax_, f),
            stringify("Geometry Compiler: got ",k,", expected 0..",
                arg1.type.count()-1));

    GL_Value result = f.gl.newvalue(GL_Type::Num());
    f.gl.out << "  float "<<result<<" = "<<arg1<<arg2<<";\n";
    return result;
}

GL_Value Index_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = arg1_->gl_eval(f);
    return gl_eval_index_expr(arg1, *arg1_->syntax_, *arg2_, f);
}

GL_Value Call_Expr::gl_eval(GL_Frame& f) const
{
    GL_Value glval;
    if (gl_try_eval(*fun_, f, glval)) {
        auto list = cast<List_Expr>(arg_);
        if (list == nullptr || list->size() != 1)
            throw Exception(At_GL_Phrase(arg_->syntax_, f),
                "Geometry Compiler: expected '[index]' expression");
        return gl_eval_index_expr(glval, *fun_->syntax_, *list->at(0), f);
    }
    Value val = gl_constify(*fun_, f);
    Value v = val;
    for (;;) {
        if (auto fun = v.dycast<Function>()) {
            return fun->gl_call_expr(*arg_, call_phrase(), f);
        }
        if (auto r = v.dycast<Record>()) {
            static Symbol call_key = "call";
            if (r->hasfield(call_key)) {
                v = r->getfield(call_key,At_GL_Phrase(fun_->syntax_,f));
                continue;
            }
        }
        break;
    }
    throw Exception(At_GL_Phrase(fun_->syntax_, f),
        stringify("Geometry Compiler: ",val," is not a function"));
}

GL_Value Data_Ref::gl_eval(GL_Frame& f) const
{
    return f[slot_];
}

GL_Value Nonlocal_Data_Ref::gl_eval(GL_Frame& f) const
{
    return gl_eval_const(f, f.nonlocals_->at(slot_), *syntax_);
}
GL_Value Symbolic_Ref::gl_eval(GL_Frame& f) const
{
    auto b = f.nonlocals_->dictionary_->find(name_);
    assert(b != f.nonlocals_->dictionary_->end());
    Value val = f.nonlocals_->get(b->second);
    return gl_eval_const(f, val, *syntax_);
}

GL_Value List_Expr_Base::gl_eval(GL_Frame& f) const
{
    if (this->size() == 2) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num());
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num());
        GL_Value result = f.gl.newvalue(GL_Type::Vec(2));
        f.gl.out << "  vec2 "<<result<<" = vec2("<<e1<<","<<e2<<");\n";
        return result;
    }
    if (this->size() == 3) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num());
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num());
        auto e3 = gl_eval_expr(f, *(*this)[2], GL_Type::Num());
        GL_Value result = f.gl.newvalue(GL_Type::Vec(3));
        f.gl.out << "  vec3 "<<result<<" = vec3("
            <<e1<<","<<e2<<","<<e3<<");\n";
        return result;
    }
    if (this->size() == 4) {
        auto e1 = gl_eval_expr(f, *(*this)[0], GL_Type::Num());
        auto e2 = gl_eval_expr(f, *(*this)[1], GL_Type::Num());
        auto e3 = gl_eval_expr(f, *(*this)[2], GL_Type::Num());
        auto e4 = gl_eval_expr(f, *(*this)[3], GL_Type::Num());
        GL_Value result = f.gl.newvalue(GL_Type::Vec(4));
        f.gl.out << "  vec4 "<<result<<" = vec4("
            <<e1<<","<<e2<<","<<e3<<","<<e4<<");\n";
        return result;
    }
    throw Exception(At_GL_Phrase(syntax_, f),
        "this list constructor does not support the Geometry Compiler");
}

GL_Value Not_Expr::gl_eval(GL_Frame& f) const
{
    auto arg = gl_eval_expr(f, *arg_, GL_Type::Bool());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" = !"<<arg<<";\n";
    return result;
}
GL_Value Or_Expr::gl_eval(GL_Frame& f) const
{
    // TODO: change GL Or to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Bool());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" || "<<arg2<<");\n";
    return result;
}
GL_Value And_Expr::gl_eval(GL_Frame& f) const
{
    // TODO: change GL And to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Bool());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" && "<<arg2<<");\n";
    return result;
}
GL_Value If_Else_Op::gl_eval(GL_Frame& f) const
{
    // TODO: change GL If to use lazy evaluation.
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool());
    auto arg2 = arg2_->gl_eval(f);
    auto arg3 = arg3_->gl_eval(f);
    if (arg2.type != arg3.type) {
        throw Exception(At_GL_Phrase(syntax_, f),
            "Geometry Compiler: if: type mismatch in 'then' and 'else' arms");
    }
    GL_Value result = f.gl.newvalue(arg2.type);
    f.gl.out <<"  "<<arg2.type<<" "<<result
             <<" =("<<arg1<<" ? "<<arg2<<" : "<<arg3<<");\n";
    return result;
}
void If_Else_Op::gl_exec(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool());
    f.gl.out << "  if ("<<arg1<<") {\n";
    arg2_->gl_exec(f);
    f.gl.out << "  } else {\n";
    arg3_->gl_exec(f);
    f.gl.out << "  }\n";
}
void If_Op::gl_exec(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Bool());
    f.gl.out << "  if ("<<arg1<<") {\n";
    arg2_->gl_exec(f);
    f.gl.out << "  }\n";
}
void While_Action::gl_exec(GL_Frame& f) const
{
    f.gl.out << "  while (true) {\n";
    auto cond = gl_eval_expr(f, *cond_, GL_Type::Bool());
    f.gl.out << "  if (!"<<cond<<") break;\n";
    body_->gl_exec(f);
    f.gl.out << "  }\n";
}
void For_Op::gl_exec(GL_Frame& f) const
{
    auto range = cast<const Range_Expr>(list_);
    if (range == nullptr)
        throw Exception(At_GL_Phrase(list_->syntax_, f),
            "GL: not a range");
    /*
    auto first = gl_eval_expr(f, *range->arg1_, GL_Type::Num());
    auto last = gl_eval_expr(f, *range->arg2_, GL_Type::Num());
    auto step = gl_eval_expr(f, *range->arg3_, GL_Type::Num());
    */
    double first = gl_constify(*range->arg1_, f)
        .to_num(At_GL_Phrase(range->arg1_->syntax_, f));
    double last = gl_constify(*range->arg2_, f)
        .to_num(At_GL_Phrase(range->arg2_->syntax_, f));
    double step = range->arg3_ != nullptr
        ? gl_constify(*range->arg3_, f)
          .to_num(At_GL_Phrase(range->arg3_->syntax_, f))
        : 1.0;
    auto i = f.gl.newvalue(GL_Type::Num());
    f.gl.out << "  for (float " << i << "=" << dfmt(first, dfmt::EXPR) << ";"
             << i << (range->half_open_ ? "<" : "<=") << dfmt(last, dfmt::EXPR) << ";"
             << i << "+=" << dfmt(step, dfmt::EXPR) << ") {\n";
    pattern_->gl_exec(i, At_GL_Phrase(list_->syntax_, f), f);
    body_->gl_exec(f);
    f.gl.out << "  }\n";
}
GL_Value Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" == "<<arg2<<");\n";
    return result;
}
GL_Value Not_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" != "<<arg2<<");\n";
    return result;
}
GL_Value Less_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" < "<<arg2<<");\n";
    return result;
}
GL_Value Greater_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" > "<<arg2<<");\n";
    return result;
}
GL_Value Less_Or_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" <= "<<arg2<<");\n";
    return result;
}
GL_Value Greater_Or_Equal_Expr::gl_eval(GL_Frame& f) const
{
    auto arg1 = gl_eval_expr(f, *arg1_, GL_Type::Num());
    auto arg2 = gl_eval_expr(f, *arg2_, GL_Type::Num());
    GL_Value result = f.gl.newvalue(GL_Type::Bool());
    f.gl.out <<"  bool "<<result<<" =("<<arg1<<" >= "<<arg2<<");\n";
    return result;
}

GL_Value gl_vec_element(GL_Frame& f, GL_Value vec, int i)
{
    GL_Value r = f.gl.newvalue(GL_Type::Num());
    f.gl.out << "  float " << r << " = " << vec << "[" << i << "];\n";
    return r;
}

} // namespace curv
