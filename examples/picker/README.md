# Parametric Shapes
2018 Dec 4: First public preview of the Parametric Shape feature.
This is an experimental feature that will change based on feedback,
testing and bug fixes.

You can now create parametric shapes, which contain named parameters bound
to GUI widgets (like sliders), that you can manipulate in the Viewer window.
The shape animates in real time as you change parameter values.

## GUI Changes
The following program declares a shape parameter that is 
bound to a graphical slider widget:
```
  parametric {
      size :: slider(1,5) = 3;
  }
  cube size
```
When you run this program, the Viewer window contains a slider that lets
you vary the `size` parameter from 1 to 5, with an initial value of `3`.
For more examples, see the `*.curv` files in this directory.

The value picker widgets are contained in a window named `Shape Parameters`.
You can resize this window, move it, and minimize it by double clicking the
title bar. Use CTRL+H to hide or show the window.

Each value picker widget has a `(?)` help button that explains its user interface.

## Language Changes
To add graphical parameters to a shape in a Curv program,
you prefix a shape expression with a `parametric` clause:
```
  parametric { <parameter1>; <parameter2>; ... } <shape-expression>
```
This is an expression that returns a parametric shape value.

Each `<parameter>` has the form:
```
  <identifier> :: <picker> = <initial-value>;
```
A *picker* expression specifies the type and range of values of the parameter,
and also specifies what kind of value picker widget is used in the GUI.

Here are the currently supported picker expressions:
* `checkbox` -- A boolean parameter (true or false), represented by
  a checkbox widget.
* `colour_picker` -- An RGB colour value. The widget allows you to edit RGB
  or HSV colour components directly, or use a colour wheel to select colours
  visually.
* `slider(low,high)` -- The parameter is a number in a continuous range
  between `low` and `high`. A linear slider widget is used to set the number.
* `int_slider(low,high)` -- The parameter is an integer between `low` and
  `high`. A linear slider widget is used to set the integer.
* `scale_picker` -- The parameter is a *scale factor*: a number \> 0 and \< infinity.
  The widget lets you increase or decrease the value by dragging with the mouse,
  and the value changes according to a logarithmic (not linear) scale.
  This is the same logic used to modify the zoom factor in the Viewer window
  using a mouse scroll wheel or trackpad scroll gesture.

Parametric shapes (and pickers) are first class values that can be bound to a
variable, passed as a parameter to a function, or returned as a result.
Parametric shapes contain metadata that names their parameters, and allows
new versions of the shape to be constructed using modified parameter values.
One consequence is that you can create library functions that return
parametric shapes. This is in contrast to the OpenSCAD "customizer" feature,
which relies on structured comments in source code to identify parameters.
These comments can only appear in the top level source file.

## Bugs and Limitations
There is no way to save the parameter data.

The `color_picker` widget sets its parameter to an sRGB triple.
However, the internal representation for colour values in Curv is not
sRGB, but rather linear RGB. So you can't pass a colour parameter directly
to the `colour` operator, without using a conversion.
You could work around this by explicitly
converting a colour parameter `c` to linear RGB using `sRGB(c)`.
However, I think it is better if colour parameters are represented
directly as linear RGB.

The shape expression that follows a "parametric clause" must be compiled into
a GLSL shader program for execution on the GPU. This means that only a restricted
subset of Curv may be used. Right now, there are too many limitations. The
implementation is not yet complete.

In one case, I saw a performance hit for adding shape parameters. This may be
dependent on the quality of the GPU driver. A better GLSL code generator
would address the problem.

There is no way for a library function to take a parametric shape as an argument,
add more GUI parameters, and return an extended parametric shape as the result.

The widgets are somewhat ugly. I am using the ImGui widget library.

## TODO
Any parameter can be copied and then pasted as a Curv expression.

Here are some ideas for new picker types:
* `dropdown_menu(menu_items)` -- select one of N labelled alternatives.
* vector sliders -- the parameter value is a vector with between 2 and 4 elements.
  Each vector element is represented by a slider.
* angle -- an angle picker widget lets you graphically select an angle between `0`
  and `tau` using a circular pick area.
* bounded 2D vector -- specify a 2D vector by clicking on one of the points
  in a rectangular pick area.
* unbounded 2D vector -- specify a 2D vector by using pan and zoom gestures
  within a rectangular pick area.
* 3D direction -- a 3D picker widget that lets you specify a 3D normalized vector,
  visualized as a direction in 3D space. As seen in the AntTweakBar widget library.
* 3D rotation -- a 3D picker widget that lets you specify a 3D rotation,
  encoded as a quaternion. As seen in the AntTweakBar widget library.

Make the GUI look more beautiful and have a more intuitive UI.
Should I consider using a different widget library?
