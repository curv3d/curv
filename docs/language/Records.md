# Records

The Object proposal splits OpenSCAD2 objects into two types:
"simple objects" and (a more sophisticated subtype) "scoped objects".
In this proposal, "simple objects" are replaced by a simpler type
called "records", and scoped objects are deferred until later.

A record is a set of named fields.

For simple dynamically typed records, the basic operations are:
* construction: `{a=1, b=2}`. Does not introduce a scope.
* selection: `x.a`
* `merge[r1, r2]`: update existing fields with new values, add new fields
* `defined x.a`: test if a field exists
* equality

The `merge` operation is basically the same as simple object concatenation
from the Object proposal. It's a monoid with `{}` as the identity.
The name `merge` comes from Clojure.
Lists are not records, so you can't merge a list or concat a record.

Simple objects are records, not maps, and they are deliberately limited.
* No way to obtain a list of field names (as strings).
  (Hence there's no ordering on field names.)
* No way to select a field using a string value.

These operations aren't needed. They are classified as introspection.
Also, these operations would complicate the model, turning records
into a map data type, and this would interfere with the possible introduction
later of scoped objects as a subtype of records.

So until I have a solid design for libraries and parameterized shapes,
I'm going to limit the power of records.

Records don't have anonymous elements, so they aren't a subtype of list.
It's not needed for the TCAD3 use cases. That's another possible extension that
could interfere with the future design of libraries and parameterized shapes.

## Haskell Data.Map operations:

null(x) -- is the map empty
size(x) -- # of elements
member(key, map) -> Bool -- is key a member of map
lookup(key,map) -> Maybe a
findWithDefault(default,key,map) -> a

insert(key,value,map) -- add new field, or override existing field
delete(key,map) -- remove field from map, return original map if key not member
adjust(f,key,map) -- if key exists, update it using f, otherwise no effect
... lots of variants

union(map1,map2) -- left biased
difference(map1,map2) -- fields of map1 that aren't in map2
intersection(map1,map2) -- left biased
...

map(f,map)
mapAccum
fold
...

elems(map), keys(map), assocs(map)

toList(map) -> list of key/value pairs
fromList(assocList) -> map, last key value wins

## TeaCAD Simple Object Operations
There's like 100 Haskell Data.Map operations.
What's the minimal kernel, using which the rest can be implemented efficiently?

Also, in my conception of Object, an object has an API that can be extended
but not contracted. So there's no way to remove a field from an object.
* More accurately, if you can get a list of keys, then you can construct a
  new simple object with all but one of the keys in a source object, but
  this will downgrade a scoped object to a simple object. You can't remove
  a field from a scoped object.
* So yes, all of the Haskell Data.Map operations could be implemented.
* But, object keys are actually *names*. Eg, it's a set of model parameters.
  So objects are more like records.

Records in Elm.
pt = {x=1, y=2}
pt.x
.x pt
pattern matching: {x} matches any record with an x field, binds x
update: { pt | y=42 }

// using Elm pattern matching:
is_polytope{polytope=false} = polytope;

If simple objects are records, then the basic operations would be:
* construct: {x=1, y=2}
* select: pt.x
* update a field or add a new field
* test if a field exists

In the basic design, there's no operation for 
