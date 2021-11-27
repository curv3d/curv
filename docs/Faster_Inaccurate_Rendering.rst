Faster Inaccurate Rendering
============

It's desirable in limited environments to increase performance as much as possible
in order to properly work. Below are options and techniques which can be used
to do that.

* Reduce the size of the viewer window.
* Pass `-O stoch=p`, where p is the chance of a pixel rendering. This enables
"stochastic rendering mode" which fills in the screen each frame with randomly
sampled pixels.
* Build your scene piece by piece in separate files, then combine them using
`include` in the final file.
* Render only the bounding boxes of shapes until you are ready to render the
actual shape.
