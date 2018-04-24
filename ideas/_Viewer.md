# 3D Viewer
Goals:
* View a compact object (eg, a dodecahedron).
  * Orbit around the object, viewing it from all directions (left, right,
    above, below, front, back).
    * There is an origin and an eye point.
      By default, the origin is at the centre of the compact object.
      The eye direction is the vector from the eye point to the origin.
      We are using controls to orbit the eye point around the origin without
      changing the distance.
    * OpenSCAD does this using button 1 drag in 2 dimensions. The implementation
      using Euler angles is subject to gimbal lock.
    * My idea is: button 1 drag. When the button goes down, you are conceptually
      grabbing a point on the surface of the sphere whose origin is `origin`
      and whose radius is `magnitude(eye-origin)`. By dragging the mouse,
      you rotate this sphere using quaternions, by slerping the control point
      to the new location. No gimbal lock.
  * Zoom in and out. In OpenSCAD, you use the scroll wheel. The eye point
    zooms in and out without moving the origin. The eye/origin distance is
    being changed.
* Fly through an infinite lattice. Need controls to:
  * Translate the eye/origin pair (without changing the distance).
    * Move in and out along the eye direction.
      How about: SHIFT + scroll wheel
    * Pan left/right/up/down.
      In OpenSCAD, this is button2 drag.
  * Rotate the viewing direction, keeping the eye point constant and moving
    the origin. (?) SHIFT button-1.
* Zoom in on an infinitely detailed fractal surface.
  Using scroll wheel, as in the u_view2d/mandelbrot example.
  * In the 2D viewer, the camera origin is always embedded in the 2D plane,
    so zooming is uncomplicated.
  * In the 3D viewer, if we use the scroll wheel to zoom with OpenSCAD semantics
    then the position of the origin is critical, you can zoom in only so close
    without moving the origin to the surface of the object you want to zoom
    in on.
  * What does this do in terms of the camera model?
    Does the distance between eye and origin get shorter?
    Or am I decreasing the field of view?
* Roll the camera around the eye/origin axis?
