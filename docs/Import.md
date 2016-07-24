# The Global Resource Table

A *resource* is a value that is referenced via a URL, using `import`.
`import(URL)` or `import(URL,extension)`.

## On-Disk Resource File Cache
We cache copies of resource files locally, to reduce network access and avoid
pounding servers containing popular resources.

## In-Memory Resource Cache
And maybe we also cache currently referenced resource values in memory,
in a resource table, to avoid loading the same resource file multiple times.
This is a map from URLs to resource values, that goes into the `Session` object.

## In-Memory Script Cache
A script is a special case of a resource file.
The main examples will be Curv and OpenSCAD scripts.
The defining characteristics are:
* A script can contain references to other resources.
* A script is compiled/evaluated into a more efficient representation
  before it can be used as a value. And the evaluation may need to be
  redone if a referenced resource changes.

Maybe we also have an in-memory script cache. Scripts are not resource values;
they are *source code* that is evaluated to produce a resource value.

## Auto Reload
You can set the Auto Reload flag on a resource file referenced by the Resource
or Script Cache. If the resource file is modified (on disk), then it is
automatically reloaded. And this triggers zero or more preview windows
to be updated, if their contents depends directly or indirectly on
the resource that was reloaded.

There needs to be an abstraction which divides responsibility between the Curv
engine (which knows about resource dependencies, but doesn't know about
user interfaces) and the UI.

This requires that a dependency DAG be maintained.
This sounds like the Observer design pattern.

## Scripts and Modules
Three design alternatives:
* **[1] Each script module compiles all its sources.**
  So if there are 3 primary script modules being edited in the GUI,
  and they all depend on some external library L,
  then there will be three copies of the compiled representation of L
  in memory, one copy for each script module.
  * Auto Reload: When a script is explicitly evaluated (via `eval_script`),
    a `Script_Module` object is first constructed. Each time a script is
    referenced (during compile time or eval time, the distinction is fuzzy),
    the Script Cache is consulted. We build a mapping from Script objects
    to the list of Script Modules that reference them.
* **[2] Compiled script modules are cached in a global module table.**
  For each Script in the Script Cache, there's only one copy of its
  compiled representation. So the Script Cache becomes a Module Cache.
  * **[2a] Global optimization.**
    When a script is modified, it is recompiled,
    and all of the modules that reference it must also be recompiled.
    * I guess each script module has a list of weak references to the modules
      that directly reference it. Follow the list and recompile those modules
      as well.
      * Might hit the same module twice (it's a DAG, not a tree), so store a
        timestamp/generation number in the module for each compile sequence.
      * Or, just mark each referendum as dirty, and then lazily recompile.
  * **[2b] Late binding.** Script modules are the unit of separate compilation.
    When a script is modified, it must be recompiled, but none of the other
    script modules referencing it need to be recompiled.
    * `use M` is implemented by emiting a sequence of by-name bindings:
      `x=M.x; y=M.y; ...`. The value M in a module slot, and it is not static.
      Each reference to `x` must evaluate `M.x`.
    * Since a `Script_Module` contains a counted reference to each script module
      that it uses, there cannot be recursive references between script modules,
      otherwise there's a reference cycle that leaks memory.
      So this is checked for at compile time.
    * The `Script_Module` values stored in the global module table
      are mutable objects: when a script is modified, the script is recompiled
      and the `Script_Module` is replaced with the new value, without the
      address changing.
    * Sounds like a Lisp or Smalltalk system.
    * Even with "late binding", a script module being previewed has top
      level shapes, which change when dependent resources change.
      And when those shape values change, we'd typically need to regenerate
      the GLSL code, which is a compilation step. So "late binding"
      doesn't eliminate the need to maintain a dependency dag and do
      dependency-driven re-evaluation. Instead, "late binding" seems like
      a performance optimization using a specific choice about which
      intermediate products can be preserved and which need to be regenerated.

So there are a range of design choices, involving tradeoffs:
faster recompilation vs faster evaluation.
But: which design is the simplest?
