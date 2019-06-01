Curv has an optional user-defined configuration file, which the ``curv``
command reads during startup.

You use the configuration file to customize the behaviour of Curv when viewing or exporting shapes.
It works by providing default values for ``-O`` options which are
given on the command line.
Use ``curv --help`` to get a list of the current ``-O`` options.

Config File Location
--------------------
The config file is normally located at ``~/.config/curv``.
This can be a regular file, evaluated as a Curv syntax source file,
or it can be a directory, which is evaluated using `directory syntax`_.

The actual rules for locating the config file are more complicated,
since Curv attempts to conform to the `XDG Base Directory Specification`_:

* The environment variable ``$XDG_CONFIG_HOME``
  defines the base directory relative to which the Curv config file is stored.
* If ``$XDG_CONFIG_HOME`` is either not set or empty,
  it defaults to ``$HOME/.config``, unless ``$HOME`` is either not set or empty,
  in which case there is no Curv config file.
* The config file is named ``$XDG_CONFIG_HOME/curv``.
* Curv ignores the ``$XDG_CONFIG_DIRS`` environment variable and the ``/etc/xdg`` directory.

.. _`directory syntax`: language/File_Import.rst
.. _`XDG Base Directory Specification`: https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

The Config Namespace
--------------------
The config value (the result of evaluating the config file)
is a hierarchical namespace of configuration parameters,
represented as a tree of records.

The top level names are ``export`` and ``viewer``.

* ``export`` contains defaults for ``-O`` parameters used when exporting a shape
  using ``-o``.
* ``viewer`` contains defaults for ``-O`` parameters used when viewing a shape.

Example
-------
Here is an example. This is a text file stored at ``~/.config/curv``::

  {
    export = {
      aa=4;
      colouring=#vertex;
    };
    viewer = {
      bg=black; // dark mode
    };
  }

We have set the anti-aliasing parameter to ``4`` in order to obtain
higher quality when performing an image export. But anti-aliasing is disabled
when viewing objects (the default), for performance reasons.

Use vertex colouring when exporting X3D files.

The background colour is set to black when viewing shapes, because we prefer "dark mode".
But the background colour is set to the default (white) when exporting PNG files,
because we haven't overridden that value in ``export``.
