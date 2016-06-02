= Classification of Values

Standard "type" predicates like is_boolean, is_number, ...

The "variety" or "contract" mechanism for classifying objects.
* Contract is a subtype of Predicate. A Contract is an opaque token that
  can be attached to an object: it means, the developer certifies that this
  object obeys the contract, which is specified in documentation only.
* C = new_contract(C1,C2,...);
* implements C;

For F-Rep objects, I want a system of inherited and synthesized attributes,
inspired by AST technology.

Synthesized attributes:
* An object synthesizes an attribute (which classifies the object)
  based on its implementation, and the attribute values of the same name of
  its component objects.
* Could use boolean fields. Each field needs a predicate for testing
  an object, returning the boolean field value or false if it isn't defined.
* Could use contracts.
  * Need a way to conditionally implement a contract.
    * Treat 'implements' like a generator,
      and permit 'if (cond) implements C;'
      So this means that 'implements' statements are ordered,
      unlike field definitions?

Inherited attributes:
When you construct a shape's distance field,
you can specify requirements, like the outside should be euclidean.
* Sounds like the requirements would be passed as an argument or a set of
  arguments to get_distance_field(). A given distance field constructor
  receives a set of requirements from its caller, and can augment this set
  before passing it to get_distance_field() calls on its component shapes.
* The requirements are usually a set of named boolean attributes,
  where if the attribute is missing then it is false. (A familiar pattern.)
* Might there also be numeric attributes? Like an error bound on how
  euclidean the outside should be. Again with a default if missing.
* However these attributes are defined, it is decentralized.
  A specialized library might invent new requirements, which are defaulted
  in shapes not designed for use with this library.

Test if an object field is defined: `defined(<selection>.<identifier>)`
or `defined(<selection>@<string>)`.

A "requirement" named R is a generalized contract.
* It has a name and a default value.
* It is a unary function that tests if an object has a field of that name.
  If so, it returns the field value. If not, it returns the default value.
* `requirement <id>(<default-value>);` defines a new requirement.

```
requirement is_polyhedron(false);
```
is the same as
```
is_polyhedron(X) = if (defined(X.is_polyhedron)) X.is_polyhedron else false;
is_polyhedron(X) = defined(X.is_polyhedron) ? X.is_polyhedron : false;
```

More briefly,
* `? obj.id` -- true if defined, else false
* `obj.id ? default` -- value of obj.id if defined, else default
* is_polyhedron(X) = X.is_polyhedron ? false;

This reminds me of the default/override system in the Gen language (root.vars,
tree.vars, leaf.vars, local.vars; set, cset, append, prepend, var||default).
And also, the default/override system in OpenSCAD2 (mixins & override,
include and overlay).

== Atoms
One of the ideas behind 'contract' and 'requirement' is to provide a way
to create new unique "branded" values, using either a special definition
syntax, or a special new_* function with the compile time side effect of
creating a new value. Each new value needs to be bound to a name in order
to be useable.

Which reminds me of 'enum' in C, and Haskell algebraic types.

Why not encapsulate just this idea in a new language mechanism?
* `atom id;` -- create a new atomic value, bind it to `id`.
  It prints as `id`. It is equal to itself, but to no other value.
  The only operations are equality and printing.
  In effect, `id` is a new literal constant.
* `atom f(x) = ...;` -- define a named function which is also an atom.
  Same atomic properties of equality and printing.
  Eg, `atom id(x)=x;`, which satisfies a requirement of Transformations.
  * All of the function values that result from evaluating this definition
    compare equal to one another, even if the function captures a nonlocal
    variable that varies from one instance to the next.
  * Atomic functions violate the Law of Substitution, which requires that
    a function name can be replaced by its body without changing the meaning
    of the program. The `atom` keyword makes it explicit that the
    substitution law doesn't hold. This also explains why function definitions
    are not atomic by default.
* `atom obj = {...};` -- define a named object which is also an atom.
  An atom `A` bound within the object is printed as `obj.A`.
* `atom p{a,b,c,...};` defines atoms named `p`,`a`,`b`,`c`,...
  where `p` is a predicate that is true of `a`,`b`,`c`,...
  This is like `enum` in C.
  It's equivalent to:
    ```
    atom a;
    atom b;
    atom c;
    atom p(x) = x==a||x==b||x==c;
    ```

I'll argue this is a more solid design than Lisp's
interned and uninterned atoms.

This is *not* generalized to permit `atom zero = 0;`
because that would screw up the algebraic properties of arithmetic expressions
and impede the optimizer.

How would we design Haskell-like algebraic types, and specifically constructors
with arguments?
* The obvious case (mimicking Haskell):
  `atom p(a,b,c(x,y));`
  * Is there a predicate for values of constructor `c`? (Not obviously.)
  * Do we support pattern matching for values of constructor `c`?
    Not like in Haskell; we can't distinguish a formal parameter name from
    a constructor name due to dynamic typing.
    We'd need an operator to specify the presence of a constructor name.
    I have no good ideas, let's try `$`.
    Eg, `$c(x,y)=foo;` binds `x` and `y`.
  * With pattern matching, you can construct a predicate
    by writing `is_c(x)=switch(x)case $c(_,_)=>true else=>false;`
  * The constructed value `c(1,2)` prints as `c(1,2)`.
  * Equality follows from this.
* Can a constructor with arguments stand alone? Eg, `atom c(x,y);`.
* Can we access the components of a constructed tuple using dot notation?
  `t=c(1,2);assert(t.x==1);`
* I have no intention of implementing abstract data types
  with enforced information hiding. The complexity tax is too high.
  However, a pseudo ADT coding style
  is to use functions for the abstract interface,
  and dot notation to access the representation of abstract values.
* Dot notation suggests that constructed values are a kind of object,
  since both contain a set of name/value pairs.
  Does this suggest an extension, eg where you have data fields (that can be
  pattern matched) and function fields (that are not)?
  Not really.
  * The functions can capture non-local variables, and two constructed values
    that print the same (with the same data values) might be operationally
    distinct. (Technically, equality would still be an equivalence relation,
    but this could lead to non-obvious problems, make programs harder to
    reason about.)
  * This sounds like an attempt to introduce an OOP coding style,
    where operations on an abstract value are invoked as obj.f(x).
    I don't want to go down this path.
    * In TeaCAD, operations on an abstract
      value are f(obj,x). That keeps things simple and consistent,
      and is consistent with multiple dispatch.
    * Dot notation is for: access to the internal representation of an
      abstract value (data), concrete structures with only data components,
      and modules.
