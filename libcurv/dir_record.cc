// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/dir_record.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/import.h>
#include <cstdlib>
#include <iostream>

namespace curv {

/*
struct Dir_Record : public Record
{
    Filesystem::path dir_;
    struct File
    {
        System::Importer importer_;
        Filesystem::path path_;
        Value value_;
    };
    Symbol_Map<File> fields_;
};
*/

Dir_Record::Dir_Record(Filesystem::path dir, const Context& cx)
:
    Record(Ref_Value::sty_dir_record),
    dir_(dir),
    fields_{}
{
    namespace fs = std::filesystem;
    System& sys(cx.system());

    fs::directory_iterator i(dir);
    fs::directory_iterator end;
    for (; i != end; ++i) {
        auto path = i->path();
        auto name = path.filename().string();
        auto cname = name.c_str();
        if (cname[0] == '.') continue;
        
        // Construct filename extension (includes leading '.').
        std::string ext = path.extension().string();
        for (char& c : ext)
            c = std::tolower(c);

        // Import file based on its filename extension. Filenames with
        // unknown extensions are ignored.
        if (ext.empty()) {
            // If a filename has no extension, it is statted to test if it is
            // a directory. If yes, it is imported, otherwise it is ignored.
            std::error_code errcode;
            if (fs::is_directory(path, errcode))
                fields_[make_symbol(cname)] = File{dir_import, path, missing};
            else if (errcode)
                throw Exception(cx, stringify(path,": ",errcode.message()));
            continue;
        }
        auto importp = sys.importers_.find(ext);
        if (importp != sys.importers_.end())
            fields_[make_symbol(path.stem().string())] =
                File{importp->second, path, missing};
        // TODO: detect duplicate entries
    }
}

Dir_Record::Dir_Record(Filesystem::path dir, Symbol_Map<File> fields)
:
    Record(Ref_Value::sty_dir_record),
    dir_(dir),
    fields_(fields)
{
}

void Dir_Record::print_repr(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (auto& f : fields_) {
        if (!first) out << ",";
        first = false;
        out << f.first;
    }
    out << "}";
}

Value Dir_Record::find_field(Symbol_Ref sym, const Context& cx) const
{
    auto p = fields_.find(sym);
    if (p == fields_.end())
        return missing;
    if (p->second.value_.is_missing())
        p->second.value_ =
            import_value(p->second.importer_, p->second.path_, cx);
    return p->second.value_;
}

bool Dir_Record::hasfield(Symbol_Ref sym) const
{
    return fields_.find(sym) != fields_.end();
}

size_t Dir_Record::size() const
{
    return fields_.size();
}

Shared<Record>
Dir_Record::clone() const
{
    return make<Dir_Record>(dir_, fields_);
}

Value*
Dir_Record::ref_field(Symbol_Ref name, bool need_value, const Context& cx)
{
    auto p = fields_.find(name);
    if (p == fields_.end()) {
        throw Exception(cx, stringify(Value{share(*this)},
            " has no field named ", name));
    }
    if (p->second.value_.is_missing()) {
        if (need_value)
            p->second.value_ =
                import_value(p->second.importer_, p->second.path_, cx);
    }
    return &p->second.value_;
}

/*
class Dir_Record::Iter : public Record::Iter
{
protected:
    Dir_Record& rec_;
    Symbol_Map<File>::const_iterator i_;
};
*/

Dir_Record::Iter::Iter(const Dir_Record& rec)
:
    rec_{rec},
    i_{rec.fields_.begin()}
{
    if (i_ != rec_.fields_.end()) {
        key_ = i_->first;
        value_ = i_->second.value_;
    }
}

void Dir_Record::Iter::load_value(const Context& cx)
{
    if (i_ != rec_.fields_.end()) {
        if (i_->second.value_.is_missing())
            i_->second.value_ =
                import_value(i_->second.importer_, i_->second.path_, cx);
        value_ = i_->second.value_;
    }
}

void Dir_Record::Iter::next()
{
    ++i_;
    if (i_ != rec_.fields_.end()) {
        key_ = i_->first;
        value_ = i_->second.value_;
    } else
        key_ = Symbol_Ref();
}

std::unique_ptr<Record::Iter> Dir_Record::iter() const
{
    return std::make_unique<Dir_Record::Iter>(*this);
}

}
