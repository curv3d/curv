Curv is documented, the language is mostly stable (with exceptions noted in
the documentation). It is ready for people to play with.

You can contribute, by:
* Reporting bugs.
* Requesting new features.
* Fixing bugs.
* Improving the documentation. Eg, a good tutorial.
* Implementing new features. See `<docs/Future_Work.rst>`_ for a list of ideas.

## Bug Reports
Just create a new issue, if you can't find an existing issue that describes your problem.

If a Curv script doesn't render properly, it might be a bug in glslViewer or an incompatibility
with your GPU driver or your GPU hardware. Please report:
* What you see in the graphics window. If it's totally black, that may mean that
  the generated GPU code (a GLSL script) did not compile, in which case there should
  be error messages in the shell window from which you invoked `curv`.
  If it's white with no other content, or it looks glitchy and weird,
  that could be a problem with your model, or a problem with the sphere tracer.
* Any error messages in the shell window from which you invoked `curv`.
* The make and model of your graphics card (GPU).
* The name and version of the graphics driver that you are using (especially important on Linux).

## Feature Requests
Create a new issue.

## Bug Fixes
For small bug fixes and documentation fixes (eg, 10 lines or less),
just submit a Pull Request using git.

For large changes, please create a feature request first,
so that we can discuss the work you plan to do.
Don't surprise me with a huge PR containing weeks of work,
because I can't guarantee that I will accept it.

## For More Information
Contact Doug Moen <doug@moens.org>
for more information about contributing to Curv.
