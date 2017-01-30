# Time and Animation

In Curv, time is the fourth dimension.
Time is an extra parameter to distance functions and colour field functions.
An animation is a shape or colour field that varies in time.

Time is represented by a floating point number, measured in units of seconds,
like in ShaderToy. The zero point is arbitrary, and is not tied to clock time.
Eg, for a movie, the zero point is the beginning of the movie.

Animation is always "turned on". Individual shapes and colour fields can be
animated, in a modular way, without complicating their ability to be
included in larger assemblies. Like putting an animated GIF into a web page.

Time is relative. Since time is a coordinate, it can be transformed.
You can apply temporal transformations to speed up or slow down the passage
of time within a shape, loop over a specified time range, concatenate
a sequence of fixed length animations, etc.
* Or apply a transformation that creates motion trails from an animation.
* motion blur: https://www.shadertoy.com/view/MdB3Dw

Since time is a coordinate, animated 2D shapes are actually static objects
in 3D space-time, and animated 3D shapes are static objects in 4D space-time.
A 2D animated object can be trivially transformed into a 3D static object
where time runs along the Z axis, and vice versa.
* As a hack, you can repurpose this feature to build 4 dimensional static
  geometric objects.

Most objects have infinite extension along the T axis.
Most shapes aren't animated, and many animations are endlessly repeating loops.
But we include time in the bounding box, so that we can represent
fixed duration animations with a start and end time.

(I considered making time a global variable, like in OpenSCAD and Shadertoy,
like in Newtonian physics, but this design is more awesome.)

A new goal is to be able to import and export animated GIFs and video files.

A possible optimization is to perform a global analysis of the program,
and either remove the time coordinate completely (from generated GLSL code)
or convert it to a global variable. That's likely not worth the effort,
but we could benchmark that to be sure.

A more likely optimization is to partition code according to when it runs,
so that if you concatenate a sequence of fixed duration animations,
we run the code for each animation in sequence, instead of running
all of the animation codes in parallel.
