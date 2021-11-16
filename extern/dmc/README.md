This code is derived from the TMC project <https://github.com/rogrosso/tmc>,
from the subdirectory `cpp_dmc`.

It has been fixed to compile using G++, and then changed from being "research"
code to being "product" code. Debug print output has been removed.

# tmc

**Construction of Topologically Correct and Manifold Iso-surfaces** \
Author: Roberto Grosso

Different algorithms for computing topologically correct and manifold iso-surfaces from volume data (hexahedral meshes or voxel grids) were implemented. The output surfaces are represented using an indexed face set (shared vertex) and a halfedge data structure. The acronym **tmc** is intended to mean *topologically correct and manifold iso-surface extraction by using a Marching Cubes like algorithm*.

## Folder cpp_dmc

This folder contains a C++ implementation of the *Dual Marching Cubes* method presented in [4],[3]. Mesh simplification only removes elements with vertex valence pattern 3X3Y. The code can certainly be optimized. The simplification of elements with vertex valence pattern, which are not so common, will follows in a later version of the code.

## References
[1]: Roberto Grosso: **Construction of Topologically Correct and Manifold Isosurfaces**. *Computer Graphics Forum 35(5):187-196 · August 2016*

[2]: Roberto Grosso: **An asymptotic decider for robust and topologically correct triangulation of isosurfaces: topologically correct isosurfaces**. *CGI '17 Proceedings of the Computer Graphics International Conference*. Japan, June 2017

[3]: Roberto Grosso, Daniel Zint: **Parallel reconstruction of quad only meshes
from volume data**. In: *Proceedings of the 15th International Joint
Conference on Computer Vision, Imaging and Computer Graphics
Theory and Applications* - Volume 1: GRAPP,, pp. 102–112.
INSTICC, SciTePress (2020).

[4]: Roberto Grosso, Daniel Zint:
**A parallel dual marching cubes approach to quad only surface reconstruction**.
The Visual Computer, (), 1-16.
DOI 10.1007/s00371-021-02139-w
