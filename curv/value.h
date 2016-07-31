// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_VALUE_H
#define CURV_VALUE_H

#include <aux/shared.h>
#include <ostream>

namespace curv {

/// Base class for the object referenced by a curv reference value.
///
/// The memory layout for Ref_Value is:
/// * vtable_pointer -- 64 bits
/// * use_count -- 32 bits
/// * type -- 32 bits
///
/// The next data member has 128 bit alignment with no hole on 64 bit platforms.
///
/// The type_ field enables us to query the type by loading the first 128 bits
/// of the object into a cache line, without indirecting through the vtable,
/// which would cost a 2nd cache line hit. Note, there are an extra 3 bits in
/// curv::Value where I could store a type code, but that adds complexity,
/// and I doubt it increases performance, since you almost certainly have to
/// load the object into a cache line anyway to bump the use_count. Putting a
/// type code here lets users add lots of new types without messing with the
/// black magic that is curv::Value.
///
/// All Ref_Values must be allocated on the heap: see aux::Shared_Base.
struct Ref_Value : public aux::Shared_Base
{
    uint32_t type_;
    enum {
        ty_string,
        ty_list,
        ty_function,
        ty_object
    };
    Ref_Value(int type) : aux::Shared_Base(), type_(type) {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const = 0;
};

/// A boxed, dynamically typed value in the Curv runtime.
///
/// A Value is 64 bits. 64 bit IEEE floats are represented as themselves
/// (same bit pattern), while all other types are represented as NaNs,
/// with the data stored in the low order 48 bits of the NaN. This is called
/// "NaN boxing", a technique also used by LuaJIT and JavaScriptCore.
///
/// Null, Boolean and Number values are "immediate" values, stored entirely
/// in the 64 bit pattern of a Value. There are 3 special immediate values
/// which aren't numbers: k_null, k_false and k_true.
///
/// String, List, Object and Function values are "reference" values:
/// a Ref_Value* pointer is stored in the low order 48 bits of the Value.
/// This works on 64 bit Intel and ARM systems because those architectures
/// use 48 bit virtual addresses, with the upper 16 bits of a 64 bit pointer
/// being wasted space.
///
/// Reference values have Shared_Ptr semantics. The copy constructor increments
/// the reference count, the destructor decrements the reference count and
/// deletes the object if the refcount reaches 0.
///
/// Each Value has a unique bit pattern (not a given: I'm forcing the values
/// of unused bits in the NaN box to ensure this). Only positive NaNs are used.
/// This speeds up is_ref() and some equality tests.
class Value final
{
private:
    // internal representation
    union {
        double number_;
        uint64_t bits_;
        int64_t signed_bits_;
    };

    // This asserts that the binary encodings for quiet and signaling NaNs
    // are as specified in IEEE 754-2008. Will fail on PA-RISC and MIPS.
    #if defined __GNUC__
    static_assert(__GCC_IEC_559 == 2, "IEEE 754-2008 floating point is required");
    #endif

    // A double whose upper 16 bits is 0x7FFF is a quiet NaN on both Intel
    // and ARM. The low order 48 bits can store arbitrary data which the FPU
    // will ignore. (On PA-RISC and MIPS we'd use 0x7FF7 for a quiet NaN, but
    // we don't currently support those architectures.)
    static constexpr uint64_t k_nanbits = 0x7FFF'0000'0000'0000;
    // The null value has a 48 bit payload of 0, same representation as the
    // null pointer. So Value((Ref_Value*)0) is null.
    static constexpr uint64_t k_nullbits = k_nanbits|0;
    // false and true have a payload of 2 and 3. The '2' bit is only set in the
    // payload for a boolean value (assuming all Ref_Value* pointers have
    // at least 4 byte alignment). The '1' bit is 0 or 1 for false and true.
    static constexpr uint64_t k_boolbits = k_nanbits|2;
    static constexpr uint64_t k_boolmask = 0xFFFF'FFFF'FFFF'FFFE;

    // Note: the corresponding public constructor takes a Shared_Ptr argument.
    inline Value(const Ref_Value* r)
    {
        #if UINTPTR_MAX == UINT64_MAX
            // 64 bit pointers
            bits_ = ((uint64_t)r & 0x0000'FFFF'FFFF'FFFF) | Value::k_nanbits;
        #elif UINTPTR_MAX == UINT32_MAX
            // 32 bit pointers
            bits_ = (uint64_t)(uint32_t)r | Value::k_nanbits;
        #else
            static_assert(false, "only 32 and 64 bit architectures supported");
        #endif
    }
    template<class T, class... Args> friend Value make_ref_value(Args&&...);
public:
    /// Construct the `null` value (default constructor).
    ///
    /// This is the `null` value in Curv.
    /// It corresponds to both NaN and `undef` in OpenSCAD.
    inline Value() noexcept
    {
        bits_ = k_nullbits;
    }

    /// True if value is null.
    inline bool is_null() const
    {
        return bits_ == k_nullbits;
    }

    /// Construct a boolean value.
    inline Value(bool b)
    {
        bits_ = k_boolbits|(uint64_t)b;
    }

    /// True if the value is boolean.
    inline bool is_bool() const
    {
        return (bits_ & k_boolmask) == k_boolbits;
    }
    /// Convert a boolean value to `bool`.
    ///
    /// Only defined if is_bool() is true.
    inline bool get_bool_unsafe() const
    {
        return (bool)(bits_ & 1);
    }

    /// Construct a number value.
    ///
    /// The Curv Number type includes all of the IEEE 64 bit floating point
    /// values except for NaN.
    /// If the argument is NaN, construct the null value.
    inline Value(double n)
    {
        if (n == n)
            number_ = n;
        else
            bits_ = k_nullbits;
    }

    /// True if the value is a number.
    inline bool is_num() const
    {
        return number_ == number_;
    }

    /// Convert a number value to `double`.
    ///
    /// Only defined if `is_num()` is true.
    /// Potentially faster than `get_num_or_nan()`,
    /// so call this version when guarded by `if(v.is_num())`.
    inline double get_num_unsafe() const
    {
        return number_;
    }

    /// Convert a number value to `double`.
    ///
    /// If is_num() is false then NaN is returned.
    inline double get_num_or_nan() const
    {
        return number_;
    }

    /// Construct a reference value.
    ///
    /// If the argument is nullptr, construct the null value.
    inline Value(aux::Shared_Ptr<Ref_Value> ptr) : Value(ptr.detach())
    {
    }

    /// True if the value is a reference value.
    inline bool is_ref() const noexcept
    {
        // Negative numbers will have the sign bit set, which means
        // signed_bits_ < 0. Positive infinity has all 1s in the exponent,
        // and is 0x7FF0'0000'0000'0000. Positive numbers have a smaller
        // exponent than this, so will test < +inf as an integer bit pattern.
        // The positive NaNs have the largest signed integer magnitude,
        // and all non-numeric values are encoded as positive NaNs.
        // The 4 special immediate values, void, null, false and true,
        // are encoded like pointer values in the range 0...3.
        return signed_bits_ > (int64_t)(k_nanbits|3);
    }

    /// Convert a reference value to `Ref_Value&`.
    ///
    /// Unsafe unless `is_ref()` is true.
    inline Ref_Value& get_ref_unsafe() const noexcept
    {
        #if UINTPTR_MAX == UINT64_MAX
            // 64 bit pointers.

            // Intel: "bits 48 through 63 of any virtual address must be copies
            // of bit 47 (in a manner akin to sign extension), or the processor
            // will raise an exception." https://en.wikipedia.org/wiki/X86-64

            // Arm-64: also has 48 bit virtual addresses, and also seems to
            // require the sign extension logic. "AArch64 features two 48-bit
            // virtual address spaces, one for the kernel and one for
            // applications. Application addressing starts at 0 and grows
            // upwards, while kernel space grows down from 2^64; any references
            // to unmapped addresses in between will trigger a fault. Pointers
            // are sign extended to 64-bits, and can optionally be configured
            // to use the upper 8-bits for tagging pointers with additional
            // information." http://www.realworldtech.com/arm64/4/

            // This sign extension may be unnecessary. Ref_Values must be
            // allocated on the heap, and it may be that heap pointers are
            // positive on all supported platforms. LuaJIT and SpiderMonkey
            // both assume 47 bit pointers (not 48) in their nan boxes.
            return *(Ref_Value*)((signed_bits_ << 16) >> 16);
        #elif UINTPTR_MAX == UINT32_MAX
            // 32 bit pointers
            return *(Ref_Value*)(uint32_t)bits_;
        #else
            static_assert(false, "only 32 and 64 bit architectures supported");
        #endif
    }

    /// The copy constructor increments the use_count of a ref value.
    inline Value(const Value& val) noexcept
    {
        bits_ = val.bits_;
        if (is_ref())
            aux::intrusive_ptr_add_ref(&get_ref_unsafe());
    }

    /// The move constructor.
    inline Value(Value&& val) noexcept
    {
        bits_ = val.bits_;
        val.bits_ = k_nullbits;
    }

    /// The assignment operator.
    ///
    /// There's just one assignment operator, and it's by-value.
    /// This is simpler than `const Value&` and `const Value&&`
    /// copy and move assignments, and allegedly the most efficient.
    /// When possible, use `val = std::move(rhs);` for a non-rvalue rhs
    /// for efficiency.
    inline Value& operator=(Value rhs) noexcept
    {
        rhs.swap(*this);
        return *this;
    }

    /// The swap operation.
    void swap(Value& rhs) noexcept
    {
        auto tmpbits = bits_;
        bits_ = rhs.bits_;
        rhs.bits_ = tmpbits;
    }

    /// The destructor.
    ~Value()
    {
        if (is_ref())
            aux::intrusive_ptr_release(&get_ref_unsafe());
    }

    /// Print a value like a Curv expression.
    void print(std::ostream&) const;
};

inline std::ostream&
operator<<(std::ostream& out, Value val)
{
    val.print(out);
    return out;
}

/// Allocate a reference value with given class and constructor arguments.
template<typename T, class... Args> Value make_ref_value(Args&&... args)
{
    T* ptr = new T(args...);
    aux::intrusive_ptr_add_ref(ptr);
    return Value(ptr);
}

} // namespace curv
#endif // header guard
