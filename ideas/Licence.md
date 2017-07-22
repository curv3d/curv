# Licensing Rationale

My reasons for using an open source licence are:
 1. To encourage other people to use the software, make modifications,
    contribute bug fixes and enhancements back to the project.
 2. To protect me (and other contributors) from being sued.

Which open source/free software licence?
* Should be a standard licence, not a novel one.
  * Boost -- this is a good non-restrictive licence, and you aren't required
    to copy the full licence text into every source file.
    Three lines of boilerplate suffice.
  * Apache 2.0 -- more restrictive, includes a patent licence
  * GPL 2 -- copyleft, incompatible with a patent licence
  * GPL 3 -- copyleft with patent licence
  * Affero GPL 3 -- sharing requirement extends to server code exporting
    a public API. Eg, something like TinkerCAD.
* Should provide a patent grant: if you contribute to the project,
  and you own patents required to use your contributions,
  then you grant a licence for those patents w.r.t. use of the project.
  This means: Apache 2.0 or GPL 3.
* Should be compatible with OpenSCAD licensing, and for that matter,
  with all other open source licences.
  * OpenSCAD uses GPL 2 or later, with an exception:
    "As a special exception, you have permission to link this program
    with the CGAL library and distribute executables, as long as you
    follow the requirements of the GNU GPL in regard to all of the
    software in the executable aside from CGAL."
  * Note CGAL 4.0 and later is licensed using GPL/LGPL 3+,
    "due to the commercial value of the code",
    (with a commercial licence being available on request).
  * Carve is GPL 2 only (thus incompatible with patent grant).

I want to be able to link any open source geometry library into my code
without worrying about "licence compatibility". The only choice given
above that is compatible with all open source licences is the Boost licence.
So be it.

The debate over Ubuntu bundling ZFS demonstrates that the real situation of
licence incompatibility is less dire than Richard Stallman would have us
believe. In practice, Ubuntu ZFS suggests that it's probably safe for Curv
to mix any collection of open source geometry libraries together, even though
they are theoretically incompatible with each other. Using Boost reduces this
risk for any code I write, and for the rest, we'll see how the licencing
landscape evolves post Ubuntu-ZFS.

If I'm worried about commercial contributors infecting my code base with
patented techniques, then suing me, then I could use a Contributor Licence
Agreement for corporate contributors. CLAs are poison for hobbyist contributors,
but a corporate legal department might actually prohibit contributions to my
project unless a CLA is signed.

I am not interested in playing the game of using [Affero] GPL 3 and then
selling commercial licences. That requires other contributors to sign over
their rights to me, or negotiate profit sharing, or whatever. Too much
complex legal bullshit, and it conflicts with goal #1 above.

The bottom line is, I'm giving away the Curv source as an act of generosity.
