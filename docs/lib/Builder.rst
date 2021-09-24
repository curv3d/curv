# Builder

lib/curv/lib/builder.curv offers a sub-language written in Curv to easily
compose models which have parts which are relatively positioned. It makes
heavy use of the `>>` operator to pass along the state as the first argument
of every function.

Please see `examples/snowman.curv` for immediate usage.

## State / build function and done function

The state of the builder is passed to each function as mentioned. It is
structured like this:

```
[combiner_function, shape, origins[], last_position]
```

The `build` function will return an initial state to work from. This state is:

```
[union, nothing, [[0, 0, 0]], [0, 0, 0]]
```

## '><' function

Arguments: f :: is_func, state
Returns: state with a new combiner_function

This function can be read as "(use) combiner", and is used to specify which
combining operation will be used on the following shapes.

Example: `'><' ((smooth 0.5).union)

## '*' function

Arguments: s :: is_shape, p :: is_vec3, state
Returns: state with a new shape

This can be read as "combine". This invokes the `combiner_function` against the
previous and current shape passed into `'*'`. The position is calculated based
on the parent and the  current position passed to `'*'`.

Example: `'*' (sphere 10) [20, 0, 0]`

## '{' and '}' functions

Arguments: stack
Returns: stack with last origin pushed or popped to/from the origins stack

These are read "fork" and "join" respectively. They indicate when to push
the current shape's position onto the origin stack, so that "child" shapes
are positioned relative to it.

Note this **only** modifies the origin stack and nothing else, like the
`combiner_function`.
