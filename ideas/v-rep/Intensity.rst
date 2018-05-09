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
