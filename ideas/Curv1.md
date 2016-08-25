# CURV 0.1 milestone: "Lisp 1.0"

Just enough to write real programs and run benchmarks.
* string literals, [a,b,c] list literals, no list comprehensions
* seq@i, len(seq), concat[s1,s2,s3]
* && || !, if-else, relational ops
* Top level function defs only. No function literals. No optional, keyword args.
* f(x,y) only, no right associative function calls.
* Top level value defs only, order independent.
  No 'let', no {...}, no local definitions.
* script("filename").foo

Focus on data structures.
Worry about LLVM later. Use a 'Meaning' tree interpreter.

## String
`struct String:public Ref_Value {size_t n; char data[n];}`

`mk_str(const char*)`

## List
`struct String:public Ref_Value {size_t n; char data[n];}`

`mk_list(size_t n)`

## Object
A Top_Level_Object is a lazy run-time value that JIT compiles functions
on first call. Initially, only the function parse tree is stored.

contents:
* an element list.
  * at parse time, it's a list of phrases
  * at analysis time, it's a list of meanings. Or is it lazy too?
  * at runtime, it's a Value, constructed at Object construction time
* a slot array. Used to store values of definitions, so that compiled code
  can index into the array rather than use a run-time map lookup.
  * at parse time, doesn't exist
  * at analysis time, we store the size
  * at runtime, it's a List allocated during construction
* In CURV0, there is one slot per definition.
  Initially, all of the slots hold parse trees.
  On first reference, analysis and evaluation take place, and the slot is
  updated with a Value, thunk or rfunction.
* a map from names to slot indexes
  * at parse time, phrases
  * at analysis time

Script_Phrase:
* children: list in source order
* analyze() -> Script_Meaning(phrase)

Script_Meaning:
* phrase
* actions: list of phrases
* names, map name -> slotid
* slots, array of phrases
* eval() -> Script(phrase, names)

Script:
* phrase
* elements, or null (array of values)
* slots. initially each item is a phrase
* names, map name -> slotid

selection: `foo.bar`
* at parse time, Selection_Phrase
* at analysis time, Selection_Meaning (fully boxed case)

Value::select(name) -> Value

## Identifier
An Identifier phrase is resolved to a builtin, or a top-level definition,
or a parameter. No other choices in CURV0, due to a lack of nested scopes.

The VM has
* a 'current script' register, pointing to a slot array
* a 'current arguments' register, pointing to an argument array

meaning of an Identifier:
* builtin: Constant
* topdef: Slot_Ref(index)
* parameter: Arg_Ref(index)

The builtin namespace is a map from name to Constant.
(Later, Entity, with ::apply and ::select, where some Entities are constants.)

curv::Expression::eval(Context) -> Value
Eval_Context is `(Value* slots, Value* args)`

curv::Phrase::analyze_expression(env) -> Expression
Analysis_Context is builtins, Script_Meaning, parameters
::lookup(id) -> Expression

## Function
Two kinds of function: builtin and top-level function definition.

Builtin:
* parse time: unresolved identifier
* analysis time: Constant. Later we'll need more info for optimization,
  but it should still be a Constant node.
* run time: Function object with a virtual apply() method that
  maps a sized array of Values to a Value. (This is the fully boxed
  representation, needed even with LLVM.)

User-defined:
* parse time: Function_Definition
* analysis time: Function_Meaning
* run time: if the function escapes to a boxed value context,
  then we construct a Closure <: Function value from an rfunction,
  as an (Object,Function_Meaning) pair.

Function call: f(x,y)
* parse time: Call_Phrase(fun-phrase, list(arg-phrase))
* analysis time:
  * if fun-phrase resolves to a Slot_Ref,
    then generate a Slot_Call(index,list(arg-meaning)),
    otherwise a Boxed_Call(meaning,list(arg-meaning)).

DEFER: stack trace printed if exception thrown
