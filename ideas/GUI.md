# Building a GUI

Currently, my GUI is glslViewer: C++ and GLFW. That's probably okay for MVP.

Later, I could write a more sophisticated custom GUI.
Maybe with an OpenSCAD-like interface.
Or maybe a visual programming environment.

My current preference is C++ and QT5, for a desktop app.
Or focus on building a web app.

QT provides a portable, high level OpenGL and GUI interface.
(Raw OpenGL is low level, verbose, and platform dependent.)
It's one library solving both problems.

C++ is the native language for QT and OpenGL. Played with Python, but the
necessary modules seem "second tier" compared to C++ libraries.
PyQT5 supports "most" of QT.
"Desktop bindings for OpenGL 2.0 are mostly complete."
QT5's OpenGL ES bindings aren't supported yet by PyQT5.

Maybe reuse the OpenSCAD GUI code.

Platforms?
* Ubuntu 16.04 is my primary platform.
* macOS: #2, as long as my macbook holds up anyway. Not interested in buying
  a new Mac.
* Windows 10? Ecch. Not interested. (But it's popular!)
* Raspberry Pi, probably Raspian. OpenGL ES2, full screen only.
  No GLFW port. QT5 exists in full screen mode, but no standard package, you
  compile it yourself. No gcc 6 unless you compile it yourself.
  glslViewer has a Raspberry Pi port. Use SSH and glslViewer, that works.
  (OpenSCAD is ported; how does it work if you can't run OpenGL in a window?)
* The web.
* Android??

------------------------------------------------------------------------------
Python: PyQT5 and PyOpenGL seem good choices: popular, well supported, powerful.
Their APIs are very close to the corresponding C++ libraries, which might
also help me.

Benefits of Python?
* It's not clear if the Python code will actually be much shorter or any better
  than the C++ code.
* Faster turnaround between editing and running, might speed development.
* Will encourage me to think about and develop a Python interface to Curv.
* Make it easier for others to modify the gui and submit changes?

Benefits of C++?
I could just steal the existing OpenSCAD C++ GUI and reuse it, maybe.

With PyOpenGL, I don't know what I'm getting. Looks like I'm using the
deprecated immediate-mode API. Probably better if I know which version
of the OpenGL API that I'm using.  The shading languages are different
for different versions, I need to pick a target.

I should target OpenGL ES 2.0, since it's supported by web browsers
(=WebGL 1.0), mobile and desktop. (Not WebGL 2.0 since it hasn't been
released yet and is still in flux.)

I should use ES 2.0 documentation and tutorials from kronos.org.
Maybe use C++, so I can better integrate with my existing code.

ES 2.0 on Ubuntu? Virtually nobody does this. (How are Chrome and Firefox
implemented?) Chrome uses the ANGLE library:
https://github.com/google/angle
A conformant OpenGL ES implementation for Windows, (Mac) and Linux.
http://angleproject.org
* macOS support is "in progress"
* Windows, Linux, Android and Chrome are supported. (Not iOS)
* not an official Ubuntu package

The OpenGL API is a nightmare. There is a ton of boilerplate required.
It's normal to use high level libraries to hide all this shit.
* GLFW -- create windows, contexts, surfaces, receive mouse&keyboard events.
  No rendering support: use raw OpenGL with the context object.
* GLEW -- extension wrangler. complements GLFW.
* FreeGLUT -- covers a superset of what GLFW attempts

There's really no such thing as "portable OpenGL" right now.
* I'm going to write a desktop GUI targetting Linux, macOS, Windows.
  "Desktop OpenGL" will work.
* I need to generate GLSL code. It would be nice if this output could be
  platform-independent. Is OpenGL ES 2.0 GLSL a subset of desktop GLSL,
  or are there incompatibilities? If I verify my GLSL using Angle, will it
  work anywhere? (Tentatively, yes.)

