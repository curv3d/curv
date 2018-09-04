Image Export
============

To export a shape to a PNG file, use::

   curv -o foo.png [options] foo.curv

Use the ``-v`` flag to get progress and render time information written to stderr.

Exporting a 2D Shape
--------------------
This exports a rectangular image of everything inside of the shape's bounding
box. Pixels outside the shape's boundary are set to white.

The size of the image in pixels is determined by command line options::

   -O xsize=<width of image in pixels>
   -O ysize=<height of image in pixels>

These are both optional.

* If one of these values is specified, the other value is computed so as
  to preserve the aspect ratio of the shape's bounding box.
* If neither is specified, the aspect ratio is preserved
  and the larger dimension is set to 500 pixels. 
* If both ``xsize`` and ``ysize`` are specified, then the shape is
  "letterboxed" inside the image, adding a horizontal or vertical white border if necessary to
  preserve the shape's aspect ratio.

You can control the bounding box, and add a border around the shape,
by writing Curv code.
For example, to export a unit circle with a border thickness of `0.1`::

    let
    shape = circle;
    in
    shape >> set_bbox [shape.bbox[MIN]-0.1, shape.bbox[MAX]+0.1]

Exporting a 3D Shape
--------------------
The results are similar to creating a screen shot of the Viewer window.

* If neither ``xsize`` nor ``ysize`` is specified,
  the image size defaults to 500×500 pixels.
* If exactly one is specified, you get a square image.

There are no options yet for controlling the camera position,
so for now I modify the source code:
I rotate the shape and shrink the bbox using ``set_bbox`` to zoom in.

Spatial Antialiasing
--------------------
By default, images are exported using 4× spatial antialiasing.
This provides good results in most cases.
You can override this using::

    -O aa=<supersampling factor>

where ``1`` means no anti-aliasing.

Export an Animation Frame
-------------------------
You can export a single frame from an animation of a time varying shape
by using::

    -O fstart=<frame start time, in seconds>

``fstart`` defaults to 0.

Exporting an Image Sequence
---------------------------
If you want to make an animated GIF or a video file,
then you can export a PNG image sequence using these options::

    -O animate=<length of animation, in seconds>
    -O fdur=<animation frame duration, in seconds>

and by including a ``*`` in the name of the output file.
The ``*`` is replaced by a sequence number in each of the image files
that is exported. Use ``-O fstart=`` if you don't want the animation to
start at time 0.

For example, try this::

    curv -o "*.png" -Oxsize=300 -Oanimate=tau -Ofdur=1/25 examples/pulsate.curv

Numeric -O arguments are Curv expressions. The ``pulsate.curv`` demo uses
``sin t`` to vary its shape over time ``t``, and ``tau`` (6.28)
is the period of the ``sin`` function, so this animation repeats
every 6.28 seconds. We use ``-Ofdur=1/25`` to specify
a frame rate of 25 frames per second.

Once you have an image sequence, you can convert this other formats
using many different third party tools and web services.

To convert the pulsate image sequence
to an animated GIF using Image Magick 7, try::

    magick -delay 0.04 *.png pulsate.gif

GIF is the worst available animation file format. The files are 20× larger
than they should be, and there can be serious artifacts due to compressing
the number of colours down to 256. However, many popular web services will
allow you to post animated GIFs, but they won't let you post video files.
So the animated GIF format persists.

To convert the pulsate image sequence
to a WEBM file using ``ffmpeg``, try::

    ffmpeg -i %03d.png -r 25 pulsate.webm

WEBM is a popular, open source and royalty free
video file format. The pulsate video needs to be looped, but you enable that
in your viewer, or in your HTML5 ``<video>`` tag, not in the WEBM file itself.

Temporal Antialiasing
---------------------
As an advanced feature, you can turn on temporal antialiasing using::

    -O taa=<supersampling factor>
    -O fdur=<animation frame duration, in seconds>

It is disabled by default. This works for exporting either image sequences,
or individual frames.

Whether temporal antialiasing improves the quality of an animation
depends in part on the frame rate. Consider that spatial antialiasing
only improves image quality if the pixels are too small to see.
If you blow up the pixels so that they are huge, then the resulting
image just looks blurry. Likewise, TAA can make an animation look better
by removing quantization artifacts, but it is best used when the frame rate
is too high for you to perceive the individual frames, otherwise you will see
exaggerated motion blur. So, you wouldn't use this for a low frame rate
animated GIF, unless you are using TAA as a hack to create an artistic effect.
