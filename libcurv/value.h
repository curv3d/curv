// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_VALUE_H
#define LIBCURV_VALUE_H

#include <libcurv/shared.h>
#include <ostream>

namespace curv {

union Value;
struct Symbol;
struct Context;

/// Base class for the object referenced by a curv reference value.
///
/// The memory layout for Ref_Value is:
/// * vtable_pointer -- 64 bits
/// * use_count -- 32 bits
/// * type -- 32 bits
///
/// The next data member has 128 bit alignment with no hole on 64 bit platforms.
///
/// The type_ and subtype_ fields enable us to query the type by loading the
/// first 128 bits of the object into a cache line, without indirecting through
/// the vtable, which would cost a 2nd cache line hit and either a virtual
/// function call or an RTTI lookup, both relatively costly.
///
/// Why not put the type code into Value? Right now, there are an extra 3 bits
/// in curv::Value where I could store a type code, similar to LuaJIT and most
/// Javascript VMs. I chose not to do this, for simplicity, and to leave room
/// for updating later to support 52 bit pointers.
///
/// All Ref_Values must be allocated on the heap: see Shared_Base.
struct Ref_Value : public Shared_Base
{
    uint16_t type_;
    uint16_t subtype_;
    enum {
        ty_string,
        ty_list,
        ty_record,
            sty_drecord,
            sty_module,
            sty_dir_record,
        ty_function,
        ty_missing,
        ty_lambda,
        ty_reactive,
            sty_uniform_variable,
            sty_reactive_expression
    };
    Ref_Value(int type) : Shared_Base(), type_(type), subtype_(type) {}
    Ref_Value(int type, int subtype)
    :
        Shared_Base(), type_(type), subtype_(subtype)
    {}

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
/// Since this code was written, ARM and Intel have added support for pointers
/// with more than 48 significant bits. The Linux kernel does not enable this
/// feature by default, because it breaks all of the modern web browsers,
/// which use NaN boxing. So my implementation continues to be viable, for now.
/// I could upgrade my Nan boxing implementation to support 52 bit pointers,
/// but it's not clear if that would actually help anybody.
///
/// Reference values have Shared semantics. The copy constructor increments
/// the reference count, the destructor decrements the reference count and
/// deletes the object if the refcount reaches 0.
///
/// Each Value has a unique bit pattern (not a given: I'm forcing the values
/// of unused bits in the NaN box to ensure this). Only positive NaNs are used.
/// This speeds up is_ref() and some equality tests.
union Value
{
private:
    // internal representation
    double number_;
    uint64_t bits_;
    int64_t signed_bits_;

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

    // Note: the corresponding public constructor takes a Shared argument.
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
    inline constexpr Value() noexcept : bits_{k_nullbits} {}

    /// True if value is null.
    inline bool is_null() const noexcept
    {
        return bits_ == k_nullbits;
    }
    /// Abort if value is not null.
    void to_null(const Context& cx) const;

    /// Construct a boolean value.
    inline constexpr Value(bool b) : bits_{k_boolbits|(uint64_t)b} {}

    /// True if the value is boolean.
    inline bool is_bool() const noexcept
    {
        return (bits_ & k_boolmask) == k_boolbits;
    }
    /// Convert a boolean value to `bool`.
    ///
    /// Only defined if is_bool() is true.
    inline bool get_bool_unsafe() const noexcept
    {
        return (bool)(bits_ & 1);
    }
    /// Convert a Value to `bool`, throw an exception if wrong type.
    bool to_bool(const Context&) const;

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
    inline bool is_num() const noexcept
    {
        return number_ == number_;
    }

    bool is_int() const noexcept;

    /// Convert a number value to `double`.
    ///
    /// Only defined if `is_num()` is true.
    /// Potentially faster than `get_num_or_nan()`,
    /// so call this version when guarded by `if(v.is_num())`.
    inline double get_num_unsafe() const noexcept
    {
        return number_;
    }

    /// Convert a number value to `double`.
    ///
    /// If is_num() is false then NaN is returned.
    inline double get_num_or_nan() const noexcept
    {
        return number_;
    }

    /// Convert a Value to `double`, throw an exception if wrong type.
    double to_num(const Context&) const;

    // Convert a Value to `int`.
    // Throw an exception if wrong type or out of range.
    int to_int(int lo, int hi, const Context&) const;

    /// Construct a reference value.
    ///
    /// If the argument is nullptr, construct the null value.
    inline Value(Shared<Ref_Value> ptr) : Value(ptr.detach())
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
        // The 3 special immediate values (null, false and true)
        // are encoded like pointer values in the range 0...3.
        return signed_bits_ > (int64_t)(k_nanbits|3);
    }

    /// Convert a reference value to `Ref_Value&`.
    ///
    /// Unsafe unless `is_ref()` is true, otherwise it returns a bad reference,
    /// causing memory corruption or a crash if dereferenced.
    /// Another reason it's unsafe is that the returned reference becomes
    /// invalid if the original Value is destroyed.
    /// See `dycast` for a completely safe alternative.
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

            // The following code truncates to 48 bits, then fills the upper
            // 16 bits using sign extension. Is this sign extension necessary?
            // LuaJIT and SpiderMonkey both assume 47 bit pointers (not 48)
            // in their NaN boxes. They assume all pointers are positive.
            //
            // The sign extension is necessary. Here's a LuaJIT bug where
            // a negative 48 bit pointer on ARM64 leads to a crash.
            // https://github.com/LuaJIT/LuaJIT/issues/49
            //
            // ARMv8.2 will support 52 bit virtual addresses.
            // That can still work (a NaN has a 52 bit payload). We'll need
            // changes to the Value internals but the API won't change.
            // I don't store type codes in the NaN box (unlike LuaJIT)
            // which will make this change easier.
            return *(Ref_Value*)((signed_bits_ << 16) >> 16);
        #elif UINTPTR_MAX == UINT32_MAX
            // 32 bit pointers
            return *(Ref_Value*)(uint32_t)bits_;
        #else
            static_assert(false, "only 32 and 64 bit architectures supported");
        #endif
    }

    /// Like dynamic_cast for a Value.
    template <class T>
    inline Shared<T> dycast() const noexcept
    {
        if (is_ref()) {
            T* p = dynamic_cast<T*>(&get_ref_unsafe());
            if (p != nullptr)
                return share(*p);
        }
        return nullptr;
    }
    template <class T>
    inline Shared<T> to(const Context& cx)
    {
        if (is_ref()) {
            T* p = dynamic_cast<T*>(&get_ref_unsafe());
            if (p != nullptr)
                return share(*p);
        }
        to_abort(cx, T::name);
    }
    void to_abort [[noreturn]] (const Context&, const char*);

    Value at(Symbol fieldname, const Context& cx) const;

    /// The copy constructor increments the use_count of a ref value.
    inline Value(const Value& val) noexcept
    {
        bits_ = val.bits_;
        if (is_ref())
            intrusive_ptr_add_ref(&get_ref_unsafe());
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
            intrusive_ptr_release(&get_ref_unsafe());
    }

    /// Print a value like a Curv expression.
    void print(std::ostream&) const;

    // Deep equality, which traverses the value tree and forces thunks.
    // May throw an Exception if forcing a thunk fails.
    // Used to implement `a == b` in the Curv language.
    // Optimized for numbers, may be expensive for lists and records.
    bool equal(Value, const Context&) const;

    // Shallow equality, compares the bit patterns of two Values. Fast.
    bool eq(Value rhs) const
    {
        return bits_ == rhs.bits_;
    }

    size_t hash() const noexcept;
    bool hash_eq(Value) const noexcept;

    struct Hash
    {
        size_t operator()(Value val) const noexcept
        {
            return val.hash();
        }
    };
    struct Hash_Eq
    {
        bool operator()(Value v1, Value v2) const noexcept
        {
            return v1.hash_eq(v2);
        }
    };
};

/// Special marker that denotes the absence of a value
extern Value missing;

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
    intrusive_ptr_add_ref(ptr);
    return Value(ptr);
}

} // namespace curv
#endif // header guard
