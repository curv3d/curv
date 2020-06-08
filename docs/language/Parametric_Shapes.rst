Parametric Shapes
=================
A parametric shape contains named parameters bound to GUI widgets
that you can manipulate in the Viewer window.
The shape animates in real time as you change parameter values.

The following program declares a shape parameter that is bound
to a graphical slider widget::

    parametric
        Size :: slider[1,5] = 3;
    in
    cube Size

When you run this program, the Viewer window contains a slider that lets
you vary the ``Size`` parameter from 1 to 5, with an initial value of ``3``.

The value picker widgets are contained in a window named "Shape Parameters".
You can resize this window, move it, and minimize it by double clicking the
title bar. Use CTRL+H to hide or show the window.

Each value picker widget has a ``(?)`` help button
that explains its user interface.

Programming Interface
---------------------
To add graphical parameters to a shape in a Curv program,
you prefix a shape expression with a ``parametric`` clause::

    parametric <parameter1>; <parameter2>; ... in <shape-expression>

This is an expression that returns a parametric shape value.

Each ``<parameter>`` has the form::

    <identifier> :: <picker> = <initial-value>;

The ``<identifier>`` is displayed in the GUI as a label string.
If you want to include spaces or punctuation in the label, you can use a quoted
identifier, eg ``'Body Colour'`` instead of ``Body_Colour``.

A **picker** expression specifies the type and range of values of the parameter,
and also specifies what kind of value picker widget is used in the GUI.

Here are the currently supported picker expressions:

* ``checkbox`` -- A boolean parameter (true or false), represented by
  a checkbox widget.
* ``colour_picker`` -- An RGB colour value. The widget allows you to edit RGB
  or HSV colour components directly, or use a colour wheel to select colours
  visually.
* ``slider[low,high]`` -- The parameter is a number in a continuous range
  between ``low`` and ``high``. A linear slider widget is used to set the number.
* ``int_slider[low,high]`` -- The parameter is an integer between ``low`` and
  ``high``. A linear slider widget is used to set the integer.
* ``scale_picker`` -- The parameter is a *scale factor*: a number > 0
  and < infinity. The widget lets you increase or decrease the value by dragging
  with the mouse, and the value changes according to a logarithmic (not linear)
  scale. This is the same logic used to modify the zoom factor in the Viewer
  window using a mouse scroll wheel or trackpad scroll gesture.

Parametric shapes (and pickers) are first class values that an be bound to a
variable, passed as a parameter to a function, or returned as a result.
Parametric shapes contain metadata that names their parameters, and allows
new versions of the shape to be constructed using modified parameter values.
One consequence is that you can define library functions that return
parametric shapes.

..
  Details and Caveats
  -------------------
