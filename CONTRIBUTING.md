At this stage, the project is in the Alpha test stage.
The code is incomplete, the language and APIs are not stable.

But you can still contribute if you like. Download curv, build it,
play with the examples. Then, you can report bugs, submit bug fixes
or documentation fixes, or contribute to/help run the project in other ways,
depending on your interests.

## Bug Reports
Just create a new issue, if you can't find an existing issue that describes your problem.

If a Curv script doesn't render properly, it might be a bug in glslViewer or an incompatibility
with your GPU driver or your GPU hardware. Please report:
* What you see in the graphics window. If it's totally black, that may mean that
  the generated GPU code (a GLSL script) did not compile, in which case there should
  be error messages in the shell window from which you invoked `curv`.
  If it's sky blue with no other content, that might be a 3D model that failed to render.
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
Contact Doug Moen for more information about contributing to Curv.
The author's email address is this Curv expression:
```
let name="doug"; domain="moens.org" in "${name}@${domain}"
```
