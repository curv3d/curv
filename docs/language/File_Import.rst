File Import
===========

``file filename``
  Evaluate the program stored in the file named ``filename``,
  and return the resulting value. ``filename`` is a string.

The Curv ``file`` operation can import data from multiple file formats,
of which the text-based Curv language (``*.curv`` files) is just one example.
All of these file formats are considered "source files",
and all of them are "evaluated" to produce a value.

Supported file formats:

==================   ===========
Filename Extension   Description
==================   ===========
``*.curv``           Curv language source file
*none*, directory    Directory syntax
==================   ===========

More graphical file formats will be added to this list in the future.

Directory Syntax
----------------
In the directory syntax, the filename names a directory (also known as a folder),
instead of a regular file.

A directory is evaluated to produce a record value, with one record field for
each contained file of a recognized type. (Files of an unknown type are ignored.)
For each file contained within the directory,

* If the filename begins with ``.``, then it is ignored.
* Otherwise, if the file is a directory, and the filename has no extension,
  then it is imported using directory syntax.
  The corresponding record field name is the filename.
* Otherwise, if the file is a regular file and the filename extension is one
  of the known extensions in the above table, then the file is imported
  based on its extension. The corresponding record field name is the filename
  with its extension removed.

The record values constructed by parsing directory syntax use lazy evaluation.
The file contents of a record field are not read into memory and converted to a value
until the field value is required. This means you can import a large directory,
or a deep directory hierarchy, containing many ``*.curv`` files, without reading and
evaluating all of those files.
