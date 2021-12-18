// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/symbol.h>

#include <libcurv/exception.h>
#include <libcurv/reactive.h>

#include <cctype>

namespace curv {

bool is_symbol(Value a)
{
    if (a.is_bool()) return true;
    if (a.maybe<Symbol>() != nullptr) return true;
    auto r = a.maybe<Reactive_Value>();
    return r && r->sctype_.is_bool();
}

const char Symbol_Base::name[] = "symbol";

Symbol_Ref
make_symbol(const char* str, size_t len)
{
    return Symbol::make(str, len);
}

Symbol_Ref make_symbol(const char* s)
{
    return Symbol::make(s, strlen(s));
}

bool is_C_identifier(const char* p)
{
    if (!(isalpha(*p) || *p == '_'))
        return false;
    for (++p; *p != '\0'; ++p)
        if (!(isalnum(*p) || *p == '_'))
            return false;
    return true;
}

bool Symbol_Ref::is_identifier() const
{
    if (empty())
        return false;
    return is_C_identifier(c_str());
}

Symbol_Ref token_to_symbol(Range<const char*> str)
{
    if (str[0] == '\'') {
        // parse the quoted identifier
        assert(str.size() >= 2);
        const char* end = &str[str.size() - 1];
        assert(*end == '\'');
        String_Builder sb;
        for (const char* p = &str[1]; p < end; ++p) {
            if (p[0] == '\'') {
                ++p;
                assert(p < end && *p == '_');
                sb << '\'';
            } else
                sb << *p;
        }
        return make_symbol(sb.str());
    } else {
        return make_symbol(str);
    }
}

Symbol_Ref value_to_symbol(Value val, const Context& cx)
{
    static Symbol_Ref true_sym = make_symbol("true");
    static Symbol_Ref false_sym = make_symbol("false");

    if (val.eq(Value{true}))
        return true_sym;
    if (val.eq(Value{false}))
        return false_sym;
    return val.to<Symbol>(cx);
}
Symbol_Ref maybe_symbol(Value val)
{
    static Symbol_Ref true_sym = make_symbol("true");
    static Symbol_Ref false_sym = make_symbol("false");

    if (val.eq(Value{true}))
        return true_sym;
    if (val.eq(Value{false}))
        return false_sym;
    return val.maybe<Symbol>();
}
Value Symbol_Ref::to_value() const
{
    if (*this == "true")
        return Value{true};
    if (*this == "false")
        return Value{false};
    return Value{*this};
}

void print_idstr(const char* id, std::ostream& out)
{
    if (is_C_identifier(id))
        out << id;
    else {
        out << '\'';
        for (const char* s = id; *s != '\0'; ++s) {
            char c = *s;
            if (c == '\'')
                out << "'_";
            else
                out << c;
        }
        out << '\'';
    }
}

std::ostream& operator<<(std::ostream& out, Symbol_Ref a)
{
    print_idstr(a.c_str(), out);
    return out;
}

void
Symbol_Base::print_repr(std::ostream& out, Prec) const
{
    out << "#";
    print_idstr(data_, out);
}

int
value_to_enum(
    Value val,
    const std::vector<const char*>& e,
    const Context& cx)
{
    auto sym = value_to_symbol(val, cx);
    for (unsigned i = 0; i < e.size(); ++i) {
        if (strcmp(sym.c_str(), e[i]) == 0)
            return int(i);
    }
    String_Builder msg;
    msg << val << " is not in [";
    bool first = true;
    for (unsigned i = 0; i < e.size(); ++i) {
        if (!first) msg << ",";
        first = false;
        msg << "#" << e[i];
    }
    msg << "]";
    throw Exception(cx, msg.get_string());
}

}
