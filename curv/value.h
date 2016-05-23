// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_VALUE_H
#define CURV_VALUE_H

namespace curv {

class Ref_Value;

/// A boxed, dynamically typed value in the Curv runtime.
///
/// A Value is 64 bits. 64 bit IEEE floats are represented as themselves
/// (same bit pattern), while all other types are represented as NaNs,
/// with the data stored in the low order 48 bits of the NaN. This is called
/// "NaN boxing", a technique also used by LuaJIT and JavaScriptCore.
///
/// The Null, Boolean and Number values are "immediate" values, stored
/// entirely in the 64 bit pattern of a Value. String, List, Object and Function
/// values are "reference" values: a Ref_Value* pointer is stored in the
/// low order 48 bits of the Value. This works on 64 bit Intel and ARM systems
/// because those architectures use 48 bit virtual addresses, with the upper
/// 16 bits of a 64 bit pointer being wasted space.
class Value
{
private:
    // internal representation
    union {
        double number_;
        uint64_t bits_;
        int64_t signed_bits_;
    };
    // A double whose upper 16 bits is 0x7FFF is a quiet NaN on both Intel
    // and ARM. The low order 48 bits can store arbitrary data which the FPU
    // will ignore. (On PA-RISC and MIPS we'd use 0x7FF7 for a quiet NaN, but
    // we don't currently support those architectures.)
    static constexpr uint64_t k_nanbits = 0x7FFF'0000'0000'0000;
    static constexpr uint64_t k_voidbits = k_nanbits|0;
    static constexpr uint64_t k_nullbits = k_nanbits|1;
    static constexpr uint64_t k_boolbits = k_nanbits|2;
    static constexpr uint64_t k_boolmask = 0xFFFF'FFFF'FFFF'FFFE;

    friend constexpr Value mk_void();
    friend constexpr Value mk_null();
    friend constexpr Value mk_bool(bool);
    friend constexpr Value mk_num(double);
    friend constexpr Value mk_ref(Ref_Value* r);
public:
    // 'k_void' is a special bit pattern that is used internally to denote
    // the absence of a Curv Value. It differs from 'k_null', a legal value.
    inline constexpr bool is_void() const
    {
        return bits_ == k_voidbits;
    }

    // k_null is the TeaCAD value 'null'. It corresponds to both NaN and undef
    // in OpenSCAD.
    inline constexpr bool is_null() const
    {
        return bits_ == k_nullbits;
    }

    // The boolean values 'true' and 'false'.
    inline constexpr bool is_bool() const
    {
        return (bits_ & k_boolmask) == k_boolbits;
    }
    // only defined if is_bool() is true
    inline constexpr bool get_bool_unsafe() const
    {
        return (bool)(bits_ & 1);
    }

    // The Curv Number type includes all of the IEEE 64 bit float values
    // except for the NaNs. (+inf and -inf are included as Numbers.)

    inline constexpr bool is_num() const
    {
        return number_ == number_;
    }
    // only defined if is_num() is true
    inline constexpr double get_num_unsafe() const
    {
        return number_;
    }

    inline constexpr bool is_ref() const
    {
        // Negative numbers will have the sign bit set, which means
        // signed_bits_ < 0. Positive infinity has all 1s in the exponent,
        // and is 0x7FF0'0000'0000'0000. Positive numbers have a smaller
        // exponent than this, so will test < +inf as an integer bit pattern.
        // The positive NaNs have the largest signed integer magnitude,
        // and all non-numeric values are encoded as positive NaNs.
        // The 4 special immediate values, void, null, false and true,
        // are encoded like pointer values in the range 0...3.
        return signed_bits_ > (k_nanbits|3);
    }
    inline constexpr Ref_Value* get_ref_unsafe() const
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
            return (Ref_Value*)((signed_bits_ << 16) >> 16);
        #elif UINTPTR_MAX == UINT32_MAX
            // 32 bit pointers
            return (Ref_Value*)(uint32_t)bits_;
        #else
            static_assert(false, "only 32 and 64 bit architectures supported");
        #endif
    }
};

inline constexpr Value mk_void()
{
    Value v;
    v.bits_ = Value::k_voidbits;
    return v;
}
constexpr Value k_void = mk_void();

inline constexpr Value mk_null()
{
    Value v;
    v.bits_ = Value::k_nullbits;
    return v;
}
constexpr Value k_null = mk_null();

inline constexpr Value mk_bool(bool b)
{
    Value v;
    v.bits_ = Value::k_boolbits|(uint64_t)b;
    return v;
}
constexpr Value k_false = mk_bool(false);
constexpr Value k_true = mk_bool(true);

/// mk_num doesn't assert that its argument is not a NaN.
/// That's expensive. Instead, if the argument is a machine generated NaN,
/// then the result is k_null.
inline constexpr Value mk_num(double n)
{
    Value v;
    if (n == n)
        v.number_ = n;
    else
        v.bits_ = Value::k_nullbits;
    return v;
}

inline constexpr Value mk_ref(Ref_Value* r)
{
    Value v;
    #if UINTPTR_MAX == UINT64_MAX
        // 64 bit pointers
        v.bits_ = ((uint64_t)r & 0x0000'FFFF'FFFF'FFFF) | Value::k_nanbits;
    #elif UINTPTR_MAX == UINT32_MAX
        // 32 bit pointers
        v.bits_ = (uint64_t)(uint32_t)r | Value::k_nanbits;
    #else
        static_assert(false, "only 32 and 64 bit architectures supported");
    #endif
    return v;
}

} // namespace curv
#endif // header guard
