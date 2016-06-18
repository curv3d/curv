# Bounding Boxes

Representation:
* 2d: [[xmin,ymin],[xmax,ymax]]
* 3d: [[xmin,ymin,zmin],[xmax,ymax,zmax]]

Transformations:
* a transformation works on a list of vectors,
  and a bbox is a list of vectors.

Set Operations:
* bb_union[bb1,bb2,...]
* bb_intersection[bb1,bb2,...]

Constants (monoid identities)
* bb_nothing
* bb_everything

Predicates:
* bb_contains(container,containee)
* bb_is_infinite(bb)
* bb_is_empty(bb)

Implementation:
```
// These are polymorphic for 2d and 3d bboxes.
bb_union(bbs) =
  [ min[for(x=bbs)x@0]		// min(map(x->x@0)bbs)
  , max[for(x=bbs)x@1]
  ];
bb_intersection(bbs) =
  [ max[for(x=bbs)x@0]
  , min[for(x=bbs)x@1]
  ];

// Note: These monoid identities fall out of the above implementations.
// They actually work (in some cases) due to broadcasting.
bb_nothing = bb_union[]; // == [inf,-inf]
bb_everything = bb_intersection[]; // == [-inf,inf]

// assumes <=, >= broadcasts across arrays
bb_contains(er,ee) =
  all(er@0 <= ee@0) && all(er@1 >= ee@1);


bb_is_empty(bb) =
  any(bb@0 >= bb@1);

// ~= is the broadcast version of ==
// Fails for bb_is_infinite(bb_everything) if 'any' doesn't accept
// a single Bool argument, which would be a Tensor-like extension.
bb_is_infinite(bb) =
  any(bb@0 ~= -inf) || any(bb@1 ~= inf);
```
