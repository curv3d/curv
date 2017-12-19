Programs
========

A Curv program is typically stored in a source file
with the extension ``*.curv``.

Programs are expressions, which are evaluated to yield a result.

* In the common case, the result is a shape value.
* Another common pattern is to construct a record value,
  representing a set of definitions: either a library of
  reusable components, or a set of model parameters for a
  large, multi-source file model.

Here is an example of a source file ``lollipop.curv``,
that constructs a shape using some common idioms::

  let
  diam = 1;
  len = 4;

  in
  union(candy, stick)

  where
  candy = sphere diam >> colour red;
  stick = cylinder {h: len, d: diam/8} >> move(0,0,-len/2);

The idiom here is to put the model parameters at the top (using ``let...in``),
and place auxiliary definitions at the bottom (using ``where...``).
