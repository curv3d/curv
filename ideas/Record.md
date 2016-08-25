# Record Implementation

There is a global atom table stored in the Session. Record field names
are represented by atoms, and equality is pointer equality.
The atom table is a modified LLVM StringMap, with curv::Atom pointers
stored as hash table entries. An Atom is a {hash value, string length, string}.
There is no reference count or type code.

A record is represented by a variant of persistent hash maps, as found in
Clojure hash-maps and Erlang maps.
* For small records and leaf nodes, a sorted array of {atom,value} pairs
  with binary search lookup. Possible variations:
  * Put the atoms first, followed by the values. Better cache locality
    during lookup.
  * Put the atom vector into a separate heap object, with a reference count.
    It can be shared by multiple records, in the case where you update an
    existing field.

## Ideas

Possible data structures:
* Binary search array. Easy. Good cache locality, efficient if small.
* Binary tree. Typical choice for a functional map; O(log N) operations
  with large constant, scales well for large records.
* Hash table. Fast lookup, expensive merge unless unique.
* Radix tree. Faster merge than hash table, otherwise comparable.
  No need for global atom table.

Records could be implemented as Lisp-style **association lists**.
That makes lookup O(N), but merge(R,{a=1}) would have constant time.
Updating one or two entries in a record is probably a common operation,
in the shape3d protocol, or if records are used as a generalized replacement
for special variables.

Skip lists?

Records could be implemented as a **sorted array** of (atom,value) pairs.
There's only 1 allocated block, instead of O(N) blocks, so good cache locality.
Lookup is O(ln N) instead of O(N), with a small constant.
`merge` is O(N) of the final result.
Store the keys contiguously, followed by the values: this improves cache
locality during lookup (from Erlang).

Maybe use a quadratically probed **hash table**, like the LLVM DenseMap.
Fast lookup.
The `merge` implementation uses a `use_count==1` test to destructively update
the hash table where possible; otherwise the hash table must be copied,
which is more expensive than copying a binary search array due to holes.

**Hash trees** are persistent data structures that replace hash tables in
pure functional programming.
* Hash Array Mapped Trie (HAMT) -- array mapped trie where keys are first
  hashed to ensure even key distribution and constant key length. Typical
  implementation: each node is 32 maybe-null pointers, represented as 32 bits
  followed by 1 pointer for each bit set. Used for hash map by Clojure, Scala,
  Frege. One in Rubinius. Erlang uses them for large maps.
  Ctrie is a concurrent lock-free implementation.

Maybe use a **radix tree**; eg Patricia Tree or Judy Array.
Fast lookup. Faster merge than hash table. Traverse names in sorted order.
Efficient "command line completion". No need for global atom table.

Haskell Data.Map is based on **weight balanced binary trees**.
Lookup and merge would both be O(ln N); but with large constants due to
poor cache locality.
A functional implementation with merge is available from Adams:

Stephen Adams, "Efficient sets: a balancing act", Journal of Functional Programming 3(4):553-562, October 1993, http://www.swiss.ai.mit.edu/~adams/BB/.
J. Nievergelt and E.M. Reingold, "Binary search trees of bounded balance", SIAM journal of computing 2(1), March 1973.

**Erlang maps** are supposed to be as efficient as (older) Erlang records,
support `merge`. Small maps are "flat maps", large maps are "hash maps".

SMALL_MAP_LIMIT = 32

map-tag:
* 00 - flat map tag (non-hamt) -> val:16 = #items
* 01 - map-node bitmap tag     -> val:16 = bitmap
* 10 - map-head (array-node)   -> val:16 = 0xffff
* 11 - map-head (bitmap-node)  -> val:16 = bitmap

Flat maps use linear search, not binary search!
flat map {
    size
    keys: tuple of keys
    val1
    val2
    ...
    valn
}

hash maps: HAMTs -- an adaption of Rich Hickeys Persistent HashMaps.
A node is a leaf, array or bitmap.
