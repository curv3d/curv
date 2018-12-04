# Parametric Shapes
2018 Dec 4: First public preview of the Parametric Shape feature.
This is an experimental feature that will change based on feedback,
testing and bug fixes.

You can now create parametric shapes, which contain named parameters bound
to GUI widgets (like sliders), that you can manipulate in the Viewer window.
The shape animates in real time as you change parameter values.

Here is an example of how to declare a shape parameter that is 
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
* `scale_picker` -- A 'scale factor' is a number \>0 and \<infinity.
  The widget lets you increase or decrease the value by dragging with the mouse,
  and the value changes according to a logarithmic (not linear) scale.
  This is the same logic used to modify the zoom factor in the Viewer window
  using a mouse scroll wheel or trackpad scroll gesture.

In Curv, a parametric shape is a first class value that can be bound to a
variable, passed as a parameter to a function, or returned as a result.
Parametric shapes contain metadata that names their parameters, and allows
new versions of the shape to be constructed using modified parameter values.
One consequence is that you can create library functions that return
parametric shapes. This is in contrast to the OpenSCAD "customizer" feature,
which relies on structured comments in source code to identify parameters.
These comments can only appear in the top level source file.

## Bugs and Limitations

## TODO
