ContextFree is a domain specific language for drawing 2D images,
which are represented as context free grammars. It's good for specifying
recursive fractals, using very short programs.

I'd like to find a way to reproduce some of these programs in Curv.
* ContextFree is implemented by iteratively painting a shape repeatedly on a
  canvas, with each copy transformed from the previous copy. By contrast,
  Curv uses F-Rep. It might be possible to automatically translate some CF
  grammars into an efficient F-Rep. On the flip side, Curv needs
  to more efficiently support painter's algorithm style image specifications.
* ContextFree is non-deterministic. Each render of a grammar may produce a
  different image. That's not compatible with Curv's pure functional nature,
  but we can work around this by parameterizing a sketch with a random seed.

main documentation on github:
https://github.com/MtnViewJohn/context-free/wiki

a javascript port, with a more visually oriented overview and introduction:
http://www.azarask.in/blog/post/contextfreejs-algorithm-ink-making-art-with-javascript/

The context-free project uses Anti-Grain, a fast, high quality 2D graphics
kernel that may exceed what you can do with more well known 2D graphics APIs.
The output of context-free is gorgeous (due to sub-pixel positioning and
anti-aliasing). Can I reproduce this output quality in Curv?

Anti-Grain is a boost-like collection of C++ algorithm templates.
http://antigrain.com/
