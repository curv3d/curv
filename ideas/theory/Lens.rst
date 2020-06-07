Lens
====
A Lens gives access to a part of a larger data structure.
Given a Lens value, you can fetch that part of a data structure, or you
can update the part with a new value.

Curv provides simple lenses (for accessing the i'th element of a list,
or a specific field within a record). It also provides operations for
constructing compound lenses out of simpler lenses (for example, you
can chain together a sequence of lenses into a "path" for traversing
a nested data structure).

Based on just this, you might conclude that Lens values are a query language
for Curv data structures, analogous to JSONPath for JSON, or XPath for XML.
And you would be correct, but Lenses are more powerful than this, because
unlike JSONPath or XPath, the set of Lens constructors isn't fixed:
you can define new Lens constructors by writing Curv code.

In general, lenses can be arbitrary bidirectional transformations of data.
When you fetch the data named by a lens from a data structure,
that is the "forward" direction of the transformation. 
When you update the data named by a lens, that is the "reverse"
direction of the transformation.

Example: fahrenheit and celcius, or, colour spaces.

modularity: lens values provide an abstract interface to data, without
being part of the data (as would happen in OOP style programming).

functions as unidirectional lenses

relevance to Curv:

 * colour spaces
 * FRP (synchronizing the model and the view without using shared mutable state and callbacks)

Lens Reference
--------------

For example, an integer ``i`` is a Lens: it is an index into a List,
and selects the ``i``'th element of the list.

Bibliography
------------
The Boomerang programming language: https://www.seas.upenn.edu/~harmony/

   Lenses are well-behaved bidirectional transformations. Every lens program,
   when read from left to right, describes a function that maps an input to
   an output; when read from right to left, the very same program describes
   a "backwards" function that maps a modified output, together with the
   original input, back to a modified input.

The original paper on Lenses:

 * Relational Lenses: A Language for Updatable Views
 * https://www.cis.upenn.edu/~bcpierce/papers/dblenses-tr.pdf
 * We propose a novel approach to the classical view update problem. The
   view update problem problem arises from the fact that modifications
   to a database view may not correspond uniquely to modifications on the
   underlying database; we need a means of determining an “update policy”
   that guides how view updates are reflected in the database. Our approach
   is to define a bidirectional query language, in which every expression
   can be read both (from left to right) as a view definition and (from
   right to left) as an update policy.
