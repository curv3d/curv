The Viewer Window
=================
The Viewer window is a graphics window for viewing a 2D or 3D shape.

Opening a Viewer Window
-----------------------
There are multiple ways to open a viewer window from the command line.

1. In batch mode, you run a Curv program directly from the shell prompt.
   When you close the Viewer window, the ``curv`` command will exit.

   * ``curv myprogram.curv`` -- Evaluate a Curv source file and view its shape.
   * ``curv -x "cube >> colour red"`` --
     Evaluate a Curv expression, and view the resulting shape.

2. In live editing mode, a text editor window and a viewer window are opened.
   Each time you save the file from the text editor, the Viewer window is updated.
   When you close the text editor window, the ``curv`` command will exit.

   * ``curv -le myprogram.curv``

3. You enter interactive shell mode by typing ``curv`` with no arguments.
   At the ``curv>`` prompt, when you type a shape expression,
   a Viewer window is opened, or the already open Viewer window is updated.
  
Mouse and Keyboard Shortcuts
----------------------------

* To zoom in and out, use the mouse scroll wheel, or the trackpad vertical scroll gesture.
* To pan,

  * In 2D, drag with the left mouse button.
  * In 3D, drag with the right mouse button.

* To rotate in 3D, drag with the left mouse button.
  Use pan to change the centre of rotation.

=======  =========
Key      Effect
=======  =========
CTRL-W   Close the window (COMMAND-W on MacOS)
HOME     Reset camera to original viewing position
U        View top (up side)
D        View bottom (down side)
L        View left side
R        View right side
F        View front side
B        View back side
=======  =========

Head Up Display
---------------
When a parametric shape is viewed, the value picker GUI is displayed
as an overlay on top of the shape.
This overlay is called the HUD (Head Up Display), and it is toggled
using CTRL-H (or COMMAND-H on MacOS).
  
Command Line Options
--------------------
You can use command line options to modify the operation of the Viewer window.

* ``-O lazy`` reduces GPU power consumption, which is important on a laptop.
  By default, Curv continually redraws the shape at every video frame, even when
  nothing is changing. This will cause a laptop to heat up and drain battery power.
  With the lazy option, the shape is only redrawn when there is mouse or keyboard input.
  The drawback is that this disables animation (in animated shapes) and disables
  the FPS counter.

* ``-O bg=<colour>`` changes the background colour from the default white.
  You can use any colour expression.
  For example, try ``-Obg=black``, or ``-Obg="sRGB.grey 0.2"`` for a charcoal grey,
  or ``-Obg="sRGB(.2,.2,.35)"`` for a steel blue.

* ``-O aa=<supersampling factor>`` enables spatial anti-aliasing.
  Try 2, 3 or 4, or 1 to disable. This is expensive and slow, but looks nice.

* ``-v`` logs debug information to standard error, while the shape is being
  loaded into the GPU. This information provides some additional information
  about how expensive the GPU program is, in addition to what can be inferred
  from the FPS counter.

For a complete list of ``-O`` options that control the Viewer window,
use ``curv --help``.
