================
The Colour Atlas
================

This is a brainstorming/research document
that catalogues colour-related operations and technology
that I am considering for inclusion in Curv 1.0.

Colour Spaces and Gamuts
========================
Curv will initially just support the sRGB colour gamut,
like the majority of 3D graphics programs out there.
Full support for multiple colour spaces is too complicated
for release 1.0.

Colours are represented internally as linear RGB.
This is necessary for colour blending and lighting calculations
to work correctly.

Colours will normally be input using sRGB encoding,
then converted to linear RGB. For example, web colours
are represented as 8 bit sRGB.

Most image files (*.png, *.jpeg) use sRGB encoding.
On import, image files will by default be converted to linear RGB.
There will be a way for the user to override this, and declare that
an image file contains linear RGB or linear greyscale.

Currently, Curv 3D shapes compile to a GLSL fragment shader that
converts linear RGB to sRGB just before returning a colour value.
For consistency, 2D shape processing must be changed to work the same way.

I should make sure to use the correct code for converting linear RGB to sRGB.
Right now, I cheat and use "gamma 2.2":

  https://gamedevdaily.io/the-srgb-learning-curve-773b7f68cf7a

In the future, I'd like to switch to "linear rendering", where shaders
put linear RGB into the frame buffer, and the graphics driver/GPU performs
the conversion to the display hardware's prefered encoding.
This is now fully supported in WebGL 2.0 as of 2017, and has been supported
on desktops since at least 2008. It's not a requirement for the
1.0 release: it's basically a performance enhancement.

Tone mapping? (Post 1.0.)

Constructing Colour Values
==========================
We only really support the sRGB colour space.
There are several colour constructor functions, all of them
return a linear RGB triple ``(R,G,B)``, where the colour components
are in the range 0..1.

* ``sRGB(r,g,b)``. Each argument is in the range 0..1.
* Support for #abcdef web colour notation.
  Maybe ``#ABCDEF`` is parsed as a token,
  and compiled into a list literal, ``(0xAB/255, 0xCD/255, 0xEF/255)``.
  So you would write ``sRGB #ABCDEF``.
  Maybe I should support hexadecimal integer literals.
* Support for sHSV and sHSL colour constructors (which are linear transformations of sRGB coordinates).
* Perceptually uniform colour spaces like HCL are cool, but are not supported in the Curv 1.0 standard library,
  because it's a tar pit. (Somebody else can write a 3rd party library.)
* Look at CSS and SVG for some ideas.

Names for standard colours.
Currently I've got white, black, red, green, blue, cyan, magenta, yellow, and orange.
The values I've chosen don't align with other colour naming standards.

Standard colour names are a giant tar pit. Look at what CSS and SVG are doing,
and adopt a simplified subset.

Colour Blending
===============
Define what colour blending actually means.
It'll be whatever is cheap to implement using linear RGB on a GPU,
and it won't be "perceptually uniform", but it will probably be good enough.

I will have to think about how colour blending works for exporting to file formats
for 3D printing and traditional dye/pigment printing on flat media.
Probably that's another tar pit that I don't enter for release 1.0?

``smooth_union r`` will perform colour blending within the blending radius ``r``.

There should be a colour blending variant of ordinary "sharp" ``union``.
Call this ``blended_union``?

Maybe there's a whole family of blending operators that can be used
with ``blended_union``??

Gradients and Stuff
===================
CSS and SVG have "gradients". Look for APIs I can borrow and adapt.

A "colour map" is a mapping from the range 0..1 to a colour.
They are much used in scientific/data visualization.
Come up with a library of popular/useful colour maps.
Which would include continuous gradients between two end-point colours.
See also "heat map".

Image Import
============
You can import an image file (*.png or *.jpeg).
Geometrically, it's a rectangle, with a colour function that uses bicubic interpolation
to map the pixel RGB values onto a continous function.

No support for alpha values, because I don't know what that means in Curv.

By default, image files are assumed to contain sRGB encoding.
The colour values are automatically converted to linear RGB.
So when you query an image file's colour function, you get linear RGB.
Alpha
=====
RGBA encoding and Duff/Porter compositing?
I dunno about this in Curv 1.0.
How does this map onto the idea of solid modelling and geometric shapes?
What is the signed distance field for an RGBA image?
Maybe later, if I come up with a theory to make sense of this stuff.
