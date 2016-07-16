# Easy Interoperability with Popular Programming Languages
I'll focus on C++, Python, Javascript, OpenSCAD.

## JSON Import/Export
JSON data exchange is a way to interoperate with any programming language.
Curv makes this easy because the 6 core data types are mostly isomorphic
to JSON.
* Import JSON using something like `import("foo.json")`.
* Export JSON. Provide a CLI option for exporting a specified member
  (or expression) of a script module as JSON.
  `curv -i expr -o out.json in.curv`

JSON objects correspond to Curv records.
JSON object keys are arbitrary string literals (not identifiers, as in Curv).
In order to fully support JSON import, I'd need a syntax for identifiers
containing an arbitrary sequence of zero or more printable characters.
These would be quoted identifiers.
The syntax I have in mind is `` `foo` `` as a quoted identifier.
How do you include a backtick character in a quoted identifier?
I could support generalized unicode escape sequences, but that's overkill.
The simple option is to represent a backtick by doubling it up.
(Importing a JSON dictionary with duplicate keys, or keys containing
control characters, is an error.)

## JCSG Import/Export
CSG trees are exported using JSON syntax. Call this format "jcsg".
This means shape values (and modules) know how to serialize themselves as JCSG.
And there is some way to import this format and recreate a shape/module.
Details are elsewhere.

## Modules
Curv modules are the unit of modularity,
and they are the unit of interoperability with other programming languages.

To call functions written in another language, we will import that language's
analogue of a module and convert it to a Curv module value. The foreign module
will need to be specifically designed to be compatible with Curv. Bridge code
will translate between the Curv data types and the data types of the other
language. Foreign functions should not have side effects. Curv values that
are shared with foreign code must be immutable.
* For C++, the use case would be importing highly efficient geometry code.
  * It's easy to create new builtins that invoke C++ functions,
    by hacking curv/builtin.cc. Part of "easiness" is that storage is managed
    using referencing counting and RAII shared pointers, and you can throw an
    exception derived from std::exception to report an error.
  * We could import a `*.so` or `*.dll` file as a Curv module.
    It would need a Curv-specific entry point that returns a Curv module value.
    This is easy to implement using libcurv.
* For Python, the use case would be using the extensive Python libraries
  to write geometry code callable from Curv.
  We could import a `*.pyc` Python module, which is wrapped up
  to behave like a Curv module. Python definitions are restricted to types
  that can be translated to the Curv types.
* For Javascript, the use case would be running the Curv UI in a web browser,
  or using Curv as a geometry library in a web browser.
  The details will depend on how that is accomplished, TBD.
* For OpenSCAD, there will be an effort to implement OpenSCAD in the Curv VM
  with at least 99% compatibility. OpenSCAD scripts can be imported as Curv
  modules. OpenSCAD functions and modules will be treated as Curv functions.
  The 3 separate namespaces is an issue: we might need two options for dealing
  with that, one being to prefix each OpenSCAD top level definition with 'v',
  'f' or 'm' to ensure that the names are different when referenced from Curv.

When calling Curv functions from another language,
the use case is writing a geometry application in your favourite language
while taking advantage of the Curv geometry engine and libraries.
I expect that the Curv API in your favourite language would include:
* Evaluating a string or file as Curv source code, or loading a Curv library
  module from a URL, producing an in-memory Curv module value.
  This is relatively easy using libcurv.
* An API for creating a Curv preview widget for incorporation into your
  program's GUI. This is also provided by libcurv and just needs to be wrapped
  for your language.
* With a lot more work, there could be an ahead-of-time compilation process
  to compile Curv code into something that can be more efficiently loaded into
  your program.

Libcurv already provides this for C++.
With a lot more work, we could use LLVM to translate a Curv script
into an optimized shared object or DLL file, plus a C++ header file.

## OpenSCAD
### OpenSCAD on the Curv VM
Mapping OpenSCAD types to Curv types:
* `undef` and `NaN`: `null`. IMO collapsing the two values should not cause any
  serious compatibility issues.
* boolean: `Bool`
* number: `Num`
* string: `Str`
* list and range: `List`
* function: Function (caveats below)
* module: double curried Function (caveats below)
* module shape argument: a Thunk, evaluated by each `children()` call,
  which returns a Module or Shape
* group: Module, extended with flags for the debug modifiers
* shape: Shape

OpenSCAD module calls pass the shape or group by-name, as a thunk that
is evaluated each time `children()` is invoked. Relativity.scad relies on
this behaviour; 99.99% of scripts don't care. Curv function values don't
support this. However, OpenSCAD doesn't have module values, and there's no
problem compiling calls to staticly specified modules into the needed bytecode.
The module function doesn't actually need to be double curried, just to make
OpenSCAD code run, the shape can just be a special argument.

The Curv language may or may not support first class functions with
OpenSCAD style named arguments and special variables, but that doesn't matter
for the purpose of just compiling and running OpenSCAD code in the VM.
We just need to provide the right set of opcodes for the compiler to generate.

We don't run into any real issue unless the two languages try to interoperate.

### Curv imports OpenSCAD
The use case is reusing the large amount of OpenSCAD code that is available
on Thingiverse, etc, without having to convert the code to Curv.

A good goal is 99.9% compatibility. Much of the truly weird semantics in
OpenSCAD are in function and module calls. It's easier for the Curv VM
to be bug compatible when running an OpenSCAD script directly, than when
calling a Curv first-class function value that has been converted from an
OpenSCAD function or module. So there might be less compatibility in that case.

Problem: the triple namespace. Solution:
* The three OpenSCAD namespaces are merged into one.
  If you try to reference an ambiguous name, you get an error.
* Use special notation (qualified identifiers) to disambiguate where necessary.
  Eg, `v$varname`, `f$funname`, `m$modname`.

Importing variables: No problem, the OpenSCAD value types map cleanly to
the Curv value types. `undef` and `NaN` are merged into `null`, but that's
not a big deal. OpenSCAD ranges are mapped onto lists, that's slightly more
disruptive, but still okay.

Importing functions: The main issue is whether we support named arguments and
special variables in exactly the same way as OpenSCAD. This would be the best
solution for OpenSCAD import and the OpenSCAD community, because we could
keep the syntax of function call exactly the same, and OpenSCAD functions
map directly to Curv function values. It's less ideal for interoperability
with Javascript and other languages, so a compromise is looming.

If Curv functions don't work exactly the same, then OpenSCAD functions can
still be mapped to compile time entities that preserve the same call syntax
and semantics. When a compile time OpenSCAD entity is converted to a Curv
runtime value (if that is supported), then the semantics have to change.

To what extent do special variables make sense in the context of first class
function values? Does a function closure ever capture the dynamic variable
environment, or does it only capture the lexical environment? (By definition,
the latter.)

If we want total bug compatibility with OpenSCAD, then we have to consider
that all unrecognized named arguments are 'special variables' that override
lexical bindings within the called function. With the Curv VM, that causes
a performance hit, and that is definitely something that only happens at
compile time, so OpenSCAD functions can't be promoted to values. Marius
isn't religious about total bug compatibility. There could be problems in
the field, where people want to run legacy OpenSCAD code, where we could
consider supporting selectable levels of bug compatibility.

Modules are weirder than functions. The second curried argument of an OpenSCAD
module is a thunk that evaluates to a shape or group each time `children()`
is invoked. 99.99% of OpenSCAD scripts don't rely on the thunk
`relativity.scad` is the one script I know that relies on this; it's a library
that a small number of other scripts use.
If we don't support the thunk, and instead use eager (or even lazy) evaluation,
then we can promote an OpenSCAD module to a Curv first class function value,
double curried, to the same extent that this is supported for functions.

### OpenSCAD imports Curv
Use case:
* Creating an OpenSCAD implementation that uses the Curv VM, and implementing
  many OpenSCAD geometric primitives in Curv rather than C++.
* Being able to use F-Rep geometry primitives within OpenSCAD,
  even though those primitives are coded in Curv.

Let's assume that certain Curv libraries are designed to be also used from
within OpenSCAD. That's more tractable than trying to support unrestricted
use of Curv modules from OpenSCAD. In the general case, this could get quite
ugly. For the general case, a better approach is to provide a migration tool
that mostly automates the translation from OpenSCAD to Curv.

OpenSCAD has 3 namespaces, Curv has 1. How does the mapping work?
* Each member of the Curv module is mapped to all three OpenSCAD namespaces:
  * A Curv binding can be used as a value (no problem),
  * it can be called as a function (error if wrong type),
  * it can be called as a module with 1 or 2 curried arglists (error if wrong
    type, error if it doesn't return a shape or group.

Note that this may create an issue in OpenSCAD, if the builtin `cube` is
also visible in the variable and function namespaces. This should mostly not
matter, but a possible workaround is to use qualified identifiers like `m$cube`
in definitions. The 'm$' prefix is invisible to Curv users of the same library.

We could introduce some new OpenSCAD syntax for dealing with Curv values
and scripts. This could make OpenSCAD as powerful as Curv, at the expensive
of ugly, bolted on syntax.
* `m$cube` is an expression that looks up a name in the module namespace.
* `f$sqrt` is an expression that looks up a name in the function namespace.
* In a function or module call context, v$f(x) calls `f` from the value
  namespace, `f$g(x)` calls `g` from the function namespace,
  `m$h(x)` calls `h` from the module namespace.
* Call a value-returning expression as a function or module:
  `(expr)(x,y)`, `(expr)(x,y){s;}`.
* In OpenSCAD, `a.b(c)` treats `a` as a Curv module or record value
  (that exports a single namespace), selects member `b`,
  then calls it as a function.
* `import("foo.curv")` imports a Curv script, returns a Curv module value
  which can be assigned to a variable.
* `import("foo.scad")` imports an OpenSCAD script, returns a Curv module value
  which can be assigned to a variable.

Requirement: OpenSCAD functions must preserve their semantics, WRT named
arguments and special variables, when they are converted to Curv values,
and then called as unboxed values from within OpenSCAD.
* The OpenSCAD boxed function call opcode takes a positional argument list,
  a named argument list, and a dynamic (special variable) environment,
  and calls the function, which can be either an OpenSCAD defined function
  or a Curv defined function.

## Python
Unlike Javascript, I assume that CPython uses the current bytecode
implementation of Curv. A Curv module is compiled into Curv bytecodes,
a Python module is compiled into Python bytecodes, and they can call into
each other. Curv uses non-atomic reference counting, but so does Python,
so it's safe. (Non-atomic refcounts are guarded by the Python GIL, the global
interpreter lock.)

Python has positional and named arguments. Externally defined functions
(not written in Python) may support only positional arguments, or they
may support a combination of positional and keyword.

### Python imports Curv
```
# import a Curv module, as an object
import curv
foomod = curv.Import("file:foo.curv")

# make a Curv module visible to the Python module system
import sys
sys.modules['foo'] = foomod
import foo

# configure the Curv preview window and display a shape
curv.configure_preview(...)
curv.display(curv.cube(10))
```
`curv` is a Python extension module written in C.
Python modules are just python objects, with special syntax for naming and
importing them. Using the Python `curv` module, you can import a Curv or
OpenSCAD script as a Python object, which can also be used as a Python module
if desired.

From Python, you can call an OpenSCAD function with positional and keyword
arguments, it works the same in both languages. Special variable arguments
are keyword arguments using the prefix `__`, since Python does not permit
`$` in identifiers. Eg, `curv.sphere(10,__fn=10)`.  Or use `f(x,**ENV)`
to pass a group of special variables in a dictionary.

In Python, an OpenSCAD module is a double-curried function.
It doesn't seem worthwhile to implement the thunk semantics for
the shape argument, which would require Python users to pass
`lambda():...` instead of `...` as the shape argument.
I'd like to just drop this aspect of OpenSCAD semantics,
as it messes up every language binding.

From Python, you can call a Curv function.

### Curv imports Python
So, `import("file:foo.py")` or `import("file:foo.pyc")`
or `import("python:foo")`.
This returns a Curv module (or it could be a record, not sure)
containing a translated representation of a Python module.
There's a mapping between Python and Curv data types.
If the Python module tries to export values (or a Python function tries
to return values) that can't be translated into Curv, then an error occurs.
I guess dictionaries with string keys are records and objects are modules,
the others should be obvious.

How does the Curv VM call a function (which might be a Curv wrapper for a
Python function)?
* For compatibility with Python multi-threading, we need to use deep binding,
  not shallow binding, for dynamic variables. So, there's a pointer to a
  dynamic environment that is supplied to functions.
* Original design: a function has a fixed number of arguments, N. The entry
  point to a function expects N values on the stack. Optional and keyword
  arguments are processed using function metadata, before the function is
  entered.

## Javascript
The use case for Javascript is running Curv code inside a web browser.

### Curv calls into Javascript
This happens when a web page calls a Curv function and passes a Javascript
function as a parameter.
* Javascript doesn't support named parameters in a general way.
  Instead, there is a convention where the final positional parameter
  is called the 'options' parameter, and it's an object literal of name/value
  pairs.
* I think the common case for calling a function parameter is to pass
  a small number of positional parameters. We won't have a large, elaborate
  named parameter interface similar to the `cylinder` function.
* As long as we are calling a Javascript function with positional parameters
  only, there's no problem. I'd like to say that in this case, you just pass
  a native Javascript function with no special encoding.
* Javascript functions can have metadata (dot attributes). If named parameters
  need to be interpreted, then the Javascript function should have an attribute
  specifying how those named arguments are to be interpreted.
  This is a `params` attribute which is a list of (parameter name,default value)
  pairs. Let's say that an error occurs if a Javascript function is called
  with labeled arguments and there is no `params` attribute to interpret them
  with. And there is some story about how special variables are handled.
  If a Javascript function is prepared to deal with metadata, then an attribute
  is used to declare this.

### Javascript calls into Curv
This is the more normal case. We want the ability to call OpenSCAD and
Curv functions from Javascript.
* How is a compiled Curv module represented within a web browser?
  The current C++ bytecode runtime is not multi-threaded, and uses non-atomic
  reference counts, so values can't be shared between threads. That won't work
  in Javascript. What might be better is to compile a Curv module into
  Javascript. That will have benefits and drawbacks, but it's probably the best
  approach for web browser integration.
* Efficiency is an issue. We don't want a heavy and expensive argument
  encoding. Ideally Curv and OpenSCAD functions just get compiled into
  conventional and idiomatic Javascript functions that the Javascript
  compiler knows how to implement efficiently.
* User defined OpenSCAD and Curv functions all have a fixed number of
  parameters, some of which are optional. If we say that Javascript must
  use positional parameters when calling these functions, all is well.
* Curv additionally supports the ability to support the Javascript
  convention of passing a record as the final parameter (an options parameter).
  And I'm planning to use that convention in the shape3d protocol.
* OpenSCAD functions can require the use of special variables.
  * In theory, the Curv compiler can use global analysis to determine
    that a given OpenSCAD function uses or ignores special variables.
    The Javascript version could be decorated with an attribute for this,
    if it helps.
  * How does Javascript code pass special variables to an OpenSCAD function?
    Maybe there is a postultimate optional parameter which is a dictionary
    of special variables.
  * Or maybe there's an attribute with an alternate entry point to the function
    with an extra environment parameter.
  * In Javascript, there is a special way to call a function where you
    specify an extra magic `this` parameter. So maybe use that as the
    alternate entry point. `fun.call(this,arg1,arg2,...)`

## Function Parameters
What is the impact of the Curv function calling convention on interoperability?
* In the case of calling foreign functions, they have to be specially designed
  to be called from Curv.
* In the case of other languages calling into Curv (eg using Curv as a
  geometry library), any Curv function needs to be callable,
  and there needs to be a reasonable default mapping from Curv functions
  to the other language.
* There is also the case of compiling Curv into other languages,
  specifically machine code or C++ compatible object code via LLVM,
  and GLSL for execution on the GPU.
  In these cases, I need a high performance translation.

Curv functions have a fixed number of parameters. There is no 'varargs'
mechanism, unlike Python and Javascript. This is good for interoperability.
* It's a design issue. A varargs mechanism adds extra complexity we don't need,
  given that it's just as easy to pass a list argument. If there are two ways
  to pass a variable number of arguments (varargs and lists), then both will
  be used, and users will need to explicitly convert between the two
  conventions. It's not worth it.
* It's a performance issue. When compiling to machine code or GPU code,
  I want to use a fixed-argument calling convention where arguments can be
  assigned to registers. And/or I want to use a calling convention that is
  compatible with tail call optimization, and in LLVM, varargs isn't compatible.

Tail parameters may be optional, with a default value.
This causes no interoperability issues. The 4 target languages all
support this.

Keyword arguments?
There's no rush: I'll start without them, and see how things evolve.
* If we don't support these, then it makes it easier to call Curv code from
  other languages that lack these features.
* In Javascript, there isn't keyword argument support in the same way as
  Python or OpenSCAD. Instead, functions that need keyword arguments
  use a convention where the last argument is an "options" argument consisting
  of an object literal that contains keyword arguments. In most cases,
  the options argument is optional. There is a pattern matching feature in ES 6
  that makes this convention easy to use.
  * More abstractly, Javascript function arguments are classified in advance
    as either positional or keyword, they can't be both. Also, "JavaScript
    engines should be able to optimize invocations of functions and methods
    with ES6-style named parameters such that no temporary object is created."
  * If I use the Javascript convention instead of supporting keyword arguments,
    then I have more flexibility in wrapping a Curv function for being called
    from another language. This convention consists of a final 'options' 
    parameter that is a record pattern containing a fixed set of fields.
    Obviously Javascript would be easier. For C++, I have the option of
    constructing a curv::Record value and passing that as a parameter,
    or flattening the options argument into a sequence of positional parameters.
    (If necessary, Curv functions could have two entry points for these
    different conventions.)
    For OpenSCAD and Python, I could convert the options parameter into
    keyword parameters.
  * If Curv lacks keyword parameters, then how do you call an OpenSCAD function?
    As long as OpenSCAD lacks record values, I could interpret a
    final record literal argument as a set of keyword parameters.
    But OpenSCAD could be extended to be more Curv-like. So we'll see.
  * This approach is also more flexible than keyword arguments, since records
    are first class values that represent sets of model parameters.
    And then there is the design argument of not supporting two mechanisms
    for doing the same thing.
* I could support keyword arguments on the same basis as OpenSCAD.
  In that case, from Javascript, there could be a special object type that
  represents a set of keyword arguments, distinct from the Javascript type
  that represents a Curv record.

Special variables? None of my "popular languages" support this mechanism.
There's no rush: I'll start without them, and see how things evolve.
* Don't support special variables directly.
  Use record arguments instead. This is my current plan for the shape3d
  protocol.
* Support special variables. Provide some workaround for specifying them
  from another language.
  * Global variables.
  * A special environment argument.
