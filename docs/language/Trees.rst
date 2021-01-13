Trees
=====
Curv provides a simple `query language`_ for accessing and updating elements
of a hierarchical data structure.

In the simplest case, if ``a`` is a local variable containing
the list ``[1,2,3]``, then:

* ``a.[0]`` is the first element of the list, ``1``.
* ``a.[0] := 99`` is an assignment statement that updates the first
  element of ``a``, which now contains ``[99,2,3]``.

In the general case, ``a`` contains a hierarchical data structure.
If ``i`` is an index value, then:

* ``a.[i]`` denotes an element of the data structure,
  or a structured collection of elements of the data structure.
* ``a.[i] := new_elements`` updates those elements.

There is also a pure functional interface for updating a data structure.

::

  a.[i] := new_elements

is equivalent to::

  a := amend i new_elements a

.. _`query language`: https://en.wikipedia.org/wiki/Query_language

Tree Queries
------------
A **tree** is a hierarchical data structure made of nested lists and records.

An **atom** is a value that is neither a list nor a record. Atoms comprise
the leaf nodes of trees, and an atom is the degenerate case of a tree.
(Because of this, every value is a tree.)

An **index** is a value that traverses a tree and specifies a single
element (node) of a tree, or a structured collection of elements.

``tree.[index]`` returns the elements of ``tree`` specified by ``index``.
The ``tree`` is usually a list or record, but it can be *any value*,
since atoms are also trees.

``record.fieldname`` is a short form of ``record.[#fieldname]``.

``amend index new_elements tree`` is a function that (conceptually)
returns a copy of the ``tree``, in which the elements specified by ``index``
are replaced by ``new_elements``. In practice, this function is optimized
so that the tree is efficiently updated in place if there are no other
references to the value. This is called `copy-on-write`_.

* The ``tree`` is the final argument of ``amend`` so that you can use it
  in a pipeline like this: ``tree >> amend index elems``.

.. _`copy-on-write`: https://en.wikipedia.org/wiki/Copy-on-write

If ``tree`` is a local variable,
then ``tree.[index] := new_elements`` is a short form of::

  tree := amend index new_elements tree

The expression on the left side of the assignment can contain
a chain of indexing operations.

Tree Indexes
------------
A **natural number** indexes a list.
If ``list`` has ``n`` elements,
then ``list.[0]`` is the first element and ``list.[n-1]`` is the final element.

----------

A **symbol** indexes a record.
For example, if ``r`` is the record ``{a:1,b:2}``,
then ``r.[#a]`` is the element ``1``.

----------

A **list** of indexes selects zero or more elements from a tree,
yielding a list of element values.
For example, if local variable ``str`` contains ``"curv"``,
then ``str.[[0,3]]`` is ``"cv"``, and::

  str.[[0,3]] := "tn"

will modify ``str`` to contain ``"turn"``.
When you amend a tree using a list of indexes, then the *new_elements*
argument must be a list of the same length as the list of indexes.

----------

The special index value **this** specifies the entire tree.
``val.[this]`` returns ``val``, for any arbitrary value ``val``.
This may seem useless, but ``this`` has mathematical meaning as
the identity element for several index constructors, and ``this``
is used as the base case when constructing a compound index value
algorithmically, using iteration or recursion.

----------

A **tree path** is a sequence of indexes which are applied sequentially
to a tree.

* ``tpath index_list`` constructs a tree path from a list of indexes.
* ``tree.[tpath[i,j,k]]`` is equivalent to ``tree.[i].[j].[k]``.
* ``tpath`` is an associative operation::

    (i `tpath` j) `tpath` k == i `tpath` (j `tpath` k)
    tpath[tpath[i,j], k] == tpath[i, tpath[j,k]]

* The index ``this`` is the identity element for ``tpath``::

    tpath[this,i] == tpath[i,this] == i.
    tpath[] == this

----------

A **tree slice** is a kind of path used to extract `slices`_ from
multidimensional arrays. The main use case is described in `Arrays`_.

.. _`slices`: https://en.wikipedia.org/wiki/Array_slicing
.. _`Arrays`: Arrays.rst

Here's an example::

  let
    array =
      [ [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9] ]
  in
    array.[tslice[ [0,2], [0,2] ]]

This produces a new array containing only the corners of the matrix::

      [ [1, 3],
        [7, 9] ]

As an abbreviation, ``tree.[tslice[i,j,k]]``
can be written as ``tree.[i,j,k]``.

* ``tslice index_list`` constructs a tree slice from a list of indexes.
* ``tree.[tslice[i,j,k]]`` is equivalent to ``tree.[i,j,k]``.
* ``tslice`` is an associative operation::

    (i `tslice` j) `tslice` k == i `tslice` (j `tslice` k)
    tslice[tslice[i,j], k] == tslice[i, tslice[j,k]]

* The index ``this`` is the identity element for ``tslice``::

    tslice[this,i] == tslice[i,this] == i.
    tslice[] == this

.. Idea::
..
..   let
..     table =
..       [ {name: "Jack", age: 17, interests: [#curv, #cycling]},
..         {name: "Jill", age: 23, interests: [#cad, #design]},
..         {name: "Enid", age: 42, interests: [#curv, #art]} ]
..   in
..     table.[tslice[ [1,2], {#name, #age} ]]
.. 
.. produces a new table containing only some of the rows and columns::
.. 
..       [ {name: "Jill", age: 23},
..         {name: "Enid", age: 42} ]
.. 
.. Where ``{#foo}`` is ``{foo:#foo}``.
.. This means a record is an index, symmetrical with how lists are indexes.
.. 
.. However, this interferes with using plain records as 'classless OOP'
.. instances of application data types. I have to consider how user-defined
.. index types will be represented.
