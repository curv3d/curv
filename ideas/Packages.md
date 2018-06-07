# Files, Trees and Packages

Files, Trees, Packages and Lib are 4 proposed mechanisms for Curv source files
to reference external resources.

Curv needs a package manager. We can define a package as an encapsulated module
composed of a number of files, and then focus on mechanisms for referencing
external packages.

These features support modular programming in Curv, wherein a large system is
partitioned into encapsulated modules. One conventional property of a module is
that its dependencies on external modules are all defined in one place. Within
the body of the module, simple names are used to refer to these dependencies.
This style should be *possible* in Curv, even if it isn't enforced.

## File Syntax

File Syntax is a set of rules for interpreting a regular file as a Curv value
(based on its extension), and for interpreting a directory as a Curv value.

Some file types that might be supported:
* `*.curv` -- a Curv expression, which evaluates to an arbitrary value.
* `*.json`
* `*.toml`
* `*.rsdf` -- a Regularly Sampled Distance Field -- a voxel grid of distance
  values, in binary.
* *directory* -- if a local filename names a directory, then Tree syntax is
  used to interpret the directory as a Curv value.
* `*.vstor` -- Value Store: A compressed binary file representing an arbitrary
  Curv value. A ZIP file containing a Tree (similar to `*.ODT` or `*.3MF`).
  The primary use case is to represent a Curv shape as a single file,
  where we want to package Curv source code together with some binary files.

The shell command `curv filename` interprets `filename` using File Syntax,
reading and evaluating the file and then displaying the resulting Curv value.

Mime types:
* `*.curv` == text/curv
* `*.rsdf` == application/curv.rsdf
* `*.vstor` == application/curv.vstor

## The `file` Function

This will not be part of Curv. Case analysis:
* `file relative_pathname`: Replaced by `file.name`.
  Avoids tricky code that restricts the use of `..` to escape from a
  package boundary.
* `file absolute_pathname`: This is potentially useful in a local workspace.
  But you could also use a symlink, and reference the symlink with `file.name`.
  This feature is a security hole if used in a package or `*.curv` file
  downloaded from the internet.
* `file URL`: How important is this, when we have `package URL`?
  Potentially more susceptible to being used as a backchannel for malware
  to "phone home", than `package`.

This also means I won't have 'parameterized file readers'.

**Parameterized File Readers**
* If additional parameters must be supplied in order to interpret the contents
  of a file, then a new function must be defined that is specific to that file
  type. Eg, `svg_file` or `dxf_file`.
* This is the case where the File mechanism can provide functionality not
  duplicated by the other 2 reference mechanisms.
* But, this may be a bad idea. There's no mechanism to refer directly to a file
  embedded in an external Tree or Package. What to do?
  * An Tree can provide an API to its clients for accessing data files that
    require a parameterized file reader.
  * We can banish the concept of a parameterized file reader. Instead,
    * A file reader for something like an SVG or DXF file can return a subtype
      of Shape that provides rich access to the data.
    * An external tool can convert one of these files into an alternate form
      that can be read by Curv without parameterization. For example,
      mesh files are not directly readable in Curv, you must instead convert
      the mesh to an RSDF file, and provide the mesh conversion parameters
      to this external tool.
    * If we banish parameterized file readers, then the File mechanism can
      be deprecated, and perhaps not supported in a Tree.

## Trees

Curv has a 'directory syntax', which interprets a directory tree as a Curv
value: by default as a nested record value. Directory entries are interpreted
as record members. Regular files named 'identifier.extension' are interpreted
using File Syntax. Subdirectories named 'identifier' (no extension) are
interpreted using Directory Syntax. Entries that don't match these patterns
are ignored.

The root of the directory tree is marked, possibly by an empty file `.curvroot`.

Trees are encapsulated. You must use the Package mechanism to reference
resources outside of the Tree. If `file` is used by a `*.curv` file in a Tree,
you can only use relative pathnames, and you can't use `..` to reference files
outside the Tree.

The purpose of Tree syntax is to provide a local file system representation
of `*.cpkg` files and Packages. That's why Trees are encapsulated.

Within `*.curv` files in the Tree, other members of the tree can be referenced
using lexical scoped identifiers. A directory containing files 'foo', 'bar',
etc, is semantically equivalent to a record '{foo=..., bar=..., ...}'. The
parent scope of the root directory is 'std', the standard namespace. This
reference mechanism doesn't provide any additional expressive power over
`file`, it's just nicer and more convenient.

Trees may be nested. A directory tree with a `.curvroot` may be nested inside
another directory tree.
* This could be used for multi-package repos, or to ship a package with its
  dependencies.
* How does one subtree reference another sibling subtree as a dependency?
  Let's review the existing external reference mechanisms:
  * Lexical scoping. Nope.
  * `file`. Nope.
  * `package` + URL. Nope.
  * `lib`. Nope.
  What to do?
  * Maybe the `.curvroot` file contains definitions of dependencies,
    evaluated in the scope of the parent tree. Use lexically scoped variables
    to reference sibling packages, and `package` for Internet scoped packages.

A Tree can evaluate to a Shape. That's a requirement for `*.cpkg` files.
We will extend the directory syntax with an optional file that contains a Curv
expression that is evaluated to the directory's value. This can occur in any
directory, not just the Tree root. Call it `.main.curv`.

To export only 'public' members of a directory, use a `main.curv` file
that contains
```
{
   foo : foo,
   bar : bar,
}
```

A possible extension: `.include.curv` evaluates to a record whose members are
added to the record denoted by the directory.
Can get the same effect using `main.curv`, as shown above.

Many modern languages now have a standard tree/package/project manager
that will create a project tree for you, then perform operations on that
project tree. Often with git integration. Examples:
* Rust, `cargo`
* Clojure, `lein`

## Trees (version 2)

Maybe it's too weird that an identifier `foo` not defined anywhere in a
`*.curv` file is implicitly defined by a sibling file `foo.curv`. So, files
aren't converted into Curv bindings unless they are explicitly declared in
a `*.curv` source file. Extraneous files and directories that aren't
explicitly referenced are ignored.
* The value of a directory `foo` is specified by `foo/main.curv`.
* An explicit file reference or declaration for a file `foo.*` is:
   1. `use foo;`. Can also write `use foo.bar.baz;`.
   2. That makes it cumbersome to include a file into a scope (need two
      definitions). `file.foo` is an expression.
      `use a.b.c` is equivalent to `c=a.b.c`.
      So now we have `use file.foo` or `include file.foo`. (Orthogonality.)

The benefit of an explicit gesture like `file.foo` is that you get an
explicit error message "File not found".

`file.foo` is interpreted at compile time, because it is intended to behave
like an identifier. `file` is not a record, despite the use of dot notation,
it is a mechanism for doing lexically scoped identifier-like lookups
in a Directory Syntax document.
* Mutually recursive references between two `*.curv` scripts is illegal,
  because of implementation restrictions (ref counting not garbage collection).
  This is enforced at compile time.
* Using a fancier compiler, we could permit mutual recursion between files,
  with an implementation that still requires `file.foo` to be resolved at
  compile time.
* No immediate plan to implement `file."${foo}"`, `defined(file.foo)`,
  or `fields file`.

Under this interface, a record field could be represented by two files with
the same basename and different extensions. Eg, one contains raw data in some
standard non-Curv format, the other contains Curv metadata. (This is an
alternative to "parameterized file readers".) Or, one contains geometry
and the other contains colour.

Can relax the requirement that directories contain a `main.curv` file.
If not, construct a record from every suitable directory entry.

Can relax the requirement for a `.curvroot` file. `file.foo` means: search
for `foo.*` in the current directory, then in the parent, recursively until
either a `.curvroot` file is found, or until the filesystem root is found.

## Packages

A Package is a versioned collection of Curv source files that are distributed
over the internet as a unit. Packages explicitly declare their dependencies on
other packages. Inspired by package management in Debian and many other systems.

A Curv program can reference an external package using a URL and a version #.
Eg, `package{repo:"https://github.com/doug-moen/laser-curv",version:"1.0"}`.
Inspired by Rust and crates, it's distributed and decentralized.

The package mechanism is heavy weight. Extend `file` to accept a URL
argument, so that there is a simple way to reference remote resources?
(But I have a security concern: when and how often are these URLs fetched?)

When you evaluate a Curv program containing Package references, the UI
notifies you if you have unsatisfied dependencies, and asks you if they can
be downloaded. There is an 'upgrade' command for updating local copies of
packages. No internet access without an explicit user action is a security
feature of Curv.

Questions:
* How do I nest one package inside another? It's one way to satisfy a package
  dependency.
* Can a package be a shape? Or are they only meant to be libraries?
  How do I distribute a shape that consists of multiple files (eg, a Curv
  file and some 'assets' such as texture files)? A zip file is the
  best approach: you want shapes to be single files, and zip is the standard
  mechanism, eg OpenDocument `*.odf` or 3MF.
* How do I develop, test, run a package on my local file system?

Package metadata:
* **In-value metadata**. If the value of a Tree is a record, then metadata
  can be incorporated into the record value, using a naming convention.
  Use cases? Control how a shape is rendered. BOM metadata in a shape.
  These are shape-specific use cases, and not 'package' metadata.
* **Out-of-value metadata**. The most obvious consumer of 'package' metadata is
  the package manager, which doesn't need in-value metadata. A full description
  of the package, with author, licence, description text, keywords, an image,
  could be used to populate entries in a Curv package website (curvhub.org).
  Use a file `.metadata.json`.

## Local Packages

The Package mechanism uses URLs to name external packages.
What if you are disconnected from the internet and want to maintain a collection
of packages on a file system, old school.

You could use `package "file:/usr/local/curv/foo"`.
Or, use `file "/usr/local/curv/foo"`.
But those pathnames are not portable across systems maintained by different
administrators. An important consideration for portability across heterogenous
systems with no internet access.

Alternatively, `CURVPATH` is an environment variable containing a list of
absolute pathnames of directories. Eg, `CURVPATH=/usr/local/curv`.

`lib.foo` searches for a file with basename `foo` in `CURVPATH`, as specified
by the `file` function. If found, the file is loaded and evaluated and the
resulting value is returned.

## Standard Packages

What makes sense is to have a small standard library (std), then put
the remaining library abstractions into a collection of standard packages.
The standard library forms the outer scope of all source files, while
standard packages must be referenced explicitly. The standard library is
harder to evolve than the standard packages, since a package can be deprecated
as a whole and replaced by a new package with a different name. So it makes
sense to keep the standard library small.

How are standard packages referenced? Should we use `lib` (they are installed
on the local file system, as part of the Curv installation process),
or should we use `package` (they are referenced using URLs)?

In the long term, standard packages should be referenced by URLs, because
if they are part of the default install, then it becomes hard to abandon them
or remove them from the default install (backward compatibility reasons).
(Eg, the Python standard library is notoriously full of abandonware.)
But then, in the long term, we would want a stable URL for these packages.

I want standard packages now. What do I do?
* Put standard packages on github.
* `noise = package "https://github.com/doug-moen/noise.curv"`
* The `package` function will initially use a simple package manager that
  caches packages in `~/.cache/curv/`
* `curvpkg` subcommands: list, install, remove, upgrade

## Mutual Recursion
Mutually recursive references between two curv files within a package
is not supported: an error is reported. OTOH, what does work is defining a
library in file (exporting a record value), and including that library in
another file. That is a required feature.

Directory syntax is supposedly modelled as a record literal (w.r.t. scoping).
This suggests that mutual recursion could/should be supported. But that
creates technical difficulties, especially when we need to support including
another file. Curv does not let a record include a variable defined elsewhere
in the same file.

## Synthesis

Implement this combination of features:
* `file.foo` references a file `foo.*` or a directory `foo`,
  relative to the current directory, using "lexical scoping" lookup.
* File syntax: rules for converting files into Curv values, based on file
  extension.
* Directory syntax: rules for converting a directory into a Curv value.
  Optional `main.curv` entry. Optional `.curvroot` entry.
* `package{repo,version}`: Versioned, encapsulated packages, referenced using
  absolute https: or file: URLs, represented as git repositories.

How do you use these features, as a user?
* Create a single hierarchical workspace for all Curv projects: `~/curv`.
  Use file.name to reference external files.
* Lollipop tutorial.
  $ mkdir lollipop; cd lollipop
  $ create main.curv
  $ create param.curv
  $ create lib.curv
  Use `include file.param` and `include file.lib`.
* The curv/examples directory will change: it gains a `.curvroot` file,
  and uses `file.lib.experimental` to reference the library.
