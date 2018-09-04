# Efficient Update of Arrays and Records

I want a nice syntax for incremental update of arrays
and records, and also an efficient implementation.

Syntax:
The current plan is for lists and records to use the same indexing operator,
currently `x'i`, where `i` is an integer or string.
Then `update(x,i,newval)` is the update operation for lists and records.
Within the sequential assignment sublanguage, `next x'i = newval`
is an abbreviation for `next x = update(x,i,newval)`.
Likewise, `next x.f = newval` is `next x = update(x,"f",newval)`.

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
