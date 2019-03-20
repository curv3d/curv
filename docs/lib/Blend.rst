``lib.blend`` defines a collection of blending operators,
which combine two shapes to create a new shape.
They work in 2D and in 3D.

Some of these operators are generalized Boolean operations.

* A blended union is a generalized union that adds additional material, such as a fillet or chamfer,
  at the edges where two shapes meet.
* A blended intersection or difference removes material from the edges where two shapes meet,
  which can produce a rounded or chamfered edge.

Summary of the blended Booleans:

=========  =============
Name       Effect
=========  =============
smooth     Rounded or filleted joins and edges
chamfer    Chamfered joins and edges
stairs     Staircase effect
columns    Scalloped effect
=========  =============

Summary of the non-Boolean blends:

=========  =============
Name       Effect
=========  =============
pipe       Produces a cylindical pipe that runs along the intersection.
           No objects remain, only the pipe.
engrave    First object gets a v-shaped engraving where it intersects the second.
groove     First object gets a carpenter-style groove cut out.
tongue     First object gets a carpenter-style tongue attached.
=========  =============

Boolean Blends
==============
Smooth:
  |uRound| |iRound|

Chamfer:
  |uChamfer| |iChamfer|

Stairs:
  |uStairs| |iStairs|

Columns:
  |uColumns| |iColumns|

.. |iChamfer| image:: ../images/fOpIntersectionChamfer.png
.. |iColumns| image:: ../images/fOpIntersectionColumns.png
.. |iRound| image:: ../images/fOpIntersectionRound.png
.. |iStairs| image:: ../images/fOpIntersectionStairs.png
.. |uChamfer| image:: ../images/fOpUnionChamfer.png
.. |uColumns| image:: ../images/fOpUnionColumns.png
.. |uRound| image:: ../images/fOpUnionRound.png
.. |uStairs| image:: ../images/fOpUnionStairs.png


Technical Details
-----------------
Blending operators are distance field operations,
and are thus subject to the constraints described here:
`<../shapes/Distance_Field_Operations.rst>`_.

A family of blended Boolean operators is represented by a "blending kernel",
which is a record containing 3 functions named ``union``, ``intersection`` and ``difference``.

For More Information
--------------------
This document isn't finished yet. For more information, see:

* `<../shapes/Distance_Field_Operations.rst>`_
* The Blending section of `<../Theory.rst>`_
* `<http://mercury.sexy/hg_sdf/>`_, which inspired many of the operations in ``lib.blend``.
* The source code, `<../../lib/curv/blend.curv>`_.
