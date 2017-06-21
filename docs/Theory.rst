What is Curv?
=============
|twistor| |shreks_donut|

.. |twistor| image:: images/torus.png
.. |shreks_donut| image:: images/shreks_donut.png

Curv is an open source 3D solid modelling language, oriented towards 3D printing, procedurally generated art, and mathematical visualization.

It's easy to use for beginners. It's incredibly powerful for experts.

Curv is fast: all rendering is performed by the GPU.

Pure Functional Programming
===========================
Curv is a pure functional language. Why?

* simple, terse, pleasant programming style
* simple semantics
* can easily be translated into highly parallel GPU code

geometric shapes are first class values, and are constructed by transforming and combining simpler shapes.

F-Rep, not B-Rep
================
Instead of polyhedral meshes or other boundary representations, Curv represents shapes as pure functions (Function Representation or F-Rep). This is a volumetric representation, where a function maps every point (x,y,z) in 3D space onto the properties of a shape. This representation is powerful, supporting a wide range of shape operators, and is a good match to the volumetric nature of 3D printing.

F-Rep is well suited to being directly rendered by a GPU. To achieve this, Curv code is compiled into GPU shader programs or compute kernels.
