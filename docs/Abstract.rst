Curv: a language for making art using mathematics
=================================================

Curv is an open source 3D solid modelling language, oriented towards 3D printing,
procedurally generated art, and mathematical visualization.

It's a pure functional language where geometric shapes are first class values,
and are constructed by transforming and combining simpler shapes using an
unusually rich collection of operators.

Instead of polyhedral meshes or other boundary representations, Curv represents
shapes as pure functions (Function Representation or F-Rep). This is a volumetric
representation, where a function maps every point (x,y,z) in 3D space onto the
properties of a shape. This representation is powerful, supporting a wide range
of shape operators, and is a good match to the volumetric nature of 3D printing.

F-Rep is well suited to being directly rendered by a GPU. To achieve this,
Curv code is compiled into GPU shader programs or compute kernels.
