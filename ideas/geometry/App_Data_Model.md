# The Application Data Model

In OpenSCAD, the "metadata problem" is really about the ability to export
a high level description of a geometric model, as a tree of app-defined
components, each node labelled with the node type and parameters for that
node.

This solves the "bill of materials" problem, and provides a path for exporting
to another high level geometry format. GDML is being discussed on the OpenSCAD
mailing list right now: it's a domain specific Geometric Description Markup
Language used by CERN to describe particle detector geometry. Obviously we
don't hard code support for every app-specific format out there. Instead, we
provide the ability to describe an app-specific format and export a model
as a JSON tree with that structure (which I'll call JCSG). An external tool
converts the JSON to the custom format (XML or whatever).

Also, we'd like to import JCSG.

A shape object optionally contains information for exporting the shape as JCSG.
This is a type string (which is the name of a function for constructing a shape
of this type), plus fields which are the parameters to this function.

## Implementation
I've considered using a naming convention for identifying the parameter fields.
(Another proposal is a 'param' keyword for modules.)

The OpenSCAD2 idea of prototype-oriented programming and customizable modules
is one approach. I've run into design difficulties with that.
* Conflict between the proposed a(i) array indexing syntax and module
  customization. Although, the current a'i array indexing syntax resolves this.
* Conflict between shape attributes and module fields occupying the same
  namespace.

We don't *need* prototypes and customization. Shape constructor functions can
be distinct from shapes, which avoids the conflict with function call syntax.
It's a simpler design. (But remember, I fixed this problem.)

Let's also try using a naming convention to mark model parameters.
That also feels like a simpler design (no new language features).
* The constructor takes a record as an argument, or accepts unordered keyword
  arguments like OpenSCAD. Otherwise, we support calling a constructor with
  positional arguments, and the model parameters need to be ordered.

// for a primitive shape
square(n) = make_shape {
    $type = "square",
    $size = n,
    dist p = ...,
    bbox = ...
};

// For a user defined composite shape, not using the make_shape protocol.
popsicle(n) = emshapen {
