QT5+QML uses distance fields to render text, using OpenGL,
which allows hardware accelerated drawing of transformable text.
Scalable, sub-pixel positioned and sub-pixel antialiased… and at almost no cost.

This can be done by Curv. During evaluation, a font is converted to a distance
field stored in a texture. During GL compilation, the texture is loaded into
the GPU. During rendering, the texture is referenced.

http://blog.qt.io/blog/2011/07/15/text-rendering-in-the-qml-scene-graph/

A distance field can be generated either from a high-resolution image or
from a vector-based representation of the glyph. First technique is slow...

We instead use a vector-based representation of the glyph to generate the
distance data. We extrude the outline of the glyph by a fixed distance on the
inside and on the outside and fill the area between the extruded outlines
with a distance gradient. We store the result in a single 8-bit channel of
a 64×64 pixels cell contained in a bigger texture. By doing so we generate
a distance field in less than a millisecond per glyph on a mobile device
(on average), making any vector font usable dynamically at run-time.

This technique uses native bilinear interpolation performed by the GPU on
the texture. The distance from the edge can be accurately interpolated,
reconstructing the glyph at any scale factor. This is done by alpha-testing:
pixels are shown or discarded depending on a threshold, typically 0.5 as
it is the value at the edge of the glyph. The result is a glyph with sharp
outlines at any level of zoom, as if they were vector graphics. The only
flaw is that it cuts off sharp corners, but this is negligible considering
how bad a magnified glyph looks when this technique is not used.

Using the same distance field representation of the glyph, we can also do
high-quality anti-aliasing using a single line of shader code:

varying highp vec2 sampleCoord;
uniform sampler2D texture;
uniform lowp vec4 colour;
uniform highp float distMin;
uniform highp float distMax;
void main() {
    gl_FragColor =
        colour * smoothstep(distMin, distMax, texture2D(texture, sampleCoord).a);
}

smoothstep: perform Hermite interpolation between two values (GLSL, WebGL)

Instead of using a single threshold to do alpha-testing, we now use two
distance thresholds that the shader uses to soften the edges. The input
distance field value is interpolated between the two thresholds with the
smoothstep function to remove aliasing artifacts. The width of the soft region
can be adjusted by changing the distance between the two thresholds. The more
the glyph is minified, the wider the soft region is. The more the glyph is
magnified, the thinner the soft region is.

When the GPU is powerful enough (meaning desktop GPUs) we can even do sub-pixel
anti-aliasing, it is just about adding some lines of shader code. Instead
of using the distance data to compute the output pixel’s alpha, we use
the data of the neighboring pixels to compute each colour component of the
output pixel separately. Five texture samples are then needed instead of
one. The red component averages the three left-most distance field values, the
green component averages the three middle distance field values and the blue
component averages the three right-most distance field values. Because this
requires more processing power, sub-pixel anti-aliasing is currently disabled
on mobile platforms. Anyway, the high pixel density displays that equips
mobile devices nowadays make the use of sub-pixel anti-aliasing pointless.

...

http://www.valvesoftware.com/publications/2007/SIGGRAPH2007_AlphaTestedMagnification.pdf
