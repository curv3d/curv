# Groups of Shapes
In OpenSCAD, a group is a list of shapes, except that groups exist in the
shape universe, not the value universe. With the flattening of the universes,
the most obvious way forward is to replace groups with lists.

In OpenSCAD2, groups are replaced by objects, which are richer than lists,
because they have fields (aka metadata). And I described why it's useful
to attach metadata to the user-defined nodes in a CSG tree. It provides a
clean way to introspect a shape tree, export metadata in a CSG file,
and generate a BOM. Also, my proposed syntax for this is backwards compatible
with OpenSCAD, so it all seemed pretty sweet at the time.

Closely related to this is the idea of prototype oriented programming
with parameterized geometric objects. The fields in an object have exactly
the same role here as they do in the case of "metadata": they are high level
model parameters, mostly.

Now, with objects having morphed into records and modules, what happened
to groups with metadata? Looks like modules replace OpenSCAD2 objects
in the role of parameterized geometric objects containing model parameters
and metadata. And the syntax has remained the same.

So, modules have a role as nodes in the CSG tree, so they need a way to
be serialized as JCSG (JSON syntax).
