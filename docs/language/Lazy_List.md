= Lazy Lists
Two key features of functional programming are (a) constructing new functions
using operations on functions, and (b) lazy lists to abstract a sequence of
iteration states away from the code that operates on that state.

Lazy lists can be simulated using strict lists and function values.
You end up reimplementing standard list operations using the new representation.
T'would be nice to have built-in lazy lists.

How'es that work?
- list comprehensions are lazy.
- lazy concatenation operator, eg L1 ++ L2.

Ideally, I'd like to have a single List type, which dynamically adapts to
how it is used. It works as a lazy list, it works as an indexable array.
