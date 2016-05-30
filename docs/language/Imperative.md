# Mutable Variables and Imperative Programming

https://github.com/openscad/openscad/wiki/Mutable-Variables

Some version of this might go into TeaCAD. One requirement is a good syntax.

Reducing the scope of the feature might make it less objectionable.
Maybe the imperative syntax is only available in function bodies?
The list constructor syntax feels the weirdest.

Can I make the syntax suck less?

My biggest objection is `do {...}` as a compound action.
Seems like `{...}` would be better.
It would be nice if the imperative syntax were C-like.

Although this doesn't quite work, since C has a two level syntax,
with {} and ; at the statement level, and () and , at the expression level.
Whereas in the proposal, those lines are blurred, and it's not obvious
whether I should be using `,` or `;` or allowing both as synonyms.
This is an issue in list and object literals, where I support a sequence
of 'actions' which mix together data members (which should be `,` separated)
and declarations/assignments (which should be `;` separated or terminated).
```
[1,2,3]
[for (i=[0:9]) i]
[var i := 0; while (i < 10) { i; i := i + 1; }]
```

Maybe the list constructor thing is handled by providing
both imperative and functional list constructor syntaxes.
The imperative form uses `yield` and `;`.
This would be analogous to the `valof` syntax which has a `return` statement.
```
[for (i=[0:9]) i]
{[var i := 0; while (i < 10) { yield i; i := i + 1; }]}
do [var i := 0; while (i < 10) { yield i; i := i + 1; }]
```

What about objects? Could be handled similarly.
Let's say `{: ... :}` is object syntax. Then
```
{: x=1, y=2 :}
do {: x=1; y=2; :}
```
