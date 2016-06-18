# Object Oriented Programming in OpenSCAD2

Full blown OOP is overkill for OpenSCAD2. We don't need full support for
inheritance for the same reason we don't need user-defined data types.
It's a tar pit of weirdness and complexity, and there doesn't seem to be
a need for it. 

Although, when Runsun said he wanted to redefine the way builtin shape
primitives work, I told him to create a new library interface for basic
shape operations, using objects. That will push the object model quite far,
as it will now be used for OOP.

## Implementation Inheritance (and Super)
I would like to clarify the semantics of object specialization.
Because that comes tantalizingly close to support for
OOP implementation inheritance.

Given
  lollipop = { ... };

Then
  lollypop(a=expr)
will substitute the value of 'expr', resolved in the current lexical scope,
into lollipop as the binding for 'a'. All references within the lollipop
source code to 'a', both inside and outside of functions, will now resolve
to the value of 'expr'.

I'm thinking that
  include lollypop(a=expr);
could be quite similar to inheritance with override of virtual functions.
Suppose 'a' is a function. It can call another other function f within lollipop.
There are two choices if f happens to be overridden.
* Maybe we want to call the overridden version of f.
  So the call really looks like this:
    include lollypop(
      a=function(x) ... f(x) ...;
      f=...;
    );
* Maybe we want to call the original lollipop version of f,
  in which case we can call lollypop.f(). Will that work? What if lollypop.f()
  calls another function g which we are overriding? For proper OOP semantics,
  we'd want lollypop.f to call our g. So maybe we provide a 'super' binding,
  bound only in the argument list of lollypop(...).
    include lollypop(
      a=function(x) ... super.f(x) ...;
      f=...;
    );

This is too difficult for beginners to understand.
It really belongs in some appendix of advanced features.
Fortunately, unlike classes, you don't need to learn this arcane stuff
in order to customize objects in the normal use cases.

What I'm missing is a compelling example of why you would use 'super'
in OpenSCAD.

## Interface Inheritance (and isa)

Self doesn't seem to support 'isa'. But it has a very simple design for
delegation (which is multiple implementation inheritance).

Javascript has a very baroque and proveably wrong design for 'isa'.
Hint: the interface hierarchy is not identical to the implementation
inheritance hierarchy.

What's needed instead of the Javascript abomination is an orthogonal
concept of 'interface' objects. Something like this:
  name = interface (<interface-list>) {<identifier-list>};
  implement <interface>;
  <object> isa <interface>

If somebody wants 'isa' in OpenScAD, this is how I'll implement it.

## This/Self

Starting with Smalltalk, every OOP language has a 'this' or 'self' keyword.
But why do we need it in OpenSCAD? I don't have a reason for it yet.
* In Smalltalk, it is obviously needed because it's the only way for an
  object to send a message to itself. Not true for OpenSCAD objects.
* Maybe you need it so that a member function can pass 'this' as an argument
  to some function? Again, not very compelling, because you can use this
  idiom instead:
    this = {
      f = function(x) ... g(this) ...
    };

--------

OpenSCAD is a simple declarative language, with a much simpler conceptual
model than modern programming languages. It's designed to be used by
people with little or no programming experience, whose interest lies in
modelling 3D objects, not in acquiring computer programming skills.

When computer programmers first start using OpenSCAD, they recognize that it is,
in fact, a simple programming langauge, even though we don't describe it as
such. But, OpenSCAD is missing features that experienced programmers are used
to using. Should we extend OpenSCAD with these features?

I'll try to give a nuanced answer to this question.
 1. Modern programming languages have a much more complex conceptual structure
    than OpenSCAD. You have to learn many more concepts to use a programming
    language, than you need to use OpenSCAD. We do not want to make OpenSCAD
    fundamentally harder to use for non-programmers.
 2. There are several different types of modern programming languages.
    The features you think are missing from OpenSCAD may depend on what style
    of programming you have learned.
     * In the functional style, there is no state. You can't increment a
       variable or mutate a data structure. OpenSCAD has no state so it is
       actually a simple functional language. Functional programmers already
       know the idioms needed to define OpenSCAD functions, so they feel
       more at home in OpenSCAD than object-oriented programmers.
     * In the object-oriented style, computations are structured as a collection
       of stateful objects that send messages to one another. An object may
       change its state in response to a message. In class-based object
       oriented languages like Python or Java, every object is an instance
       of a class, and classes are the fundamental structure used for
       organizing programs. OO programmers feel less at home in OpenSCAD
       than functional programmers, since they don't know functional
       programming idioms, and the OO idioms they know don't apply.
       So a common reaction is that we should add classes to OpenSCAD.

This essay tries to answer the question: can we add the expressive
power of object oriented languages to OpenSCAD, without making it more complex?

In class-based object oriented programming, language complexity comes
from two sources:
 * state
 * classes

It might come as a surprise to some people that state makes a language
more complicated. But it's true: the field of functional programming arose
as a reaction against the complexity caused by state. Here's a transcript
of a Python session that demonstrates some of this complexity. The results
are baffling to a beginner, but unsurprising to an experienced OOP programmer.
```
  >>> a='foo'
  >>> a
  'foo'
  >>> b=a
  >>> b
  'foo'
  >>> a+='bar'
  >>> a
  'foobar'
  >>> b
  'foo'   # As expected.

  >>> a=[1,2,3]
  >>> a
  [1,2,3]
  >>> b=a
  >>> b
  [1,2,3]
  >>> a+=[4.5.6]
  >>> a
  [1,2,3,4,5,6]
  >>> b
  [1,2,3,4,5,6]    # What happened?

In Python, the difference in behaviour is caused by the fact that strings are
immutable values, while lists are mutable objects. Python has two different
equality operators, `a==b` and `a is b`, to test for value equality and
object identity, respectively. Since OpenSCAD is a functional language,
it only has immutable values. Since there are no mutable objects, there is
no need for two different equality operators.

The other source of complexity in class-based object oriented languages is
classes. The Self programming language was designed as a reaction against
the complexity of classes, and introduces the prototype/delegation style
of object oriented programming. In this style, there are no classes, but there
are instead object literals, and programs are structured as a collection of
object definitions. Instead of constructing an instance of a class, you
instead make a clone of a prototypical object. Behaviour that is shared
by multiple objects is represented by 'traits' objects, and an object can
delegate messages that it doesn't directly implement to a parent object.
The world's most popular OOP language, Javascript, is a prototype/delegation
language.

If you take Self, and remove state, then you are left with:
* object literals
* 'parent' slot definitions
* the 'self' keyword
* resend

Can this be simplified any further? Without state, we don't need to worry
about preserving object identity. If we copy a parent's fields into the
child at object construction time, can we do away with delegation?

I think we would need explicit support for inheritance or delegation.

Eg, single inheritance:
* 'inherit object;'
* 'super'
* 'self'

## Javascript

Javascript has a very complex and baroque design for delegation, and
yet still only manages to support single inheritance.
I don't want to mimic this design in OpenSCAD.

Somehow involves new, this, x.prototype, x.call, x.apply
* Every object has a 'prototype' field, which is an object.
  Objects inherit properties and methods from their prototype.
  Although the design is pretty weird.
* Javascript has object literals.
* A 'constructor' C is a function that references 'this' and initializes
  fields of 'this'. No return statement.
  Then you use 'new C(a,b,c)' to construct an object.
  (Simply calling C(a,b,c) doesn't work, returns null.)
  This is an alternate way to construct objects.
* 'C.prototype.field = value' is a way to modify the behaviour of constructor C.
  That's weird. Is prototype only a property of constructor functions?
  * Some JavaScript implementations allow direct access to the [[Prototype]]
    property, eg via a non-standard property named __proto__. In general,
    it's only possible to set an object's prototype during object creation:
    If you create a new object via new Func(), the object's [[Prototype]]
    property will be set to the object referenced by Func.prototype.
* "Function prototypes" seem incredibly baroque to me, compared to Self.
  Some stackoverflow 'what is this' queries back me up on this.
  However, they do permit 'isa' tests.
    ```
    function Ninja(){} 
   
    var ninja = new Ninja(); 
    
    assert( typeof ninja == "object" );
    assert( ninja instanceof Ninja );
    assert( ninja.constructor == Ninja );

* I'm told that assigning an instance of a 'superclass' to C.prototype
  is how you set up inheritance.
    C.prototype = new Super();
  Then you add new methods to C.prototype.

* someFun.apply(thisArg, argArray)
  Call a function, specifying the binding for 'this' explicitly,
  and specifying the remaining arguments using an array.

* someFun.call(thisArg, arg1, arg2, ...)

* someFun.bind(thisArg, arg1, ...)
  Quite strange. Create a specialized copy of someFun with it's 'this'
  permanently bound to 'thisArg', optionally with some initial arguments
  also specifies. Seems related to partial application of a curried function.

## Units: Cool Modules for HOT Languages

Maybe use the MzScheme concept of 'unit' as the model for how OpenSCAD objects
should work.
