// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_DIR_RECORD_H
#define LIBCURV_DIR_RECORD_H

#include <libcurv/filesystem.h>
#include <libcurv/record.h>
#include <libcurv/symbol.h>
#include <libcurv/system.h>

namespace curv {

struct Dir_Record : public Record
{
    Filesystem::path dir_;
    struct File
    {
        System::Importer importer_;
        Filesystem::path path_;
        mutable Value value_;
    };
    Symbol_Map<File> fields_;

    Dir_Record(Filesystem::path dir, const Context&);

    virtual void print(std::ostream&) const override;
    virtual Value find_field(Symbol_Ref, const Context&) const override;
    virtual bool hasfield(Symbol_Ref) const override;
    virtual size_t size() const override;
    class Iter : public Record::Iter
    {
    protected:
        const Dir_Record& rec_;
        Symbol_Map<File>::const_iterator i_;
        virtual void load_value(const Context&) override;
    public:
        Iter(const Dir_Record& rec);
        virtual void next() override;
    };
    virtual std::unique_ptr<Record::Iter> iter() const override;
};

}
#endif
