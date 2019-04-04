// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <cctype>
#include <iostream>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/dtostr.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <libcurv/math.h>
#include <libcurv/meaning.h>
#include <libcurv/optimizer.h>
#include <libcurv/picker.h>
#include <libcurv/reactive.h>
#include <libcurv/system.h>

namespace curv {

struct SC_Data_Ref : public Operation
{
    SC_Value val_;
    SC_Data_Ref(Shared<const Phrase> src, SC_Value v)
    : Operation(std::move(src)), val_(v)
    {}
    SC_Value sc_eval(SC_Frame&) const override { return val_; }
    void exec(Frame&, Executor&) const override { }
};

// This is the main entry point into the Shape Compiler.
void
SC_Compiler::define_function(
    const char* name, SC_Type param_type, SC_Type result_type,
    Shared<const Function> func, const Context& cx)
{
    begin_function();
    SC_Value param = newvalue(param_type);
    if (target_ == SC_Target::cpp) {
        out_
            << "extern \"C\" void " << name
            << "(const " << param_type << "* param, "
            << result_type << "* result)\n"
            "{\n"
            "  " << param_type << " " << param << " = *param;\n";
    } else {
        out_ <<
        result_type << " " << name << "(" << param_type << " " << param << ")\n"
        "{\n";
    }
    auto f = SC_Frame::make(0, *this, &cx, nullptr, nullptr);
    auto param_ref = make<SC_Data_Ref>(nullptr, param);
    auto result = func->sc_call_expr(*param_ref, nullptr, *f);
    if (result.type != result_type) {
        throw Exception(cx, stringify(name," function returns ",result.type));
    }
    end_function();
    if (target_ == SC_Target::cpp) {
        out_ << "  *result = " << result << ";\n";
    } else {
        out_ << "  return " << result << ";\n";
    }
    out_ << "}\n";
}

void
SC_Compiler::begin_function()
{
    valcount_ = 0;
    valcache_.clear();
    opcaches_.clear();
    opcaches_.emplace_back(Op_Cache{});
    constants_.str("");
    body_.str("");
}

void
SC_Compiler::end_function()
{
    out_ << "  /* constants */\n";
    out_ << constants_.str();
    out_ << "  /* body */\n";
    out_ << body_.str();
}

SC_Value sc_call_unary_numeric(SC_Frame& f, const char* name)
{
    auto arg = f[0];
    if (!arg.type.is_numeric())
        throw Exception(At_SC_Arg(0, f),
            stringify(name,": argument is not numeric"));
    auto result = f.sc.newvalue(arg.type);
    f.sc.out()<<"  "<<arg.type<<" "<<result<<" = "<<name<<"("<<arg<<");\n";
    return result;
}

struct Set_Purity
{
    SC_Compiler& sc_;
    bool previous_purity_;
    Set_Purity(SC_Compiler& sc, bool purity)
    :
        sc_(sc),
        previous_purity_(sc.in_constants_)
    {
        sc_.in_constants_ = purity;
    }
    ~Set_Purity()
    {
        sc_.in_constants_ = previous_purity_;
    }
};

// Wrapper for Operation::sc_eval(f), does common subexpression elimination.
SC_Value sc_eval_op(SC_Frame& f, const Operation& op)
{
#if OPTIMIZE
    if (!op.pure_) {
      #if 0
        bool previous = f.sc.in_constants_;
        f.sc.in_constants_ = false;
        auto k = f.sc.valcount_;
        f.sc.out()
            <<"/*in "<<k<<" IMPR "
            <<boost::core::demangle(typeid(op).name())<<"*/\n";
        SC_Value val;
        try {
            val = op.sc_eval(f);
        } catch (...) {
            f.sc.out()
                <<"/*except "<<k<<"/"<<f.sc.valcount_<<" IMPR "
                <<boost::core::demangle(typeid(op).name())<<"*/\n";
            f.sc.in_constants_ = previous;
            throw;
        }
        f.sc.out()
            <<"/*out "<<k<<"/"<<f.sc.valcount_<<" IMPR "
            <<boost::core::demangle(typeid(op).name())<<"*/\n";
        f.sc.in_constants_ = previous;
        return val;
      #else
        Set_Purity pu(f.sc, false);
        return op.sc_eval(f);
      #endif
    }
    // 'op' is a uniform expression, consisting of pure operations at interior
    // nodes and Constants at leaf nodes. There can be no variable references
    // (eg, no Data_Ref ops), other than uniform variables in reactive values.
    // What follows is a limited form of common subexpression elimination
    // which reduces code size when reactive values are used.
    for (Op_Cache& opcache : f.sc.opcaches_) {
        auto cached = opcache.find(share(op));
        if (cached != opcache.end())
            return cached->second;
    }
  #if 0
    bool previous = f.sc.in_constants_;
    f.sc.in_constants_ = true;
    auto k = f.sc.valcount_;
    f.sc.out()
        <<"/*in "<<k<<" PURE "
        <<boost::core::demangle(typeid(op).name())<<"*/\n";
    SC_Value val;
    try {
        val = op.sc_eval(f);
    } catch (...) {
        f.sc.out()
            <<"/*except "<<k<<"/"<<f.sc.valcount_<<" PURE "
            <<boost::core::demangle(typeid(op).name())<<"*/\n";
        f.sc.in_constants_ = previous;
        throw;
    }
    f.sc.out()
        <<"/*out "<<k<<"/"<<f.sc.valcount_<<" PURE "
        <<boost::core::demangle(typeid(op).name())<<"*/\n";
    f.sc.in_constants_ = previous;
    f.sc.opcache_[share(op)] = val;
    return val;
  #else
    Set_Purity pu(f.sc, true);
    auto val = op.sc_eval(f);
    f.sc.opcaches_.back()[share(op)] = val;
    return val;
  #endif
#else
    return op.sc_eval(f);
#endif
}

SC_Value sc_eval_expr(SC_Frame& f, const Operation& op, SC_Type type)
{
    SC_Value arg = sc_eval_op(f, op);
    if (arg.type != type) {
        throw Exception(At_SC_Phrase(op.syntax_, f),
            stringify("wrong argument type: expected ",type,", got ",arg.type));
    }
    return arg;
}

#if 0
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
#endif

void
sc_put_num(Value val, const Context& cx, std::ostream& out)
{
    out << dfmt(val.to_num(cx), dfmt::EXPR);
}

void
sc_put_vec(unsigned size, Value val, const Context& cx, std::ostream& out)
{
    auto list = val.to<List>(cx);
    list->assert_size(size, cx);
    out << SC_Type::Vec(size) << "(";
    bool first = true;
    for (unsigned i = 0; i < size; ++i) {
        if (!first) out << ",";
        first = false;
        sc_put_num(list->at(i), cx, out);
    }
    out << ")";
}

SC_Value sc_eval_const(SC_Frame& f, Value val, const Phrase& syntax)
{
#if OPTIMIZE
    auto cached = f.sc.valcache_.find(val);
    if (cached != f.sc.valcache_.end())
        return cached->second;
#endif

    Set_Purity pu(f.sc, true);
    At_SC_Phrase cx(share(syntax), f);
    if (val.is_num()) {
        SC_Value result = f.sc.newvalue(SC_Type::Num());
        double num = val.get_num_unsafe();
        f.sc.out() << "  const float " << result << " = "
            << dfmt(num, dfmt::EXPR) << ";\n";
        f.sc.valcache_[val] = result;
        return result;
    }
    if (val.is_bool()) {
        SC_Value result = f.sc.newvalue(SC_Type::Bool());
        bool b = val.get_bool_unsafe();
        f.sc.out() << "  const bool " << result << " = "
            << (b ? "true" : "false") << ";\n";
        f.sc.valcache_[val] = result;
        return result;
    }
    if (auto list = val.dycast<List>()) {
        if (list->size() == 0 || list->size() > SC_Type::MAX_LIST) {
            throw Exception(cx, stringify(
                "list of size ",list->size(), " is not supported"));
        }
        if (isnum(list->front())) {
            // It is a list of numbers. Size 2..4 is a vector.
            if (list->size() >= 2 && list->size() <= 4) {
                // vector
                SC_Value values[4];
                for (unsigned i = 0; i < list->size(); ++i) {
                    values[i] = sc_eval_const(f, list->at(i), syntax);
                    if (values[i].type != SC_Type::Num())
                        goto error;
                }
                static SC_Type types[5] = {
                    {}, {}, SC_Type::Vec(2), SC_Type::Vec(3), SC_Type::Vec(4)
                };
                SC_Value result = f.sc.newvalue(types[list->size()]);
                f.sc.out() << "  " << result.type << " " << result
                    << " = " << result.type << "(";
                bool first = true;
                for (unsigned i = 0; i < list->size(); ++i) {
                    if (!first) f.sc.out() << ",";
                    first = false;
                    f.sc.out() << values[i];
                }
                f.sc.out() << ");\n";
                f.sc.valcache_[val] = result;
                return result;
            }
            // It is a float array.
            SC_Value result = f.sc.newvalue(SC_Type::Num(list->size()));
            if (f.sc.target_ == SC_Target::cpp) {
                f.sc.out() << "  static const float "<<result<<"[] = {";
            } else {
                f.sc.out() << "  const " << result.type << " " << result
                    << " = " << result.type << "(";
            }
            bool first = true;
            for (unsigned i = 0; i < list->size(); ++i) {
                if (!first) f.sc.out() << ",";
                first = false;
                f.sc.out() << dfmt(list->at(i).to_num(cx), dfmt::EXPR);
            }
            if (f.sc.target_ == SC_Target::cpp) {
                f.sc.out() << "};\n";
            } else {
                f.sc.out() << ");\n";
            }
            f.sc.valcache_[val] = result;
            return result;
          #if 0
            else {
                // matrix
                static SC_Type types[5] = {
                    {}, {}, SC_Type::Mat(2), SC_Type::Mat(3), SC_Type::Mat(4)
                };
                SC_Value result = f.sc.newvalue(types[list->size()]);
                f.sc.out() << "  " << result.type << " " << result
                    << " = " << result.type << "(";
                bool first = true;
                for (size_t i = 0; i < list->size(); ++i) {
                    for (size_t j = 0; j < list->size(); ++j) {
                        double elem;
                        if (get_mat(*list, j, i, elem)) {
                            if (!first) f.sc.out() << ",";
                            first = false;
                            f.sc.out() << dfmt(elem, dfmt::EXPR);
                        } else
                            goto error;
                    }
                }
                f.sc.out() << ");\n";
                return result;
            }
          #endif
        }
        if (auto list2 = list->front().dycast<List>()) {
            if (list2->size() == 0 || list2->size() > SC_Type::MAX_LIST) {
                throw Exception(cx, stringify(
                    "list of size ",list2->size()," is not supported"));
            }
            if (isnum(list2->front())) {
                // A list of lists of numbers...
                if (list2->size() >= 2 && list2->size() <= 4) {
                    // list of vectors
                    SC_Value result =
                        f.sc.newvalue(
                            SC_Type::Vec(list2->size(), list->size()));
                    if (f.sc.target_ == SC_Target::cpp) {
                        f.sc.out()
                            << "  static const " << SC_Type::Vec(list2->size())
                            << " "<<result<<"[] = {";
                    } else {
                        f.sc.out() << "  const " << result.type << " " << result
                            << " = " << result.type << "(";
                    }
                    bool first = true;
                    for (unsigned i = 0; i < list->size(); ++i) {
                        if (!first) f.sc.out() << ",";
                        first = false;
                        sc_put_vec(list2->size(), list->at(i), cx, f.sc.out());
                    }
                    if (f.sc.target_ == SC_Target::cpp) {
                        f.sc.out() << "};\n";
                    } else {
                        f.sc.out() << ");\n";
                    }
                    f.sc.valcache_[val] = result;
                    return result;
                }
                // 2D array of numbers
                SC_Value result =
                    f.sc.newvalue(SC_Type::Num(list->size(), list2->size()));
                if (f.sc.target_ == SC_Target::cpp) {
                    f.sc.out() << "  static const float " <<result<<"[] = {";
                } else {
                    f.sc.out() << "  const " << result.type << " " << result
                        << " = " << result.type << "(";
                }
                bool first = true;
                for (unsigned i = 0; i < list->size(); ++i) {
                    auto l2 = list->at(i).to<List>(cx);
                    if (l2->size() != list2->size())
                        goto error;
                    for (unsigned j = 0; j < l2->size(); ++j) {
                        if (!first) f.sc.out() << ",";
                        first = false;
                        sc_put_num(l2->at(j), cx, f.sc.out());
                    }
                }
                if (f.sc.target_ == SC_Target::cpp) {
                    f.sc.out() << "};\n";
                } else {
                    f.sc.out() << ");\n";
                }
                f.sc.valcache_[val] = result;
                return result;
            }
            if (auto list3 = list2->front().dycast<List>()) {
                // A list of lists of lists: interpret as 2D array of vectors
                if (list3->size() < 2 || list3->size() > 4) goto error;
                SC_Value result =
                    f.sc.newvalue(SC_Type::Vec(list3->size(),
                        list->size(), list2->size()));
                if (f.sc.target_ == SC_Target::cpp) {
                    f.sc.out() << "  static const "
                        << SC_Type::Vec(list3->size())
                        << " " <<result<<"[] = {";
                } else {
                    f.sc.out() << "  const " << result.type << " " << result
                        << " = " << result.type << "(";
                }
                bool first = true;
                for (unsigned i = 0; i < list->size(); ++i) {
                    auto l2 = list->at(i).to<List>(cx);
                    if (l2->size() != list2->size())
                        goto error;
                    for (unsigned j = 0; j < l2->size(); ++j) {
                        if (!first) f.sc.out() << ",";
                        first = false;
                        sc_put_vec(list3->size(), l2->at(j), cx, f.sc.out());
                    }
                }
                if (f.sc.target_ == SC_Target::cpp) {
                    f.sc.out() << "};\n";
                } else {
                    f.sc.out() << ");\n";
                }
                f.sc.valcache_[val] = result;
                return result;
            }
        }
        goto error;
    }
    if (auto re = val.dycast<Reactive_Expression>()) {
        auto f2 = SC_Frame::make(0, f.sc, nullptr, &f, &syntax);
        auto result = sc_eval_op(*f2, *re->expr_);
        f.sc.valcache_[val] = result;
        return result;
    }
    if (auto uv = val.dycast<Uniform_Variable>()) {
        SC_Value result = f.sc.newvalue(uv->sctype_);
        f.sc.out() << "  " << uv->sctype_
            << " " << result << " = " << uv->identifier_ << ";\n";
        f.sc.valcache_[val] = result;
        return result;
    }
error:
    throw Exception(At_SC_Phrase(share(syntax), f),
        stringify("value ",val," is not supported "));
}

SC_Value Operation::sc_eval(SC_Frame& f) const
{
    throw Exception(At_SC_Phrase(syntax_, f), stringify(
        "this expression is not supported: ",
        boost::core::demangle(typeid(*this).name())));
}

void Operation::sc_exec(SC_Frame& f) const
{
    throw Exception(At_SC_Phrase(syntax_, f), stringify(
        "this action is not supported: ",
        boost::core::demangle(typeid(*this).name())));
}

SC_Value Constant::sc_eval(SC_Frame& f) const
{
    return sc_eval_const(f, value_, *syntax_);
}

SC_Value Negative_Expr::sc_eval(SC_Frame& f) const
{
    auto x = sc_eval_op(f, *arg_);
    if (!x.type.is_numeric())
        throw Exception(At_SC_Phrase(arg_->syntax_, f),
            "argument not numeric");
    SC_Value result = f.sc.newvalue(x.type);
    f.sc.out()<<"  "<<x.type<<" "<<result<<" = -"<<x<< ";\n";
    return result;
}

void sc_put_as(SC_Frame& f, SC_Value val, const Context& cx, SC_Type type)
{
    if (val.type == type) {
        f.sc.out() << val;
        return;
    }
    if (val.type == SC_Type::Num()) {
        if (sc_type_count(type) > 1) {
            f.sc.out() << type << "(";
            bool first = true;
            for (unsigned i = 0; i < sc_type_count(type); ++i) {
                if (!first) f.sc.out() << ",";
                f.sc.out() << val;
                first = false;
            }
            f.sc.out() << ")";
            return;
        }
    }
    throw Exception(cx, stringify("can't convert ",val.type," to ",type));
}

SC_Value
sc_arith_expr(SC_Frame& f, const Phrase& syntax,
    const Operation& xexpr, const char* op, const Operation& yexpr)
{
    auto x = sc_eval_op(f, xexpr);
    auto y = sc_eval_op(f, yexpr);

    SC_Type rtype = SC_Type::Bool();
    if (x.type == y.type)
        rtype = x.type;
    else if (x.type == SC_Type::Num())
        rtype = y.type;
    else if (y.type == SC_Type::Num())
        rtype = x.type;
    if (rtype == SC_Type::Bool())
        throw Exception(At_SC_Phrase(share(syntax), f),
            stringify("domain error: ",x.type,op,y.type));

    SC_Value result = f.sc.newvalue(rtype);
    f.sc.out() <<"  "<<rtype<<" "<<result<<" = ";
    if (isalpha(*op)) {
        f.sc.out() << op << "(";
        sc_put_as(f, x, At_SC_Phrase(xexpr.syntax_, f), rtype);
        f.sc.out() << ",";
        sc_put_as(f, y, At_SC_Phrase(yexpr.syntax_, f), rtype);
        f.sc.out() << ")";
    } else {
        sc_put_as(f, x, At_SC_Phrase(xexpr.syntax_, f), rtype);
        f.sc.out() << op;
        sc_put_as(f, y, At_SC_Phrase(yexpr.syntax_, f), rtype);
    }
    f.sc.out() << ";\n";
    return result;
}

SC_Value Add_Expr::sc_eval(SC_Frame& f) const
{
    return sc_arith_expr(f, *syntax_, *arg1_, "+", *arg2_);
}

SC_Value Subtract_Expr::sc_eval(SC_Frame& f) const
{
    return sc_arith_expr(f, *syntax_, *arg1_, "-", *arg2_);
}

SC_Value Multiply_Expr::sc_eval(SC_Frame& f) const
{
    return sc_arith_expr(f, *syntax_, *arg1_, "*", *arg2_);
}

SC_Value Divide_Expr::sc_eval(SC_Frame& f) const
{
    return sc_arith_expr(f, *syntax_, *arg1_, "/", *arg2_);
}

SC_Value Power_Expr::sc_eval(SC_Frame& f) const
{
    return sc_arith_expr(f, *syntax_, *arg1_, "pow", *arg2_);
}

// Evaluate an expression to a constant at SC compile time,
// or abort if it isn't a constant.
Value sc_constify(Operation& op, SC_Frame& f)
{
    if (auto c = dynamic_cast<Constant*>(&op))
        return c->value_;
    else if (auto dot = dynamic_cast<Dot_Expr*>(&op)) {
        Value base = sc_constify(*dot->base_, f);
        if (dot->selector_.id_ != nullptr)
            return base.at(dot->selector_.id_->symbol_,
                At_SC_Phrase(op.syntax_, f));
        else
            throw Exception(At_SC_Phrase(dot->selector_.string_->syntax_, f),
                "not an identifier");
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
            (*listval)[i] = sc_constify(*(*list)[i], f);
        }
        return {listval};
    } else if (auto neg = dynamic_cast<Negative_Expr*>(&op)) {
        Value arg = sc_constify(*neg->arg_, f);
        if (arg.is_num())
            return Value(-arg.get_num_unsafe());
    }
    throw Exception(At_SC_Phrase(op.syntax_, f),
        "not a constant");
}

bool sc_try_constify(Operation& op, SC_Frame& f, Value& val)
{
    try {
        val = sc_constify(op, f);
        return true;
    } catch (Exception&) {
        return false;
    }
}

bool sc_try_eval(Operation& op, SC_Frame& f, SC_Value& val)
{
    try {
        val = sc_eval_op(f, op);
        return true;
    } catch (Exception&) {
        return false;
    }
}

SC_Value Block_Op::sc_eval(SC_Frame& f) const
{
    statements_.sc_exec(f);
    return sc_eval_op(f, *body_);
}
void Block_Op::sc_exec(SC_Frame& f) const
{
    statements_.sc_exec(f);
    body_->sc_exec(f);
}

SC_Value Preaction_Op::sc_eval(SC_Frame& f) const
{
    actions_->sc_exec(f);
    return sc_eval_op(f, *body_);
}
void Preaction_Op::sc_exec(SC_Frame& f) const
{
    actions_->sc_exec(f);
    body_->sc_exec(f);
}

void Compound_Op_Base::sc_exec(SC_Frame& f) const
{
    for (auto s : *this)
        s->sc_exec(f);
}

void Scope_Executable::sc_exec(SC_Frame& f) const
{
    for (auto action : actions_)
        action->sc_exec(f);
}
void Null_Action::sc_exec(SC_Frame&) const
{
}

void
Data_Setter::sc_exec(SC_Frame& f) const
{
    SC_Value val = sc_eval_op(f, *expr_);
    if (reassign_)
        f.sc.out() << "  "<<f[slot_]<<"="<<val<<";\n";
    else {
        SC_Value var = f.sc.newvalue(val.type);
        f.sc.out() << "  "<<var.type<<" "<<var<<"="<<val<<";\n";
        f[slot_] = var;
    }
}
void
Pattern_Setter::sc_exec(SC_Frame& f) const
{
    assert(module_slot_ == (slot_t)(-1));
    pattern_->sc_exec(*definiens_, f, f);
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
        stringify("got ",k,", expected 0..",vecsize-1));
}

// compile array[i] expression
SC_Value sc_eval_index_expr(SC_Value array, Operation& index, SC_Frame& f)
{
    Value k;
    if (array.type.is_vec() && sc_try_constify(index, f, k)) {
        // A vector with a constant index. Swizzling is supported.
        if (auto list = k.dycast<List>()) {
            if (list->size() < 2 || list->size() > 4) {
                throw Exception(At_SC_Phrase(index.syntax_, f),
                    "list index vector must have between 2 and 4 elements");
            }
            char swizzle[5];
            memset(swizzle, 0, 5);
            for (size_t i = 0; i <list->size(); ++i) {
                swizzle[i] = gl_index_letter((*list)[i],
                    array.type.count(),
                    At_Index(i, At_SC_Phrase(index.syntax_, f)));
            }
            SC_Value result = f.sc.newvalue(SC_Type::Vec(list->size()));
            f.sc.out() << "  " << result.type << " "<< result<<" = ";
            if (f.sc.target_ == SC_Target::glsl) {
                // use GLSL swizzle syntax: v.xyz
                f.sc.out() <<array<<"."<<swizzle;
            } else {
                // fall back to a vector constructor: vec3(v.x,v.y,v.z)
                f.sc.out() << result.type << "(";
                bool first = true;
                for (size_t i = 0; i < list->size(); ++i) {
                    if (!first)
                        f.sc.out() << ",";
                    first = false;
                    f.sc.out() << array << "." << swizzle[i];
                }
                f.sc.out() << ")";
            }
            f.sc.out() <<";\n";
            return result;
        }
        const char* arg2 = nullptr;
        auto num = k.get_num_or_nan();
        if (num == 0.0)
            arg2 = ".x";
        else if (num == 1.0)
            arg2 = ".y";
        else if (num == 2.0 && array.type.count() > 2)
            arg2 = ".z";
        else if (num == 3.0 && array.type.count() > 3)
            arg2 = ".w";
        if (arg2 == nullptr)
            throw Exception(At_SC_Phrase(index.syntax_, f),
                stringify("got ",k,", expected 0..",
                    array.type.count()-1));

        SC_Value result = f.sc.newvalue(SC_Type::Num());
        f.sc.out() << "  float "<<result<<" = "<<array<<arg2<<";\n";
        return result;
    }
    // An array of numbers, indexed with a number.
    if (array.type.rank_ > 1) {
        throw Exception(At_SC_Phrase(index.syntax_,f), stringify(
            "can't index a ", array.type.rank_, "D array of ",
            SC_Type(array.type.base_type_), " with a single index"));
    }
    auto ix = sc_eval_expr(f, index, SC_Type::Num());
    SC_Value result = f.sc.newvalue(array.type.abase());
    f.sc.out() << "  " << result.type << " " << result << " = "
             << array << "[int(" << ix << ")];\n";
    return result;
}

// compile array[i,j] expression
SC_Value sc_eval_index2_expr(
    SC_Value array, Operation& op_ix1, Operation& op_ix2, SC_Frame& f,
    const Context& acx)
{
    auto ix1 = sc_eval_expr(f, op_ix1, SC_Type::Num());
    auto ix2 = sc_eval_expr(f, op_ix2, SC_Type::Num());
    if (array.type.rank_ == 2) {
        // 2D array of number or vector. Not supported by GLSL 1.5,
        // so we emulate this type using a 1D array.
        // Index value must be [i,j], can't use a single index.
        SC_Value result = f.sc.newvalue({array.type.base_type_});
        f.sc.out() << "  " << result.type << " " << result << " = " << array
                 << "[int(" << ix1 << ")*" << array.type.dim2_
                 << "+" << "int(" << ix2 << ")];\n";
        return result;
    }
    if (array.type.rank_ == 1 && array.type.base_info().rank == 1) {
        // 1D array of vector.
        SC_Value result = f.sc.newvalue(SC_Type::Num());
        f.sc.out() << "  " << result.type << " " << result << " = " << array
                 << "[int(" << ix1 << ")]"
                 << "[int(" << ix2 << ")];\n";
        return result;
    }
    throw Exception(acx, "2 indexes (a[i,j]) not supported for this array");
}

// compile array[i,j,k] expression
SC_Value sc_eval_index3_expr(
    SC_Value array, Operation& op_ix1, Operation& op_ix2, Operation& op_ix3,
    SC_Frame& f, const Context& acx)
{
    if (array.type.rank_ == 2 && array.type.base_info().rank == 1) {
        // 2D array of vector.
        auto ix1 = sc_eval_expr(f, op_ix1, SC_Type::Num());
        auto ix2 = sc_eval_expr(f, op_ix2, SC_Type::Num());
        auto ix3 = sc_eval_expr(f, op_ix3, SC_Type::Num());
        SC_Value result = f.sc.newvalue(SC_Type::Num());
        f.sc.out() << "  " << result.type << " " << result << " = " << array
                 << "[int(" << ix1 << ")*" << array.type.dim2_
                 << "+" << "int(" << ix2 << ")][int(" << ix3 << ")];\n";
        return result;
    }
    throw Exception(acx, "3 indexes (a[i,j,k]) not supported for this array");
}

SC_Value Call_Expr::sc_eval(SC_Frame& f) const
{
    SC_Value scval;
    if (sc_try_eval(*fun_, f, scval)) {
        if (!scval.type.is_list())
            throw Exception(At_SC_Phrase(fun_->syntax_, f),
                stringify("type ", scval.type, ": not an array or function"));
        auto list = cast<List_Expr>(arg_);
        if (list == nullptr)
            throw Exception(At_SC_Phrase(arg_->syntax_, f),
                "expected '[index]' expression");
        if (list->size() == 1)
            return sc_eval_index_expr(scval, *list->at(0), f);
        if (list->size() == 2)
            return sc_eval_index2_expr(scval, *list->at(0), *list->at(1), f,
                At_SC_Phrase(arg_->syntax_, f));
        if (list->size() == 3)
            return sc_eval_index3_expr(scval,
                *list->at(0), *list->at(1), *list->at(2),
                f, At_SC_Phrase(arg_->syntax_, f));
    }
    Value val = sc_constify(*fun_, f);
    Value v = val;
    for (;;) {
        if (auto fun = v.dycast<Function>()) {
            return fun->sc_call_expr(*arg_, syntax_, f);
        }
        if (auto r = v.dycast<Record>()) {
            static Symbol call_key = "call";
            if (r->hasfield(call_key)) {
                v = r->getfield(call_key,At_SC_Phrase(fun_->syntax_,f));
                continue;
            }
        }
        break;
    }
    throw Exception(At_SC_Phrase(fun_->syntax_, f),
        stringify("",val," is not an array or function"));
}

SC_Value Data_Ref::sc_eval(SC_Frame& f) const
{
    return f[slot_];
}

SC_Value Nonlocal_Data_Ref::sc_eval(SC_Frame& f) const
{
    return sc_eval_const(f, f.nonlocals_->at(slot_), *syntax_);
}
SC_Value Symbolic_Ref::sc_eval(SC_Frame& f) const
{
    auto b = f.nonlocals_->dictionary_->find(name_);
    assert(b != f.nonlocals_->dictionary_->end());
    Value val = f.nonlocals_->get(b->second);
    return sc_eval_const(f, val, *syntax_);
}

SC_Value List_Expr_Base::sc_eval(SC_Frame& f) const
{
    if (this->size() == 2) {
        auto e1 = sc_eval_expr(f, *(*this)[0], SC_Type::Num());
        auto e2 = sc_eval_expr(f, *(*this)[1], SC_Type::Num());
        SC_Value result = f.sc.newvalue(SC_Type::Vec(2));
        f.sc.out() << "  vec2 "<<result<<" = vec2("<<e1<<","<<e2<<");\n";
        return result;
    }
    if (this->size() == 3) {
        auto e1 = sc_eval_expr(f, *(*this)[0], SC_Type::Num());
        auto e2 = sc_eval_expr(f, *(*this)[1], SC_Type::Num());
        auto e3 = sc_eval_expr(f, *(*this)[2], SC_Type::Num());
        SC_Value result = f.sc.newvalue(SC_Type::Vec(3));
        f.sc.out() << "  vec3 "<<result<<" = vec3("
            <<e1<<","<<e2<<","<<e3<<");\n";
        return result;
    }
    if (this->size() == 4) {
        auto e1 = sc_eval_expr(f, *(*this)[0], SC_Type::Num());
        auto e2 = sc_eval_expr(f, *(*this)[1], SC_Type::Num());
        auto e3 = sc_eval_expr(f, *(*this)[2], SC_Type::Num());
        auto e4 = sc_eval_expr(f, *(*this)[3], SC_Type::Num());
        SC_Value result = f.sc.newvalue(SC_Type::Vec(4));
        f.sc.out() << "  vec4 "<<result<<" = vec4("
            <<e1<<","<<e2<<","<<e3<<","<<e4<<");\n";
        return result;
    }
    throw Exception(At_SC_Phrase(syntax_, f),
        "this list constructor is not supported");
}

SC_Value Not_Expr::sc_eval(SC_Frame& f) const
{
    auto arg = sc_eval_expr(f, *arg_, SC_Type::Bool());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" = !"<<arg<<";\n";
    return result;
}
SC_Value Or_Expr::sc_eval(SC_Frame& f) const
{
    // TODO: change Or to use lazy evaluation.
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Bool());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Bool());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" || "<<arg2<<");\n";
    return result;
}
SC_Value And_Expr::sc_eval(SC_Frame& f) const
{
    // TODO: change And to use lazy evaluation.
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Bool());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Bool());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" && "<<arg2<<");\n";
    return result;
}
SC_Value If_Else_Op::sc_eval(SC_Frame& f) const
{
    // TODO: change If to use lazy evaluation.
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Bool());
    auto arg2 = sc_eval_op(f, *arg2_);
    auto arg3 = sc_eval_op(f, *arg3_);
    if (arg2.type != arg3.type) {
        throw Exception(At_SC_Phrase(syntax_, f), stringify(
            "if: type mismatch in 'then' and 'else' arms (",
            arg2.type, ",", arg3.type, ")"));
    }
    SC_Value result = f.sc.newvalue(arg2.type);
    f.sc.out() <<"  "<<arg2.type<<" "<<result
             <<" =("<<arg1<<" ? "<<arg2<<" : "<<arg3<<");\n";
    return result;
}
void If_Else_Op::sc_exec(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Bool());
    f.sc.out() << "  if ("<<arg1<<") {\n";
    arg2_->sc_exec(f);
    f.sc.out() << "  } else {\n";
    arg3_->sc_exec(f);
    f.sc.out() << "  }\n";
}
void If_Op::sc_exec(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Bool());
    f.sc.out() << "  if ("<<arg1<<") {\n";
    arg2_->sc_exec(f);
    f.sc.out() << "  }\n";
}
void While_Op::sc_exec(SC_Frame& f) const
{
    f.sc.opcaches_.emplace_back(Op_Cache{});
    f.sc.out() << "  while (true) {\n";
    auto cond = sc_eval_expr(f, *cond_, SC_Type::Bool());
    f.sc.out() << "  if (!"<<cond<<") break;\n";
    body_->sc_exec(f);
    f.sc.out() << "  }\n";
    f.sc.opcaches_.pop_back();
}
void For_Op::sc_exec(SC_Frame& f) const
{
  #define RANGE_EXPRESSIONS 1
    auto range = cast<const Range_Expr>(list_);
    if (range == nullptr)
        throw Exception(At_SC_Phrase(list_->syntax_, f),
            "not a range");
  #if RANGE_EXPRESSIONS
    // range arguments are general expressions
    auto first = sc_eval_expr(f, *range->arg1_, SC_Type::Num());
    auto last = sc_eval_expr(f, *range->arg2_, SC_Type::Num());
    auto step = range->arg3_ != nullptr
        ? sc_eval_expr(f, *range->arg3_, SC_Type::Num())
        : sc_eval_const(f, Value{1.0}, *syntax_);
  #else
    // range arguments are constants
    double first = sc_constify(*range->arg1_, f)
        .to_num(At_SC_Phrase(range->arg1_->syntax_, f));
    double last = sc_constify(*range->arg2_, f)
        .to_num(At_SC_Phrase(range->arg2_->syntax_, f));
    double step = range->arg3_ != nullptr
        ? sc_constify(*range->arg3_, f)
          .to_num(At_SC_Phrase(range->arg3_->syntax_, f))
        : 1.0;
  #endif
    auto i = f.sc.newvalue(SC_Type::Num());
    f.sc.opcaches_.emplace_back(Op_Cache{});
  #if RANGE_EXPRESSIONS
    f.sc.out() << "  for (float " << i << "=" << first << ";"
             << i << (range->half_open_ ? "<" : "<=") << last << ";"
             << i << "+=" << step << ") {\n";
  #else
    f.sc.out() << "  for (float " << i << "=" << dfmt(first, dfmt::EXPR) << ";"
             << i << (range->half_open_ ? "<" : "<=") << dfmt(last, dfmt::EXPR) << ";"
             << i << "+=" << dfmt(step, dfmt::EXPR) << ") {\n";
  #endif
    pattern_->sc_exec(i, At_SC_Phrase(list_->syntax_, f), f);
    body_->sc_exec(f);
    f.sc.out() << "  }\n";
    f.sc.opcaches_.pop_back();
}
SC_Value Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto a = sc_eval_op(f, *arg1_);
    auto b = sc_eval_op(f, *arg2_);
    if (a.type != b.type || a.type.rank_ > 0) {
        throw Exception(At_SC_Phrase(syntax_, f),
            stringify("domain error: ",a.type," == ",b.type));
    }
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<a<<" == "<<b<<");\n";
    return result;
}
SC_Value Not_Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto a = sc_eval_op(f, *arg1_);
    auto b = sc_eval_op(f, *arg2_);
    if (a.type != b.type || a.type.rank_ > 0) {
        throw Exception(At_SC_Phrase(syntax_, f),
            stringify("domain error: ",a.type," != ",b.type));
    }
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<a<<" != "<<b<<");\n";
    return result;
}
SC_Value Less_Expr::sc_eval(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Num());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Num());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" < "<<arg2<<");\n";
    return result;
}
SC_Value Greater_Expr::sc_eval(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Num());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Num());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" > "<<arg2<<");\n";
    return result;
}
SC_Value Less_Or_Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Num());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Num());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" <= "<<arg2<<");\n";
    return result;
}
SC_Value Greater_Or_Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto arg1 = sc_eval_expr(f, *arg1_, SC_Type::Num());
    auto arg2 = sc_eval_expr(f, *arg2_, SC_Type::Num());
    SC_Value result = f.sc.newvalue(SC_Type::Bool());
    f.sc.out() <<"  bool "<<result<<" =("<<arg1<<" >= "<<arg2<<");\n";
    return result;
}

SC_Value sc_vec_element(SC_Frame& f, SC_Value vec, int i)
{
    SC_Value r = f.sc.newvalue(SC_Type::Num());
    f.sc.out() << "  float " << r << " = " << vec << "[" << i << "];\n";
    return r;
}

} // namespace curv
