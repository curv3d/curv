// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/function.h>
#include <libcurv/meaning.h>
#include <boost/functional/hash.hpp>

namespace curv
{

size_t Operation::hash() const noexcept
{
    return size_t(this);
}
bool Operation::hash_eq(const Operation& rhs) const noexcept
{
    return this == &rhs;
}

size_t Prefix_Expr_Base::hash() const noexcept
{
    size_t result = typeid(*this).hash_code();
    boost::hash_combine(result, arg_->hash());
    return result;
}
bool Prefix_Expr_Base::hash_eq(const Operation& rhs) const noexcept
{
    if (typeid(rhs) == typeid(*this)) {
        auto& r = dynamic_cast<const Prefix_Expr_Base&>(rhs);
        return arg_->hash_eq(*r.arg_);
    }
    return false;
}

#define GEN_BINARY_HASH(Class) \
size_t Class::hash() const noexcept \
{ \
    size_t result = arg1_->hash(); \
    boost::hash_combine(result, arg2_->hash()); \
    boost::hash_combine(result, #Class); \
    return result; \
} \
bool Class::hash_eq(const Operation& rhs) const noexcept \
{ \
    auto r = dynamic_cast<const Class*>(&rhs); \
    if (r) \
        return arg1_->hash_eq(*r->arg1_) \
            && arg2_->hash_eq(*r->arg2_); \
    return false; \
}

#define arg1_ func_
#define arg2_ arg_
GEN_BINARY_HASH(Call_Expr)
#undef arg1_
#undef arg2_

#define GEN_TERNARY_HASH(Class) \
size_t Class::hash() const noexcept \
{ \
    size_t result = arg1_->hash(); \
    boost::hash_combine(result, arg2_->hash()); \
    boost::hash_combine(result, arg3_->hash()); \
    boost::hash_combine(result, #Class); \
    return result; \
} \
bool Class::hash_eq(const Operation& rhs) const noexcept \
{ \
    auto r = dynamic_cast<const Class*>(&rhs); \
    if (r) \
        return arg1_->hash_eq(*r->arg1_) \
            && arg2_->hash_eq(*r->arg2_) \
            && arg3_->hash_eq(*r->arg3_); \
    return false; \
}

GEN_TERNARY_HASH(If_Else_Op)

size_t Constant::hash() const noexcept
{
    size_t result = value_.hash();
    boost::hash_combine(result, "Constant");
    return result;
}
bool Constant::hash_eq(const Operation& rhs) const noexcept
{
    auto r = dynamic_cast<const Constant*>(&rhs);
    if (r)
        return value_.hash_eq(r->value_);
    return false;
}

size_t Local_Data_Ref::hash() const noexcept
{
    size_t result = slot_;
    boost::hash_combine(result, "Local_Data_Ref");
    return result;
}
bool Local_Data_Ref::hash_eq(const Operation& rhs) const noexcept
{
    auto r = dynamic_cast<const Local_Data_Ref*>(&rhs);
    if (r)
        return slot_ == r->slot_;
    return false;
}

size_t List_Expr_Base::hash() const noexcept
{
    std::hash<const char*> hasher;
    size_t result = hasher("List");
    for (size_t i = 0; i < size_; ++i)
        boost::hash_combine(result, array_[i]->hash());
    return result;
}
bool List_Expr_Base::hash_eq(const Operation& rhs) const noexcept
{
    auto r = dynamic_cast<const List_Expr_Base*>(&rhs);
    if (r) {
        if (size_ != r->size_) return false;
        for (size_t i = 0; i < size_; ++i) {
            if (!array_[i]->hash_eq(*r->array_[i]))
                return false;
        }
        return true;
    }
    return false;
}

} // namespace curv
