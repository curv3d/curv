``lib.web_colour`` defines 148 standard CSS colour names, which are familiar to web developers.
The colour names are in lower case, with words separated by ``_``.
For example,

* ``lib.web_colour.red``
* ``lib.web_colour.alice_blue``
* ``lib.web_colour.dark_goldenrod``

There is also an `rgb` function that follows the CSS standard.
It constructs a colour from a triple of RGB intensities, each a number between 0 and 255.
For example, ``lib.web_colour.magenta`` is the same as ``lib.web_colour.rgb(255,0,255)``.

For more information about colour values, see `<../shapes/Colour.rst>`_.
