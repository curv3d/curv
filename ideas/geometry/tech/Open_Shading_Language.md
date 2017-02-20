# Open Shading Language
This project sounds interesting.
It relates to my idea for exploring variations of the F-Rep distance field
representation that add more information to the representation, for more
efficient rendering.

https://github.com/imageworks/OpenShadingLanguage/

I'm intrigued by the OSL concept of a "radiance closure":

Surface and volume shaders compute radiance closures, not final colours.

OSL's surface and volume shaders compute an explicit symbolic description, called a "closure", of the way a surface or volume scatters light, in units of radiance. These radiance closures may be evaluated in particular directions, sampled to find important directions, or saved for later evaluation and re-evaluation. This new approach is ideal for a physically-based renderer that supports ray tracing and global illumination.

In OSL, you can take derivatives of any computed quantity in a shader, and use arbitrary quantities as texture coordinates and expect correct filtering. This does not require that shaded points be arranged in a rectangular grid, or have any particular connectivity, or that any "extra points" be shaded. This is because derivatives are not computed by finite differences with neighboring points, but rather by "automatic differentiation", computing partial differentials for the variables that lead to derivatives, without any intervention required by the shader writer.

In contrast, other shading languages usually compute just a surface colour as visible from a particular direction. These old shaders are "black boxes" that a renderer can do little with but execute to find this one piece of information (for example, there is no effective way to discover from them which directions are important to sample). Furthermore, the physical units of lights and surfaces are often underspecified, making it very difficult to ensure that shaders are behaving in a physically correct manner.

The OSL project includes a complete language specification, a compiler from OSL to an intermediate assembly-like bytecode, a runtime library interpreter that executes the shaders (including just-in-time machine code generation using LLVM), and extensive standard shader function library. These all exist as libraries with straightforward C++ APIs, and so may be easily integrated into existing renderers, compositing packages, image processing tools, or other applications. Additionally, the source code can be easily customized to allow for renderer-specific extensions or alterations, or custom back-ends to translate to GPUs or other special hardware.

