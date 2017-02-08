# Customization

I like the idea of "prototype oriented programming" proposed by OpenSCAD2.

A new shape can be constructed in two ways:
 * Using the "high level api", by composing existing shapes using high level
   shape operators.
 * Using the "low level api", constructing a new shape using the "shape
   protocol", currently by invoking the `shape2d` constructor.

You can write a function that returns a shape with either of the above
2 kinds of shape expression as the body.

I want the ability to define customizable shapes using either API.

 1. OpenSCAD2 has "geometric objects", similar to Curv modules,
    which can be customized using function call notation.
    This generalizes the idea that an OpenSCAD script denotes a shape.
 2. Parameterized_Shape proposes `defshape` as a special definitional form
    for defining shape constructors that preserve the constructor name when
    the constructed shape is serialized. So we can "export the CSG tree".
    It is proposed that if the constructor isn't curried, and all arguments
    have defaults, then the shape is customizable.

So I have proposed multiple ways to define a customizable shape.

[1] `lollipop.curv` contains:
```
param radius   = 10; // candy
param diameter = 3;  // stick
param height   = 50; // stick

translate([0,0,height]) sphere(r=radius);
cylinder(d=diameter,h=height);
```
Then, `lollipop = file("lollipop.curv");`.

[2]
```
lollipop = {
    param radius   = 10; // candy
    param diameter = 3;  // stick
    param height   = 50; // stick

    translate([0,0,height]) sphere(r=radius);
    cylinder(d=diameter,h=height);
};
```

[3]
```
// automatically supports customization, due to no currying and all
// parameters having defaults.
defshape lollipop(
    radius   = 10, // candy
    diameter = 3,  // stick
    height   = 50) // stick
= union [
    translate([0,0,height]) sphere(r=radius),
    cylinder(d=diameter,h=height),
];
```

[4]
```
defshape lollipop = {
    param radius   = 10; // candy
    param diameter = 3;  // stick
    param height   = 50; // stick

    translate([0,0,height]) sphere(r=radius);
    cylinder(d=diameter,h=height);
};
```
This illustrates that `defshape` need not have any special powers of
customization if modules are customizable and modules are shapes.
