Shape Debugging
===============
``show_axes shape``
  Add an X/Y or X/Y/Z axis display to the shape.

``show_bbox shape``
  TODO: Visualize the bounding box, so you can check if it is bad (too small to contain the shape).

``set_bbox bbox shape``
  Manually fix a bad bounding box.

``show_dist shape``
  Visualize the signed distance field on the XY plane.
  Green channel: contour lines inside the shape (distance <= 0).
  Blue channel: contour lines outside the shape (distance > 0).
  Red channel: > 0 at points where the gradient > 1, ramping to full
  intensity where gradient >= 2.
  If distance is NaN (something that can only happen on the GPU),
  the colour is white.
  If distance is infinity, the colour is vivid cyan.
  If distance is -infinity, the colour is dark cyan.

``show_gradient (j,k) shape``
  Visualize the signed distance field (in the XY plane)
  by displaying gradient values.
  Gradient values < j are displayed in black.
  Gradient values > k are displayed in white.
  Gradient values between j and k are displayed using an sRGB.hue spectrum.

  To find the Lipschitz constant of a shape with a bad distance field,
  start with (j,k)=(1,2), then use binary search to find the smallest value of k
  that doesn't produce white. Then you can plug that
  value into the ``lipschitz`` function to fix the bad distance field.

``lipschitz k shape``
  Repair a distance field whose Lipschitz constant k is != 1.
  If k < 1 then rendering via sphere tracing is slower than necessary.
  If k > 1 then rendering will fail.
  The argument ``k`` is the actual Lipschitz constant of ``shape``.
  
  If an experimental shape isn't rendering correctly,
  then ``shape >> lipschitz 2`` is often a quick way to fix the problem.
  If the distance field is not Lipschitz continuous, then ``lipschitz`` can't help you.
