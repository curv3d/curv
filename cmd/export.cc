// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"
#include <fstream>
#include <sstream>
#include <libcurv/exception.h>
#include <libcurv/filesystem.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/compiled_shape.h>

void export_curv(curv::Value value,
    curv::System&, const curv::Context&, const Export_Params&,
    std::ostream& out)
{
    out << value << "\n";
}

void export_frag(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::geom::Shape_Recognizer shape(cx, sys);
    if (shape.recognize(value))
        curv::geom::export_frag(shape, std::cout);
    else
        throw curv::Exception(cx, "not a shape");
}

void export_cpp(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::geom::Shape_Recognizer shape(cx, sys);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");
    curv::geom::export_cpp(shape, out);
}

bool is_json_data(curv::Value val)
{
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        switch (ref.type_) {
        case curv::Ref_Value::ty_string:
        case curv::Ref_Value::ty_list:
        case curv::Ref_Value::ty_record:
            return true;
        default:
            return false;
        }
    } else {
        return true; // null, bool or num
    }
}
bool export_json_value(curv::Value val, std::ostream& out)
{
    if (val.is_null()) {
        out << "null";
        return true;
    }
    if (val.is_bool()) {
        out << val;
        return true;
    }
    if (val.is_num()) {
        out << curv::dfmt(val.get_num_unsafe(), curv::dfmt::JSON);
        return true;
    }
    assert(val.is_ref());
    auto& ref = val.get_ref_unsafe();
    switch (ref.type_) {
    case curv::Ref_Value::ty_string:
      {
        auto& str = (curv::String&)ref;
        out << '"';
        for (auto c : str) {
            if (c == '\\' || c == '"')
                out << '\\';
            out << c;
        }
        out << '"';
        return true;
      }
    case curv::Ref_Value::ty_list:
      {
        auto& list = (curv::List&)ref;
        out << "[";
        bool first = true;
        for (auto e : list) {
            if (is_json_data(e)) {
                if (!first) out << ",";
                first = false;
                export_json_value(e, out);
            }
        }
        out << "]";
        return true;
      }
    case curv::Ref_Value::ty_record:
      {
        auto& record = (curv::Record&)ref;
        out << "{";
        bool first = true;
        for (auto i : record.fields_) {
            if (is_json_data(i.second)) {
                if (!first) out << ",";
                first = false;
                out << '"' << i.first << "\":";
                export_json_value(i.second, out);
            }
        }
        out << "}";
        return true;
      }
    default:
        return false;
    }
}
void export_json(curv::Value value,
    curv::System&, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    if (export_json_value(value, out))
        out << "\n";
    else
        throw curv::Exception(cx, "value can't be converted to JSON");
}
