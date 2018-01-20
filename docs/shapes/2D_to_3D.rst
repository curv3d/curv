2D -> 3D Transformations
========================
Operations that convert a 2D shape into a 3D shape.

``extrude d shape``
  ``extrude`` converts a 2D shape to a 3D shape,
  linearly extruding it equal distances along the positive and negative Z axis,
  with total height ``d``.
  Similar to Autocad ``extrude`` and OpenSCAD ``linear_extrude``.
 
  * ``extrude.mitred d shape``: mitred distance field.
  * ``extrude.exact d shape``: exact distance field.

``revolve shape``
  The half-plane defined by ``x >= 0`` is rotated 90°, mapping the +Y axis to the +Z axis.
  Then this half-plane is rotated around the Z axis, creating a solid of revolution.
  Similar to Autocad ``revolve`` and OpenSCAD ``rotate_extrude``.

Future Work
-----------
``cylinder_extrude (d, d2) shape``
  TODO:
  An infinite strip of 2D space running along the Y axis
  and bounded by ``-d/2 <= x <= d/2``
  is wrapped into an infinite cylinder of diameter ``d2``,
  running along the Z axis and extruded towards the Z axis.

``helix_extrude (...) shape``
  TODO: a 2D shape is swept along a helix. Similar to AutoCAD ``helix`` command.
  Note that if you ``twist`` a cylinder around the Z axis, the cross section is egg-shaped,
  not circular. By contrast, applying ``helix_extrude`` to a circle gives you a helix with
  a circular cross section.

``stereographic_extrude shape``
  The entire 2D plane is mapped onto the surface of the unit sphere
  using a stereographic projection,
  and extruded down to the origin.
  TODO
