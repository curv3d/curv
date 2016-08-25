A Transformation value encapsulates a 2D or 3D affine transformation matrix.

// can also pass [x,y] argument for 2D case:
translate[x,y,z]
scale[x,y,z] or scale(s)
reflect[x,y,z] -- boolean vector
rotate[x,y,z]
shear[x,y,z]

transform(matrix)

composition
> compose[T1,T2]

`id = compose[]` is the identity function.
It is also the identity transformation:
* Transformation is a subtype of Function, with additional operations
  tmatrix, inv, == and is_transformation.
* The identity function `id` is a special value that also supports
  tmatrix, inv, == and is_transformation.
* But the domain of `id` is Any, which is
  broader than the domain of a Transformation.
* We can imagine that id==id is true, guaranteed by the function
  implementation, while the implementations of tmatrix, inv and
  is_transformation can test if their argument is id, as a special case.

inverse:
> inv(T)

application:
> can be applied to a 2d or 3d vector,
> a list of 2d or 3d vectors,
> a 2d or 3d shape, using function application syntax.

tmatrix(T)
> returns the underlying matrix within a transformation value T.
> Maybe we need two versions of this for returning a 2D and a 3D matrix.

Equality:
> It would make sense for transformation values to support equality tests.
> Maybe not hugely useful, but not hard to implement, and consistent with
> the presumed charter for the equality function.

Classification predicates:
* is_transformation(T) is true if T is a 2D compatible transformation.
  It doesn't modify the Z coordinate, and can be applied to either 2-vectors
  or 3-vectors.
* is_3d_transformation(T) implies is_transformation(T), and implies that T
  modifies the Z coordinate, and can only be applied to 3-vectors.
