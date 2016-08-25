# Efficient Update of Arrays and Records

So, I want to explore a nice syntax for incremental update of arrays
and records, and also an efficient implementation.

My implementation idea is copy on write: copy the collection if `use_count>1`,
then incrementally update. This is combined with a function calling convention
and optimizations (at least, tail call optimization) that make this effective.

As for syntax, I have considered:
```
a[i] := x   // returns updated list
r.f := x    // returns updated record
merge[r,{f=x}]
```

Records in Elm:
```
pt = { x=1, y=2 }
pt.x
.x pt
hypot {x,y} = sqrt(x^2 + y^2)
    -- these patterns allow extra fields to exist
{ pt | y=7 } -- update the y field
type alias Point = {x:Float, y:Float} -- type has exactly these fields
```

PureScript:
```
pt = {x:1, y:2}
pt.x
forall r.{ foo::Int, bar::Int | r } -- a polymorphic record type
-- update a record named R:
R { key1=val1, foo=R.foo+1 }
```

maybe
```
{r | x=17}
[a | 0:"zero", 1:"one"]

rupd(x=17) r
lupd(0:"zero", 1:"one") a

rupd{x=17} r
lupd[[0,"zero"], [1:"one"]] a
```
