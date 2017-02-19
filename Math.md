# math library

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
