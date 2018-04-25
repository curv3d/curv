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

## Files

Using the File mechanism, you can directly reference a file by name.
The filename is either a pathname in the local file system, or a URL.
```
  file "pathname_or_url"
```

The `file` function converts a file to a Curv value based solely on the filename
extension, or maybe on the MIME type in the case of a URL.

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

Some file types that might be supported:
* `*.curv` -- a Curv expression, which evaluates to an arbitrary value.
* `*.json`
* `*.toml`
* `*.rsdf` -- a Regularly Sampled Distance Field -- a voxel grid of distance
  values, in binary.
* *directory* -- if a local filename names a directory, then Tree syntax is
  used to interpret the directory as a Curv value. Not supported for URLs.
* `*.cpkg` -- a ZIP file containing a Tree (similar to *.ODT or *.3MF). The
  primary use case is to represent a Curv shape as a single file, in the case
  where we want to package Curv source code together with some binary files.

The shell command `curv filename` interprets `filename` the same way as the
`file` function does, reading and evaluating the file and then displaying the
resulting Curv value. URLs are supported here.

URL support in the File mechanism might be a security hole. It may provide a
backchannel for malware to "phone home". Packages are more secure.

Mime types:
* `*.curv` == text/curv
* `*.rsdf` == application/curv.rsdf
* `*.cpkg` == application/curv.cpkg

## Trees

Curv has a 'directory syntax', which interprets a directory tree as a nested
record value. Individual files within a directory, if their names have the
syntax 'identifier.extension', are interpreted as record members: the member
name is 'identifier', and 'extension' specifies how the file is interpreted as
a value, as if by the `file` function. As a special case, subdirectories whose
names are just 'identifier' (no extension) are interpreted as record values.

The root of the directory tree is marked, possibly by an empty file `.curvroot`.

Trees are encapsulated. You must use the Package mechanism to reference
resources outside of the Tree. If `file` is used by a `*.curv` file in a Tree,
you can only use relative pathnames, and you can't use `..` to reference files
outside the Tree.

The purpose of Tree syntax is to provide a local file system representation
of `*.cpkg` files and Packages. That's why Trees are encapsulated.

Within *.curv files in the Tree, other members of the tree can be referenced
using lexical scoped identifiers. A directory containing files 'foo', 'bar', etc
is semantically equivalent to a record '{foo=..., bar=..., ...}'. The parent
scope of the root directory is 'std', the standard namespace. This reference
mechanism doesn't provide any additional expressive power over `file`, it's just
nicer and more convenient.

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

A possible extension: `.include.curv` evaluates to a record whose members are
added to the record denoted by the directory.

Do we support private vs public members of a Tree?
* An easy solution: use a naming convention. Identifiers beginning with `_`
  are private.

Many modern languages now have a standard tree/package/project manager
that will create a project tree for you, then perform operations on that
project tree. Often with git integration. Examples:
* Rust, `cargo`
* Clojure, `lein`

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
But then, in the long term, we would want a stable home for these packages.

## Conclusion

Trees and Packages suffice. Don't actually need `file` or `lib`.
