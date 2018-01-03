===============
The Colour Atlas
================

This is a brainstorming/research document
that catalogues colour-related operations and technology
that I am considering for inclusion in Curv 1.0.

Data Types
==========
An Intensity is a number between 0 and 1 inclusive.

A Colour comprises red, green and blue Intensities.

A Colour Map is a function mapping an Intensity onto a Colour.

A 2D or 3D Intensity Field maps a point (x,y) or (x,y,z) to an Intensity.

A 2D or 3D Colour Field maps a point (x,y) or (x,y,z) to a Colour.
If you compose a Colour Map with an Intensity Field you get a Colour Field.

Colour Spaces and Gamuts
========================
Curv will initially just support the sRGB colour gamut,
like the majority of 3D graphics programs out there.
Full support for multiple colour spaces is too complicated
for release 1.0.
HDR (high dynamic range) and WCG (wide colour gamut) displays
are available, but full support will need to wait.

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
* Support ``sHWB(hue,whiteness,blackness)``, because it's the most intuitive way to pick colours
  (better than HSV or HSL), as popularized by CSS Color Module Level 4.
  The ``s`` prefix means it is a transformation of sRGB coordinates.
  http://alvyray.com/Papers/CG/HWB_JGTv208.pdf
* Support ``sRGB.HSV(hue,saturation,value)`` for compatibility with other programming languages (for porting code).
* CSS has HSL, but it's slightly harder to use than HWB or HSV. Do I need it?
* CSS colour modification operators. Look at CSS Color Module Level 4. Looks like a nice alternative to hexadecimal
  or databases containing thousands of obscure colour names for naming/constructing colours.
* Perceptually uniform colour spaces like HCL are cool, but are not supported in the Curv 1.0 standard library,
  because it's a tar pit. (Somebody else can write a 3rd party library.)
* Look at CSS and SVG for some ideas.

Names for standard colours.
Currently I've got white, black, red, green, blue, cyan, magenta, yellow, and orange.
The values I've chosen don't align with other colour naming standards like X11.
But, they are intended to work with colour modification operators (see above).

Standard colour names are a giant tar pit.
http://people.csail.mit.edu/jaffer/Color/Dictionaries

Look at what CSS and SVG are doing,
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

Colour Gradients
================
CSS and SVG support `colour gradients`_. Look for APIs I can borrow and adapt.

.. _`colour gradients`: https://en.wikipedia.org/wiki/Color_gradient

Separate the shape of a colour gradient from the sequence of colours used.

* An "intensity field" is a function mapping points in space onto an intensity (a number in the range [0...1]).
* A "colour map" is a mapping from an intensity to a colour.
  They are much used in scientific/data visualization.
  Come up with a library of popular/useful colour maps.
  Which would include continuous gradients between two end-point colours.
  See "Good Colour Maps: How to Design Them", https://arxiv.org/pdf/1509.03700.pdf
* A "colour field" is the composition of an intensity field with a colour map:
  it's a function mapping points in space onto colours.

Colour Field Transformations
============================
A colour field can be transformed freely, without the limitations of shape transformations,
since you don't need to ensure that the result is Lipschitz continuous, as you do with distance fields.

Provide a library of useful colour field/intensity field transformations.

Image Import
============
You can import an image file (*.png or *.jpeg) as a 2D shape.
Geometrically, it's a rectangle, with a colour function that uses bicubic interpolation
to map the pixel RGB values onto a continous function.

No support for alpha values in Curv 1.0, because I don't know what that means.
It could mean several things, all of which are research projects:

* Alpha values encode translucency in shapes using an opacity field, see Translucent Shapes.
* A zero alpha value means that the pixel is not part of the shape, so it
  modifies the distance field, instead of modifying the opacity field.
* 0 < alpha < 1 on the edge of an opaque shape encodes antialiasing, we use that
  to interpolate where the actual edge of the shape is. So the distance
  field is affected.

By default, image files are assumed to contain sRGB encoding.
The colour values are automatically converted to linear RGB.
So when you query an image file's colour function, you get linear RGB.

You can import a greyscale image that is to be interpreted as an intensity field.
No gamma correction is applied.

Translucent Shapes
==================
In "Constructive Volume Geometry", Chen proposes to extend volumetric
shape representations (like what Curv uses) with an opacity field, a function
mapping each point in space onto an opacity value from 0 to 1.
So you can represent translucent shapes.
http://www.cs.swan.ac.uk/~csjvt/JVTPublications/CVG-Forum(published).pdf

Not for Curv 1.0.

Materials
=========
You can associate a material with a shape, which controls how lighting works
for the surface of the shape. Eg, matte vs glossy. Not a requirement for Curv 1.0.

Lighting Model
==============
You can control the lighting model parameters from inside Curv.
The main challenge is designing the API.

Recursive Rendering
===================
Invoke the 3D renderer as an operation in Curv for converting
a 3D shape to a 2D coloured rectangle.
