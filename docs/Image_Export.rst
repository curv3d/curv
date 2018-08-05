Image Export
============

To export a 2D shape to a PNG file, use::

   curv -o foo.png [options] foo.curv

This exports a rectangular image of everything inside of the shape's bounding
box. Pixels outside the shape's boundary are set to white. The aspect ratio
of the exported image is determined by the bounding box.

You can control the bounding box, and add a border around the shape,
by writing Curv code.
For example, to export a unit circle with a border thickness of `0.1`::

    let
    shape = circle;
    in
    shape >> set_bbox [shape.bbox[MIN]-0.1, shape.bbox[MAX]+0.1]

The size of the image in pixels is determined by command line options::

   -O xsize=<width of image in pixels>
   -O ysize=<height of image in pixels>

These are both optional. If neither is specified, the larger dimension is set
to 500 pixels.
