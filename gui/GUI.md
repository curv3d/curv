# Building a GUI

I can implement in C++ or in Python.

QT provides a portable, high level OpenGL and GUI interface.
(Raw OpenGL is low level, verbose, and platform dependent.)
It's one library solving both problems.

C++ is the native language for QT and OpenGL. PyQT5 supports "most" of QT.
"Desktop bindings for OpenGL 2.0 are mostly complete."
(QT5's OpenGL ES bindings aren't supported yet by PyQT5.)

Use C++.
Start with a simple QT5 app that renders a shadertoy script.
Later, maybe reuse the OpenSCAD GUI.

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

