// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"
#include <fstream>
#include <curv/exception.h>
#include <curv/shape.h>

void export_curv(curv::Value value,
    curv::System&, const curv::Context&, const Export_Params&,
    std::ostream& out)
{
    out << value << "\n";
}

void export_frag(curv::Value value,
    curv::System&, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::Shape_Recognizer shape(cx);
    if (shape.recognize(value))
        curv::gl_compile(shape, std::cout, cx);
    else
        throw curv::Exception(cx, "not a shape");
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

void export_png(curv::Value value,
    curv::System&, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::Shape_Recognizer shape(cx);
    if (shape.recognize(value)) {
        auto fragname = curv::stringify(",curv",getpid(),".frag");
        auto pngname = curv::stringify(",curv",getpid(),".png");
        std::ofstream f(fragname->c_str());
        curv::gl_compile(shape, f, cx);
        f.close();
        auto cmd = curv::stringify(
            "glslViewer -s 0 --headless -o ", pngname->c_str(),
            " ", fragname->c_str(), " >/dev/null");
        system(cmd->c_str());
        auto cmd2 = curv::stringify("cat ",pngname->c_str());
        system(cmd2->c_str());
        unlink(fragname->c_str());
        unlink(pngname->c_str());
    } else
        throw curv::Exception(cx, "not a shape");
}
