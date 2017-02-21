# math library

Fractional part of a number.
Mathworld names the function `frac(n)`, gives two competing definitions:
* sawtooth wave: mod(n,1) or n - floor(n). Result is non-negative.
  Eg: HLSL frac, GLSL fract, Mathematica SawtoothWave, MuPad frac,
* rem(n,1) or n - trunc(n). Result has the same sign and digits after the
  decimal point as the argument. Eg, C Julia modf, Mathematica FractionalPart
  Maple frac(n).
* Notationally, I think mod(n,1) and rem(n,1) are fine.

mag(v) = sum(v^2);
* Magnitude of a vector. More like high-school math than 'norm', which is also
  a more abstract and general concept.

phase[x,y] = atan2(y,x).
* Convert a 2D vector to an angle. It's a standard engineering name for the
  concept, as per MathWorld and Wikipedia. `arg(p)` is a pure math alternative,
  but the word "argument" has a different meaning in programming.
  angle(z) is "phase angle" of a complex number in MatLab.
  Python cmath has "phase".

cis(theta) = [cos theta, sin theta].
* Convert an angle to a unit vector.
  The `cis` function is available in many high performance math libraries,
  where it is faster than calling cos and sin separately.
  Python cmath has "cis".

Complex numbers.
[re,im] is the representation (no abstract type).
* mag(z) -- complex magnitude -- like abs(z) in MatLab
* phase(z) -- complex phase
* conj[re,im] = [re,-im] -- complex conjugate.
  Reflects a 2-vector across the X axis.
  Is it useful in Curv? Dunno.
  * cmul(z, conj(z)) == a^2 + b^2, which is real and non-negative // z==[a,b]
    and is mag(z)^2.
  * Used when computing complex division by hand to cancel out the i's.
  * sqrt(z) * conj(z) == mag(z)
  * To convert a real function to a positive value, you can square it.
    The analog for complex functions is to multiply by the conjugate.
* from polar coordinates: r*cis(theta)
* z+w
* z-w
* cmul([a,b],[c,d]) = [a*c - b*d, b*c + a*d];
  This adds the phases and multiplies the magnitudes.
  eg, cmul(vec,cis(theta)) rotates a 2D vector by angle theta.
* cdiv(z,w)
  Subtract the phases and divide the magnitudes.
* there are also exponentials, trig

Shadertoy.com classifies the following GLSL ES features as being useful.

void bool int float vec2 vec3 vec4 bvec2 bvec3 bvec4 ivec2 ivec3 ivec4 mat2 mat3 mat4 sampler2D

sampler2D represents a 2D texture

type radians (type degrees)
type degrees (type radians)
type sin (type angle)
type cos (type angle)
type tan (type angle)
type asin (type x)
type acos (type x)
type atan (type y, type x)
type atan (type y_over_x)

type pow (type x, type y)   x^y
type exp (type x)           e^x
type log (type x)
type exp2 (type x)          2^x
type log2 (type x)
type sqrt (type x)
type inversesqrt (type x)   1/sqrt(x)

type abs (type x)
type sign (type x)
type floor (type x)
type ceil (type x)
type fract (type x)
type mod (type x, float y)
type mod (type x, type y)
type min (type x, type y)
type min (type x, float y)
type max (type x, type y)
type max (type x, float y)
type clamp (type x, type minV, type maxV)
type clamp (type x, float minV, float maxV)
    x if x>minV && x<maxV, minV if x<=minV, maxV if x>=maxV
type mix (type x, type y, type a)
type mix (type x, type y, float a)
    linear blend of x and y. x*(1-a)+y*a
type step (type edge, type x)
type step (float edge, type x)
    0 if x<edge, else 1
    bit(x<edge) or bit(edge>x)
type smoothstep (type a, type b, type x)
type smoothstep (float a, float b, type x)
    0 if x<a, 1 if x>b, else interpolate between 0 and 1 using Hermite polynom.

float length (type x)
    euclidean norm
float distance (type p0, type p1)
    norm(p0-p1)
float dot (type x, type y)
vec3 cross (vec3 x, vec3 y)
type normalize (type x)
    x/norm(x)
type faceforward (type N, type I, type Nref)
    a vector that points in the same direction as Nref
type reflect (type I, type N)
type refract (type I, type N,float eta)
float determinant(mat* m)
type matrixCompMult (type x, type y)
type inverse (type inverse)

float intBitsToFloat (int value)
uint uintBitsToFloat (uint value)
int floatBitsToInt (float value)
uint floatBitsToUint (float value)

bvec lessThan(vec x, vec y)
bvec lessThan(ivec x, ivec y)
bvec lessThanEqual(vec x, vec y)
bvec lessThanEqual(ivec x, ivec y)
bvec greaterThan(vec x, vec y)
bvec greaterThan(ivec x, ivec y)
bvec greaterThanEqual(vec x, vec y)
bvec greaterThanEqual(ivec x, ivec y)
bvec equal(vec x, vec y)
bvec equal(ivec x, ivec y)
bvec equal(bvec x, bvec y)
bvec notEqual(vec x, vec y)
bvec notEqual(ivec x, ivec y)
bvec notEqual(bvec x, bvec y)
* shorter names?
* <' <=' >' >=' ==' !='
* lesser, greater, not_lesser, not_greater, equal, not_equal
  These are adjective forms. Predicate form is better.

bool any(bvec x)
bool all(bvec x)
bvec not(bvec x)

vec4 texture(sampler* sampler, vec2 coord [, float bias])
vec4 textureLod(sampler* sampler, vec2 coord, float lod)
vec4 textureGrad(sampler* sampler, vec2 P, vec2 dPdx, vec2 dPdy)
vec4 textureProj(sampler* sampler, vec3 coord [, float bias])
vec4 textureProjLod(sampler* sampler, vec3 coord, float lod)
vec4 textureProjGrad(sampler* sampler, vec3 P, vec2 dPdx, vec2 dPdy)
vec* textureSize(sampler* sampler, int lod)

type dFdx( type x ), dFdy( type x )
type fwidth( type p )
