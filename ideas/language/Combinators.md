# Tacit Programming

Tacit Programming is a style of functional programming where you define
new functions without mentioning the arguments of those functions.
Instead, you use operators (called combinators) which take functions as
arguments and return functions as results.

This style of programming can be very convenient at the small scale:
you can write short expressions that do a lot, and in some cases it
can be very readable. Unix pipelines are actually an example of combinator
programming, it's a particular case where the style works well.

Unix:
    alias countfoo='grep foo | wc -l'
    cat myfile | countfoo
Curv:
    countfoo = find "foo" |> len;
    countfoo mylist

At a larger scale, combinator programming can be opaque.
The lack of explicit names for values obfuscates the code if you write
very large, deeply nested combinator expressions.

My goal is to provide just a basic set of facilities for small scale
combinator programming.

Naming convention: the 'lifted' version of a standard function
can be distinguished using an initial capital letter. Predicate functions
are lifted booleans, so we can use Num, Bool etc as type predicate names.
Which is convenient for predicate assertions.
The lifted version of 'any' is Any, so Any[Num,Bool] is a predicate
that is true for both numbers and booleans.

Initially, I'll focus on tacit programming for booleans.

Div(fx,fy) a = fx a / fy a;  // lifted version of `/` operator
average = Div(sum,len);

The de-facto standard name for the function composition operator
is << and >>, so we'll use that.

lift1(f)(x) = f(x);
lift2(f)(x,y) = f(x,y);
lift3(f)(x,y,z) = f(x,y,z);
Map(f) = map(g->f>>g);

True = x->true;
False = x->false;
Not = lift1 not;
Any = Map any;
All = Map all;

find(p->p'X == 0) >> 
find(_'X == 0) >> 
