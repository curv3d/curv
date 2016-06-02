// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_MEANING_H
#define CURV_MEANING_H

/*
How do I represent operations and functions? Eg, the binary + operation
as in 'x+y'?
 1. I could create a distinct subclass of Expression for each built-in operator.
    Eg, Binary_Plus_Expression. So how is sin(x) represented, and do I use
    different representations based on syntax? One difference is that 'sin'
    is a value in TeaCAD, whereas I've made no commitment that there is a '+'
    function in the builtin namespace.
 2. General purpose function application node, with a function and a
    positional argument list. (No keyword arguments yet.) Works for x+y, sin(x),
    etc (code reuse).

Does the semantic analyzer use multiple passes? Is different information stored
in the meaning tree depending on the pass? AMI had a single bottom up semantic
analyzer (including type checking). That will currently work here, for now.
But Curv may evolve to be more complicated.
 1. bottom up type checking is fine for now.
 2. identifier resolution: we support unrestricted forward references in
    scripts.
     * First pass: parse the script. Each top level entry is a definition,
       or an 'action': expression, for loop, echo, assert, etc.
       Put the definitions into a Namespace mapping name to Phrase.
       Put the action phrases into an ordered list.
     * Second pass: during evaluation, when a name is looked up,
       JIT compile the phrase into an Expression, stuff the Expression
       back into the namespace, then evaluate the expression.

How to evaluate a script:
 1. Parse the script into a Script_Phrase object.
    This contains: a namespace mapping a name to a Phrase or Expression,
    and a list of action phrases. Due to JIT, we don't have a clear
    distinction phase distinction: a Script is both a Phrase and a Meaning.
    Duplicate definition is a parse time error.
 2. Evaluate the script by evaluating each action, with the following effects:
     * partially JIT compile the Script
     * produce a list of values
     * cause side effects (echo and assert)
    An action has a side effect: it 
How to evaluate a command line:
 1. Parse it into a definition or action (including echo/assert/for).
 2. Evaluate the phrase. Action is evaluated immediately: each time a list
    value is generated, it is printed to console. 'generate_item' is virtual?
    Definition means update the console Namespace.
     * How are references resolved?
        * References are relative to the console namespace before it is updated.
          No recursive definitions. Existing definitions don't change their
          behaviour when an existing definition is overridden.
          Compile the definiens *before* updating the console namespace.
          x=x+1 increments x, an anomaly.
        * References are relative to the new namespace. Recursive defs allowed.
          Existing definitions must be recompiled if one of their dependencies
          is overridden. Harder to implement, and, it's not that important
          right now.
       Either way, compile the definiens now and show compile errors.
       (But, this may prevent mutual recursion in #2: reference to a definition
       that hasn't been entered yet. Or maybe that's not supported?
       In #2, maybe undefined reference is just a warning? Too hard for now.)

Maybe a Script exists in all 3 phases: parse, analyze and run time?
It has a value list, which may be just parsed (only a list of action phrases)
or evaluated (list of values available). It has a namespace, which may contain
parsed definiens and compiled definiens.

But, a Script may have non-local references to parameters. So there are
multiple distinct values generated from a single script phrase.
So, a Script_Value (aka Object) contains a list of Values, and a reference
to a namespace, which may include unanalyzed phrases?

But, it's more complicated. A definiens may be simple: not a function literal,
not a thunk. In that case, it makes sense to fully evalute it to a value,
taking into account external parameters, and storing the result in the Object.
 * Fine, but let's make a distinction between top level scripts that don't
   have external parameters and are partially JIT compiled, vs internal
   scripts that are compiled all-or-nothing. When an internal script is
   compiled, we determine the number of value slots it needs for its
   instances.

A Top_Level_Script exists across all 3 phases: parse, analyze and run.
 * It's a Ref_Value. It's a Meaning (or encapsulated as a Constant).
   It's a phrase (but maybe not a curv::Phrase, as it doesn't appear as an
   interior node of a parse tree).
It has a value list, which may be unanalyzed (only a list of action phrases)
or evaluated (list of values available). It has a namespace, which may contain
both unanalyzed and compiled definiens.

An internal script is either unanalyzed or fully compiled, all or nothing,
existing in either the parse or the analyze phases.
It may have multiple instances at runtime with different contents, depending
on (eg) the values of external function parameters.
 */

namespace curv {

/// An abstract base class representing a semantically analyzed Phrase.
struct Meaning : public aux::Shared_Base
{
    //virtual ~Meaning() {}
    //virtual Location location() const = 0;
};

/// A Bindable phrase denotes an entity that can be bound to a name.
///
/// But the entity might not be a run-time value, in which case the phrase
/// is not an Expression. Bindable is a supertype of Expression.
///
/// Bindable is used to represent compile time entities that are function-like
/// (can be invoked using function call syntax) or namespace-like
/// (can use '.' notation to reference members), but which aren't
/// run-time values.
struct Bindable : public Meaning
{
};

/// An Expression is a phrase that denotes a value.
struct Expression : public Meaning
{
};

/// A Constant is an Expression whose value is known at compile time.
struct Constant : public Expression
{
    Value value_;
};

struct Definition : public Meaning
{
    std::string name_;
    Shared_Ptr<Expression> value_;
};

} // namespace curv
#endif // header guard
