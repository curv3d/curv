// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_CONVAL_H
#define LIBCURV_CONVAL_H

#include <ostream>

namespace curv {

// In the Curv language, a type denotes a set of values, plus an efficient
// unboxed representation for variables that contain only those values.
// In libcurv, a Concrete Value Class (CVC) corresponds directly to some Curv
// type expression, and implements that "efficient unboxed representation"
// for that set of values.
//
// A CVC is "concrete" because CVC instances do not contain Reactive values.
// We will also need Generic Value Classes.
//
// All CVCs implement a standard, uniform interface, so that they can be
// composed using generic programming. CVCs have both Curv and C++ value
// semantics. They implement standard C++ (STL) interfaces and they implement
// Curv semantics. CVC instances are passed by value (and use a Shared<T>
// representation internally for large values).
//
// CVCs are used in libcurv for implementing Curv primitives, using types and
// operations that are as close as possible to Curv types and operations.
// In this role, the goal of CVCs are to make libcurv code compact, readable
// and high level, avoiding boilerplate.
//
// For the most part, the CVC interface can't be described using an abstract
// base class. It would be better described by a concept, but C++17 doesn't
// have concepts. What follows is the best I can do in C++17.

// the Concrete Value Class interface
// ----------------------------------
// Naming convention: C<CurvTypeName>. Eg, CNum, CBool, CString.
//
// Constructors, that take the corresponding C++ native type as an argument,
// where applicable. Eg,
//     CNum(double)
//     CString(const std::string&)
//
// If the corresponding Curv type has an algebra with constructor functions,
// then implement these Curv constructors as C++ constructors. Eg,
//     CSymbol(CString)
//
// A CVC has a special value which denotes the absence of a value.
// The default constructor creates this 'missing' value.
// This is because std:optional doubles the size of most CVCs.
// In the future, I might create a curv::Optional template based
// on https://github.com/akrzemi1/markable, for use with CVCs.
//
// operator bool returns false if the value is missing.
// This is for compatibility with C++ stdlib conventions and the TRY_DEF macro.
//     explicit operator bool() const noexcept { ... }
// (Note: potential confusion when using CBool. Use is_missing() instead?)
//
// Equality: a==b has Curv equality semantics.
// Note: Traversing a directory module requires a context, but in Curv.next,
// equality of directory modules doesn't require traversal. Instead, we compare
// if directory pathnames are equal. So equality doesn't require a context.
//
// Conversion to Value:
//     Value to_value() const
//
// Conversions from Value, using missing value to denote soft failure.
// This is where I could use curv::Optional.
//     CNum(Value, const At_Syntax&) -- fail hard
//     static CNum from_value(Value, const At_Syntax&) -- fail soft
//     static CNum from_value(Value, Fail, const At_Syntax&) -- fail soft|hard
//
// Printing values:
//     void print_repr(std::ostream&) const
//     void print_string(std::ostream&) const
//     friend std::ostream& operator<<(std::ostream&& o, CNum n)
//       { n.print_string(o); return o; }
// Following Curv semantics,
//     `ostream << val` uses print_string.
//     `ostream << repr(val)` uses print_repr.
// TODO: Enhance print_repr to use operator precedence to minimize parens.
// TODO: add `repr`.
//
// TODO: Hash
//     size_t hash() const noexcept;

// This is the common superclass of Concrete Value Classes.
// I think it just serves as documentation. Can I use this for type checking?
// In theory, I could add pure virtual functions to provide compile time
// checks that (some) required member functions are present. But this would
// add a useless vtable that would double the size of the object.
class Concrete_Value { };

} // namespace curv
#endif // header guard
