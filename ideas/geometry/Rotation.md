# Specifying Rotation

rotate(angle) shape
    rotate ccw around the z axis. Works for 2D and 3D
rotate(quaternion) shape
    3D only

A quaternion is represented as a vec4, as [x,y,z,w].

A quaternion can be specified in several ways.
http://doc.qt.io/qt-5/qquaternion.html
https://docs.unity3d.com/ScriptReference/Quaternion.html

constructors:
* qua{angle: a, axis: vec}
* qua{xyz_angles: [xangle,yangle,zangle]} // Euler angles in OpenSCAD order
* qua{zxy_angles: [zangle,xangle,yangle]} // Euler angles in Unity/QT5 order
* qua{from: direction, to: direction}
* qua{forward: direction, up: direction}
* ...

operations ...

As a possible abbreviation, `rotate(record)` applies `qua` to the record.
Eg, `rotate{angle: a, axis: vec} shape`.

What does the CSG tree look like for `rotate`?
* The quaternion is output in low level [x,y,z,w] form.
  Not unlike OpenSCAD converting `rotate` to `multmatrix`.
  If `rotate` is implemented in Curv in the normal way, this is what happens.
  This is the default design.
* The quaternion is normalized to a more friendly form like angle/axis.
  How do we make this happen? Is there a special code hook in the shape protocol
  for making the export look prettier? Or is Quaternion a special data type,
  not just a vec4?
* The original form of the quaternion constructor is preserved, at extra cost.
  Maybe this is too much.
  * Maybe, in the special case of `rotate(record)`, the record is preserved
    in the CSG tree. The 'shape requirements' protocol provides an extra level
    of indirection during which the quaternion record could be converted
    to a quaternion value, before rendering?
  * Maybe 'qua' is a Constructor which converts the quaternion to a high level
    representation when it is serialized, instead of doing this in `rotate`.
  * Maybe there are a series of constructors which encapsulate a quaternion,
    such as quaternion.from_angle_and_axis? As a way of preserving the original
    form of the constructor.

qua = switch [
    (Num angle) -> ...,
    [x,y,z,w] -> ...,
    {angle, axis} -> ...,
    {xyz_angles} -> ...,
    ...];
qua(Num angle) = ...;
qua[x,y,z,w] = ...;
qua{angle, axis} = ...;
...
