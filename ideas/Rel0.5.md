Curv Changes since Release 0.4
==============================
There are multiple language changes, with deprecation warnings for old
syntax. You should fix deprecation warnings now if you have old Curv code,
because the old syntax will be removed in a future release. By default,
deprecation warnings are throttled. Use `curv -v` to see all warnings.
Features marked experimental are unstable, subject to change.

Platform/Build Support
----------------------
Curv 0.5 includes a prebuilt AppImage for Linux.
* @ComFreek -- Windows port.
* @zeeyang: automated MacOS build (using github actions)
* @A-G-D: automated AppImage build for Linux (using github actions)
* Apple Silicon support on Macintosh.
* AMD GPUs supported on Linux using AMD Mesa 19.x driver.
* documented/tested exporting images on a headless Linux server.
* `make upgrade` command

File Import/Export
------------------
* @Xiaoyuew -- Faster mesh export (multi-threaded, multi-core).
* @zeeyang: add `jgpu` export file format (GPU program as JSON)
* @zeeyang: add `GLTF` export file format
* @zeeyang: remove `curvc` (no longer being built)
* `curv dirname` reads a directory as a Curv program (directory format)
* JSON export no longer tries hard to distinguish JSON from non-JSON data.
  Non-JSON data is now exported as a string.
  `#foo` exports as `"foo"`. `sin` exports as `"<function sin>"`.

Command Line
------------
* @A-G-D: filename now optional in `curv -le`, default is `new.curv`
* Configuration file `~/.config/curv` contains defaults for -O options,
  is a Curv source file, a record containing {viewer,export} fields.
  See `docs/Config.rst`
* All -O option values are now specified as Curv expressions.
  Symbols used in -O options: -Ocolouring=#vertex or -Ocolouring=#face
* rename -Ocolour= to -Ocolouring=
* `--depr=` option controls deprecation warnings

REPL
----
* supports := (assignment) statement
* fully supports generator expressions
* `help` in REPL is extremely experimental, only partially implemented
* see `docs/REPL.rst`

Rendering in Viewer Window
--------------------------
* @zeeyang: improved raymarching (backup if overstep due to bad SDF)
* There are configurable render parameters. For a list, use `curv --help`.
  * You can set them on the command line using `-Oname=value`.
    What's new: `value` is an arbitrary Curv expression.
  * You can set them in the config file, in the `viewer` section. [new]
  * You can set them in a shape value by manually adding a `render` field
    to the shape structure. [new, experimental]
* new render parameters:
  * ray_max_iter, ray_max_depth: bug #78 [experimental]
  * `shader` -- lighting & shadow function for object surface [experimental]

Shape Library
-------------
* @lf94: add `lib.builder` with `snowman` example [experimental]
* @lf94: add `soroban` example
* @zeeyang: add 2D `polyline` primitive
* @zeeyang: add `repeat_finite` primitive
* `capped_cone`
* fix `show_axes` to preserve the bounding box
* generalized `polygon` primitive (can be non-convex, self-intersecting)
* lib.noise [experimental]
* `a >> into union [b, c]` means `union [a, b, c]`

Compiler and Interpreter
------------------------
* general tail call optimization
* better compile/runtime error messages
* function names in stack traces
* SubCurv enhancements (more things that compile to GLSL shader language):
  * numeric matrix data types: 2x2, 3x3, 4x4
  * matrix multiplication (`dot`)
  * boolean vector types: bool2, bool3, bool4, bool32
  * boolean vector operations: and, or, xor, not
  * boolean matrix types: bool2x32, bool3x32, bool4x32
  * better support for parametric variables
* add `docs/Implementation` (source code structure, lang. implementation)

Curv Language
-------------
* Quoted identifiers: `'foo bar'` is an identifier with an embedded space.
  `'if'` is an identifier, not a reserved word. `'_` is an escape sequence
  representing a literal `'` within a quoted identifier.
* Numbers
  * number patterns like `42` or `-0.5`
  * `sign` function returns -1, 0 or 1
  * `sum` added to SubCurv
* Lists
  * `a++b` infix list catenation operator
  * deprecate `(a,b,c)` list syntax; use `[a,b,c]` instead
* A string like `"abc"` is now a list of characters.
  Lists are heterogenous, so `"abc" ++ [1,2,3]` is a list of 6 elements.
  * `#"a"` is a character literal, and works as a pattern.
  * `is_char x` is true if `x` is a character value.
  * `char` converts an integer or list of integers to a character or string.
  * `ucode` converts a character or string to an integer or list of integers.
  * `encode` removed, replaced by `ucode`.
  * `decode` removed, replaced by `char`.
  * `string` converts an arbitrary value to a string.
  * new escape sequences in string literals: `$_` -> `$` and `"_` -> `"`
* Symbols are abstract named values. They replace some uses of strings, but
  symbols are scalars, not lists. They replace 'enum' values in some languages.
  * #foo or #'hello world' is a symbol literal, which works as a pattern.
  * `symbol` function constructs symbol from string
  * `is_symbol` predicate.
  * true, false, null are now just aliases for #true, #false, #null
  * JSON export exports #true, #false, #null as JSON true, false, null.
  * Remove `is_null` predicate (breaking change).
* Data structure indexing
  * `a.[i]` is new array indexing syntax. `a[i]` is deprecated.
  * `r.[#foo]` is same as `r.foo`. Syntax r."foo" is deprecated.
  * `a.foo.[i] := ...` assignment statements
  * multi-dimensional array slicing using `a.[indexlist1,indexlist2]`
  * generalized index values (aka lenses, experimental):
    * `a.[i]` indexes a data structure with index value `i`
    * integers are index values (they index lists)
    * symbols are index values (they index records)
    * `a.[[i1,i2,i3]]` yields `[a.[i1],a.[i2],a.[i3]]`
    * `a.[this] yields `a`
    * `a.[tpath[i1,i2]]` yields `a.[i1].[i2]`
    * `a.[i1,i2]` is a multidimensional array slice,
      short for `a.[tslice[i1,i2]]`.
    * `amend index newelem tree` is pure functional update,
      like `tree.[index]:=newelem` without the side effect.
* Functions:
  * `is_fun` deprecated, replaced by `is_func` and `is_primitive_func`
  * `error` is a function value (previously was magic syntax)
  * `id` -- identity function
  * `identity` renamed to `idmatrix`
  * `compose` is generalized to work correctly on partial functions
* Boolean array operators.
  * SubCurv supports new types Bool32, Bool[2-4], Bool[2-4]x32
  * experimental boolean array operations (also in SubCurv):
    * float_to_bool32, bool32_to_float
    * nat_to_bool32 (in SubCurv, argument must be a constant)
    * bool32_to_nat (not in SubCurv)
    * bool32_sum, bool32_product
    * lshift, rshift
  * vectorized boolean `not` function; unary `!` operator is deprecated
  * vectorized boolean reduction functions `and`, `or`, `xor`
  * vectorized: <, <=, >, >=, bit
  * `select` [experimental]
* `parametric` records have a `call` field that reruns the constructor
  with different parameter values. The result is another parametric record,
  also with a `call` field.
* `test` definitions, for unit tests in a module, or anywhere else
* `sc_test` statement, for unit testing SubCurv [experimental]
* `assert_error` statement, for writing unit tests
* comma separated definition lists in `let ...` and `{...}`.
  Previously only `;` was allowed as separator.
* in a statement context, () and (a,b,c) are generators.
* `where` is deprecated, use `let` instead.
* in a dynamic record literal, [#a,1] is equivalent to a:1, both expressions
* `locative!func1!func2` is a statement, mutates a local variable by
  applying one or more functions to the contents.
* deprecate "foo":expr, in expression or pattern context.
* `until` clause for early exit from a `for` loop
* `var` definitions are deprecated, use `local` definitions instead.
* `local <definition>` is a generalized local definition, with sequential
  (not recursive) scope. Legal in any imperative context: `do` expression,
  compound statement, list constructor, dynamic record constructor.
* Only definitions are permitted in the head of a `let` or in a module
  (not actions). Use `test` definitions to add unit tests to a module.
* Remove 'generalized do'. Only `do` expressions are supported.
* fewer levels of operator precedence:
  * unite Power with Unary; not a breaking change; grammar becomes more liberal.
  * unite Relation with Range, breaking change, parse error for `1..3==[1,2,3]`
