Colours
=======
Colours are abstract values. This means that their representation is hidden
by the standard API. You call a function to construct a Colour. When a Colour
is printed, in textual form you see a call to a Colour constructor function,
not a list or record literal.

Colours are graphical values. When a colour is displayed as console output,
or in a value browser, either alone or as part of a data structure, we attempt
to display a colour swatch.

When editing code containing a Colour constant in a visual editor, the
Colour constant is displayed with a colour swatch, and you can invoke a colour
picker to graphically edit the constant, perhaps by clicking on the swatch.

A Colour denotes a *colour*, not a coordinate triple in some colour space.
* A Colour value does not contain an implicit colour space identifier that
  alters the behaviour of some colour operations. Instead, colour operations
  that are relative to a colour space need the colour space to be
  explicitly specified.
* Curv is a high level domain-specific language, and the domain concept
  that we want to model is the Colour, as a perceivable, graphical concept.
  We don't want the Colour type to model coordinate triples in a colour space,
  because that is a lower level, more difficult concept. The group of users
  that Curv is designed for do not always understand the subtleties of
  different colour spaces.
* If two colour values produce the same sRGB pixel value, then they are
  operationally equivalent, and they compare as equal (c1 == c2).

We do need the ability to extract colour space coordinates from a colour,
but we will just provide operations to do that. See also "Colour Space".
So then you can manipulate colour space coordinate vectors outside the
Colour abstraction, any way you want, then construct another Colour value
from the coordinate vector once you are ready.

The Colour API
--------------
construct a colour:
```
    sRGB(vec3)
    linRGB(vec3)
    ...
```

extract colour space coordinates from a colour:
```
    sRGB.get col
    linRGB.get col
    ...
```

The set of known colour spaces is not "built in" to the Colour type, so user
defined colour spaces have the same interface.

```
is_colour(red) == #true
```

What does the colour API look like when in use?
Maybe you want to interpolate between two colours in a shape colour function.
```
    colour p =
        let
            c1 = shape1.colour p;
            c2 = shape2.colour p;
            k = interpolation factor;
        in
            sRGB(lerp(sRGB.get c1, sRGB.get c2, k));
```

Colour Constants
----------------
How is a colour constant recognized in code by a visual editor, so that a
colour swatch can be displayed after the expression, and so that a colour picker
can be invoked to edit the constant? Some ideas:

 1. MVP. A colour constant has the form `sRGB[Rconst,Gconst,Bconst]`,
    where the arguments of `sRGB` are numeric constants. This is recognized
    by pattern matching on the program text or abstract syntax tree.
    A programmable text editor like VIM or EMACS might be powerful enough
    to recognize this pattern and display the colour swatch, as an extension
    of syntax colouring?

 2. Perhaps a larger set of built-in ternary colour constructors is supported.
    Eg, `linRGB[r,g,b]`, `sRGB.HSV[h,s,v]`, `LAB.HCL[h,c,l]`.
    It's still a fixed set of patterns, as might be recognized using a regex.
    Question: after editing such expressions with a colour picker, is the
    colour space preserved, or does the expression get replaced with a call
    to `sRGB`?
 
 3. Are user-defined ternary colour constructors supported? (At least by the
    Curv language server, maybe not by VIM.)

 4. Perhaps unary colour constructors are supported, such as `sRGB.grey(i)`.
    Question: does the colour picker restrict you to picking another
    grey-scale value, or can you pick any colour?
 
 5. Perhaps nilary colour constructors are supported, such as `white` and
    `red` and `lib.web_colour.antique_white`?
 
 6. Perhaps arbitrary colour constants are supported. The Curv language server
    does compile time analysis to recognize constant expressions, and marks
    these as such in the abstract syntax tree. The `is_colour` predicate is
    applied to the value of constant expressions to recognize colour constants.

In the general case, perhaps we support the editing of arbitrary colour
constants. The simple approach is that an edited colour constant turns into
`sRGB[r,g,b]`. Maybe we can recognize special cases, such as grey-scale
colours or named colours, and output a call to `sRGB.grey` or a colour name,
in those special cases. Preserving the syntax of the original colour
expression would be quite complicated in the general case.

Opacity
-------
I would like to have translucent materials. Eg, to support Strata J750 printing,
or to render translucent objects in a 3D world.

One idea is to extend Colour values with an alpha component (opacity). I don't
know if this is the best idea. A proper solution might be part of the more
general 'materials' model that I need for advanced 3D rendering. In the world
of 2D image alpha compositing, the "premultiplied alpha" colour representation
is more expressive than the alternative: it allows you to represent translucent
luminous materials like fire, together with translucent non-luminous materials,
in the same image. So translucency can be more complicated than just adding an
alpha field.

Elsewhere, I speculated that colour functions that return an opacity < 100%
might need to be marked as 'translucent' for performance reasons.
A plausible alternative is an optional separate 'opacity' function, if that
didn't lead to code bloat.

This is deferred until the material model is designed. Maybe there is a
'material' function that returns a Material value that contains an 'opacity'
field along with other material properties.

Colour Swatches on a Text Terminal
----------------------------------
Colour values should print as colour swatches.
I can do this on a terminal that supports 24 bit colour text (see: Pastel)
 * Since ncurses-6.0-20180121, terminfo began to support the 24-bit True Color
  capability under the name of "RGB". You need to use the "setaf" and "setab"
  commands to set the foreground and background respectively.
  https://gist.github.com/XVilka/8346728
 * A lot of systems don't have an updated terminfo, and/or don't set the
   TERM variable to a name that defines the RGB flag. So if RGB isn't defined,
   you check the COLORTERM environment variable.
 * Pastel checks the COLORTERM environment variable.
 * gnome-terminal does not define COLORTERM because "COLORTERM is a long
   obsolete slang-only variable used to work around broken termcap/terminfo
   entries.": https://bugzilla.redhat.com/show_bug.cgi?id=1173688
