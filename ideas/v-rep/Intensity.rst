Intensity Fields
================
In Curv, Intensity Fields are a standard data type.
An Intensity Field maps (x,y,z,t) onto an Intensity value between 0 and 1.

3D medical images use voxel density grids (which are just sampled Intensity
Fields). Isosurfaces are extracted to identify features (eg, bone is denser
than other tissues).

Cinematic 3D CGI uses VDB to represent scenes as density grids. Translucent
phenomena like smoke and fire have density < 1.

Curv's model of reality is derived from 3D printing. Smoke and fire is not
3D printable, and can't be represented by a SDF. However, a CGI scene containing
translucent elements like smoke and fire can be represented inside of a
translucent, multi-colour and multi-material print (like from a J750 printer).
So, Curv needs a representation for this. Maybe add an alpha component to
colour fields?

Voxel grids will soon become an important alternate to triangle meshes as
a file representation for 3D print jobs. The Shapeways SVX format uses
density grids to represent geometry, and Curv may one day need to export
this kind of format (eg, maybe 3MF will also support density grids for
geometry).

In order to print a Mandelbulb fractal, I need to sample the distance field.
I need signal processing to remove aliasing and noise, otherwise the print
looks noisy. The output of this signal processing is a density grid that
represents geometry, which must then be converted to a 3D printable file format.

It would be useful (for testing at least) to be able to export this geometry
density grid directly. This now seems like a good idea, based on the above.

fractal mesh export:

  * How Mandelbulb https://www.thingiverse.com/thing:26324 was constructed:
    * export as 2048^3 density grid, 1 bit per voxel
    * resample down to 512^3 density grid, 8 bits per voxel
      * 2D median filter (each slice)
      * resample bilinearly to 1024^2 (each slice)
        and every 2 frames were averaged together -> 1024^3
      * 3D median filter
      * trilinear resampling to 512^3
    * My version of the resampling:
      * export as 2048^3 density grid, 1 bit per voxel.
      * 3D median filter, 3x3x3 neighbourhood, zero padding at grid boundaries.
        This means: find median of the 9 values: sort and take middle value.
      * trilinear resampling down to 1024^3.
        This means: take average of each 2*2*2 partition.
      * another 3D median filter.
      * trilinear resampling down to 512^3.
    * marching cubes
  * density grid export.
  * density grid tools: import, view, convolve, export. inviwo.org I guess.
    Experiment with different convolution methods.

Intensity Fields:

* In Curv, intensity fields are first class values. I could add new operations:
  * Import and export of intensity fields as voxel grids.
  * Convert density grid to SDF by extracting a level set.
  * Convert SDF to density grid by sampling and antialiasing.

Density Grid File Formats
-------------------------
An interchange format for exchanging density grids for 3D printing.
Requirements:

* Tool support for reading, viewing and transforming a density grid.
  Ideally, a program like MeshLab for density grids.
* C/C++ library support.
* Metadata for mapping voxels onto world coordinates. Voxel size and origin.
* A density value is an integer, encoding a real number between 0 and 1.
  Support for, eg, 1,2,4,8,16 bits per voxel.
  Floating point voxel value support is not required.
* Good compression, file sizes competitive with triangle meshes.
* Rapid access to slices.

Compression:

* SVX uses an array of PNG files in a ZIP archive.
  On average, SVX is 48% the size of binary STL, for a corpus of Shapeways
  submitted objects.
* Aaron Trocola's investigation: http://40westdesigns.com/blog/?p=371
  and https://www.engineering.com/3DPrinting/3DPrintingArticles/ArticleID/4523/2012-The-Beginning-of-the-End-for-Polygons.aspx
  * "The very intricate lattice structures often used in AM parts have so much
    surface area that the memory footprint of a polygonal representation exceeds
    the voxel one by many times."
  * For the MandelBulb (https://www.thingiverse.com/thing:26324):
    * 512^3 * 8 bits, RAW: 131MB uncompressed, 10MB zipped, 22MB as a zipped
      array of PNG files.
    * STL: 320MB, 133MB compressed. PLY: 132MB, 50MB compressed.
* RAW voxel data compresses very well.
* VDB is a sparse voxel grid, which is a kind of compression. In theory, this
  is most efficient for a convex shape (the case where voxels have the least
  advantage over meshes), and least efficient for lattices with large amounts of
  surface area per unit volume (the case where voxels are most compelling).
  How well does ZIP compression work on VDB?
* The Blosc library supports compression/decompression of large data sets
  "faster than memcpy". The trick is to stream data in/out of CPU cache without
  uncompressed data ever touching RAM. It is block oriented, and gains speed at
  the expensive of more disk space--the overhead of containers and blocks.

Existing file formats:

* .3MF may get voxels in the future.
* .SVX is a proposed 3DP voxel format, but little tool support.
  Has the right metadata: gridSize,voxelSize,origin. 1-16 bits per pixel.
  Compression: choice of BPP, each slice is a PNG file.
  https://abfab3d.com/svx-format/
* .PVM; support by: Inviwo, V^3 (www.stereofx.org/volume.html).
  1 byte per voxel. 6 numbers: X,Y,Z voxel count, Xs,Ys,Zs voxel size.
* .DF3 povray; 1,2,4 bytes per voxel.
* .VDB: popular for cinema CGI, endorsed by Brad & Matt for 3DP use.
* RAW: Lots of web references to this file format.
  * For Trocola's Mandelbulb, it is a headerless array of 8 bit voxel values,
    512^3 bytes in this case.
  * For 3D printing, the Z axis is up, and we want each slice to be contiguous,
    so Z coordinates should vary the slowest and X coordinates the quickest.
    Bottom slice should be first (smallest Z coordinate).
  * Tool support (8 bit RAW voxel array): Inviwo, Blender, ...

New file format:

Most of these file formats don't have all of the required metadata.
If I need external metadata anyway, then I could use a pair of files:
RAW data plus a JSON file containing all of the metadata:
  { "grid_size":[gx,gy,gz], "voxel_size":[vx,vy,vz], "origin":[ox,oy,oz] }
Initially, only support 8 bit values. Later, add a "bits_per_voxel" attribute.
The JSON file extension is RSIF (regularly sampled intensity field).
Put this into a ZIP archive for compression (eg, a Curv VSTOR file).
Or put it in a directory 'foo' containing foo/main.rsid and foo/main.raw,
which is an example of Curv tree syntax.

RGBA: RGB + Alpha
-----------------
The RGBA colour representation is used with 2D and 3D images.
Also, Duff-Porter compositing is a thing.

How does this fit into Curv?

* RGBA is a reasonable representation for the colour field of a 3D printed
  full-colour shape with transparent and translucent voxels.
* If we do this, then Duff-Porter is used to generalize the Union operator
  when it blends the colour fields of overlapping shapes.
* RGBA can be used to represent non-rectangular images -- the opacity is 0
  outside the shape, is 1 in the interior, and is >0 and <1 for
  partially occupied pixels on the border. We can extract the geometry from
  this representation by extracting a level set and constructing a SDF.
