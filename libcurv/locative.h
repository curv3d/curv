// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LOCATIVE_H
#define LIBCURV_LOCATIVE_H

#include <libcurv/value.h>
#include <libcurv/phrase.h>
#include <libcurv/frame.h>
#include <libcurv/sc_frame.h>
#include <libcurv/sc_type.h>

namespace curv {

// A Locative is the phrase on the left side of an assignment statement.
//
// TODO: Efficient indexed update using "linear logic".
// For indexed update, we move the value out of the location (without changing
// its reference count), use COW to amend the moved value, then store().
// We need a 'steal' or 'fetch_move' operation.
struct Locative : public Shared_Base
{
    Shared<const Phrase> syntax_;
    Locative(Shared<const Phrase> syntax)
    :
        syntax_(std::move(syntax))
    {}

    virtual Value fetch(Frame&) const = 0;
    virtual void store(Frame&, Value) const = 0;
    virtual SC_Type sc_type(SC_Frame&) const { return {}; }
    virtual void sc_print(SC_Frame& f) const;
};

// A Locative representing a boxed local variable.
// Closely related to Local_Data_Ref.
struct Local_Locative : public Locative
{
    Local_Locative(Shared<const Phrase> syntax, slot_t slot)
    :
        Locative(std::move(syntax)),
        slot_(slot)
    {}
    slot_t slot_;
    virtual Value fetch(Frame&) const override;
    virtual void store(Frame&, Value) const override;
    virtual void sc_print(SC_Frame& f) const override;
    virtual SC_Type sc_type(SC_Frame&) const override;
};

// a Reference is an abstract pointer to a value or a structured collection
// of elements within a mutable local variable. It is the 'pointer' version
// of an index value. A Reference is only valid for a specific stack frame.
struct Reference
{
    virtual Value fetch() const = 0;
    virtual void store(Value) const = 0;
};

} // namespace curv
#endif // header guard
