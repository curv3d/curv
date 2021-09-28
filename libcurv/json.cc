// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/json.h>

#include <libcurv/format.h>
#include <libcurv/list.h>
#include <libcurv/record.h>

#include <cstdio>

namespace curv {

void write_json_string(const char* str, std::ostream& out)
{
    out << '"';
    for (const char* p = str; *p != '\0'; ++p) {
        // The JSON standard prohibits raw control characters in a string.
        // There are 'relaxed' JSON parsers that handle this. But in the
        // JSON-API protocol, top level objects are separated by newlines,
        // and for ease of parsing by the client, top level objects
        // cannot contain raw newlines.
        if (*p == '\n')
            out << "\\n";
        else if (*p == '\t')
            out << "\\t";
        else if (*p < 32 || *p > 126) {
            char hex[3];
            snprintf(hex, 3, "%02X", (int)(unsigned char)(*p));
            out << "\\u00" << hex;
        } else {
            if (*p == '\\' || *p == '"')
                out << '\\';
            out << *p;
        }
    }
    out << '"';
}

void write_json_value(Value val, std::ostream& out)
{
    if (val.is_bool()) {
        out << (val.to_bool_unsafe() ? "true" : "false");
        return;
    }
    if (val.is_num()) {
        out << dfmt(val.to_num_unsafe(), dfmt::JSON);
        return;
    }
    assert(val.is_ref());
    auto& ref = val.to_ref_unsafe();
    switch (ref.type_) {
    case Ref_Value::ty_symbol:
      {
        auto& sym = (Symbol&)ref;
        if (sym == "null")
            out << "null";
        else
            write_json_string(sym.c_str(), out);
        return;
      }
    case Ref_Value::ty_abstract_list:
        switch (ref.subtype_) {
        case Ref_Value::sty_string:
          {
            auto& str = (String&)ref;
            write_json_string(str.c_str(), out);
            return;
          }
        case Ref_Value::sty_list:
          {
            auto& list = (List&)ref;
            out << "[";
            bool first = true;
            for (auto e : list) {
                if (!first) out << ",";
                first = false;
                write_json_value(e, out);
            }
            out << "]";
            return;
          }
        }
    case Ref_Value::ty_record:
      {
        auto& record = (Record&)ref;
        out << "{";
        bool first = true;
        for (auto f = record.iter(); !f->empty(); f->next()) {
            if (!first) out << ",";
            first = false;
            write_json_string(f->key().c_str(), out);
            out << ":";
            Value fval = f->maybe_value();
            if (fval.is_missing()) {
                out << "\"\\u0000\"";
            } else {
                write_json_value(fval, out);
            }
        }
        out << "}";
        return;
      }
    default:
      {
        auto str = stringify(val);
        write_json_string(str->c_str(), out);
        return;
      }
    }
}

} // namespace curv
