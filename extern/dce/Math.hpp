//
//  Math.hpp
//  EmiLib 3.0
//
//  Created by emilk on 2012-09-09.

#ifndef DCE_MATH_HPP
#define DCE_MATH_HPP

#include <cstdint>
#include <cmath>
#include <limits>
#include <algorithm>

namespace dce
{
#define IS_FLOATING_POINT(Type) (std::is_floating_point<Type>::value)
#define STATIC_ASSERT_FLOATING_POINT(Type) static_assert(IS_FLOATING_POINT(Type), "Must be a float")
	
	//------------------------------------------------------------------------------
	
	typedef double real;
	
	constexpr float  PIf  = (float)3.1415926535897932384626433;
	constexpr real   PI   = PIf;
	constexpr real   TAU  = 2*PI; // Oh yes. http://tauday.com/tau-manifesto.pdf
	constexpr real   NaN  = std::numeric_limits<real>::quiet_NaN();
	constexpr real   INF  = std::numeric_limits<real>::infinity();
	
	const float SQRT_3 = std::sqrt(3.0f); // Box: outer radius over inner
	
	//------------------------------------------------------------------------------
	// Numbers smaller than eps are easily rounding errors
	
	template<typename F>
	inline constexpr F Eps();
	
	template<>
	inline constexpr float Eps() { return 2e-5f; }
	
	template<>
	inline constexpr double Eps() { return 1e-11; } // Way more than DBL_EPSILON... but whatever.
	
	const double EPSd = Eps<double>();
	const float  EPSf = Eps<float>();
	const float  EPS  = Eps<real>();
	
	//------------------------------------------------------------------------------

	template<typename F>
	constexpr int floorI(F f) {
		return (int)floor(f); // FIXME: doesn't work as constexpr!
	}
	
	template<typename F>
	constexpr int ceilI(F f) {
		return (int)ceil(f);
	}
	
	// nearest integer, rounding away from zero in halfway cases 
	template<typename F>
	constexpr int roundI(F f) {
		//return (int)round(f); // FIXME: doesn't work as constexpr!
		//return floorI(f+0.5f);
		return int(f < 0 ? f-0.5f : f+0.5f); // int(foo) rounds towards zero
	}
	static_assert(roundI(+0.4) == 0,  "roundI test");
	static_assert(roundI(+0.5) == +1, "roundI test");
	static_assert(roundI(+0.6) == +1, "roundI test");
	static_assert(roundI(-0.4) == 0,  "roundI test");
	static_assert(roundI(-0.5) == -1, "roundI test");
	static_assert(roundI(-0.6) == -1, "roundI test");
	
	template<typename F>
	constexpr F abs(F f) {
		return (f<0 ? -f : f);
	}
	static_assert(abs(-.1f) == +.1f, "abs");
	
	template<typename T>
	inline constexpr int sign(const T& val)
	{
		return (val<0 ? -1 : val>0 ? +1 : 0);
	}
	
	template<typename T>
	inline constexpr T signF(const T& val)
	{
		return (val<0 ? (T)-1 : val>0 ? (T)+1 : (T)0);
	}
	
	template<typename T>
	inline constexpr T clamp(T x, T mn, T mx) {
		return (x < mn ? mn : x > mx ? mx : x);
	}
	
	template<typename T>
	inline constexpr T clamp(T x) {
		return clamp<T>(x, 0, 1);
	}
	
	template<typename T>
	inline constexpr T lerp(const T& a, const T& b, float t) {
		return a*(1-t) + b*t;
	}
	
	template<typename T>
	inline constexpr T lerp(const T& a, const T& b, double t) {
		return a*(1-t) + b*t;
	}
	
	// For color-components:
	template<>
	inline constexpr uint8_t lerp(const uint8_t& a, const uint8_t& b, float t)
	{
		return (uint8_t)roundI((1-t)*a + t*b);
	}
	
	// IS THIS VERBOSE ENOUGH FOR YOU COMPILER? HUH? IS IT!?
	template<typename T>
	constexpr auto average(const T& a, const T& b) -> decltype((a+b)/2) {
		return (a+b)/2;
	}
	
	template<typename T>
	inline constexpr T sqr(T x) {
		return x*x;
	}
	
	template<typename T>
	inline constexpr T cube(T x) {
		return x*x*x;
	}
	
	inline real deg2Rad(real a) {
		return a * PIf / 180;
	}
	
	// To [-PI, +PI]
	template<typename T>
	inline T wrapAngle(T a) {
		while (a < -PIf) a += TAU;
		while (a > +PIf) a -= TAU;
		return a;
	}
	
	inline real lerpAngle(real a0, real a1, float t)
	{
		return a0 + t * wrapAngle(a1-a0);
	}
	
	template<typename T>
	inline void sort(T& a, T& b) {
		if (b<a)
			std::swap(a,b);
	}

	inline bool isFinite(real t) {
		return std::isfinite(t) && !std::isnan(t);
	}
	
	inline constexpr bool isZero(real v, real eps = EPSf) {
		return abs(v) <= eps;
	}
	
	template<typename T>
	inline constexpr T max(T a, T b) {
		return a>b ? a : b;
	}
	
	template<typename T>
	inline constexpr bool equals(T a, T b, T eps = Eps<T>())
	{
#if 0
		bad
		return abs(a-b) <= eps * max(abs(a),abs(b));
#elif 0
		// FIXME: std::isnan etc aren't constexpr... :(
		return
		std::isnan(a) || std::isnan(b) ? false :
		std::isinf(a) && std::isinf(b) ? sign(a) == sign(b) :
		std::isinf(a) || std::isinf(b) ? false :
		a*b==0 ? abs(a+b) <= eps :               // Any zero?
		a*b<0  ? abs(a-b) <= eps :               // Different signs?
		abs(a-b) <= eps * max(abs(a),abs(b));
#else
		return abs(a-b) < eps;
#endif
	}
	
	//static_assert(!equals<float>(+NaN, +NaN),  "equals broken");
	//static_assert( equals<float>(+INF, +INF),             "equals broken");
	//static_assert(!equals<float>(+INF, -INF),             "equals broken");
	static_assert( equals<float>(+1, +1),                   "equals broken");
	static_assert( equals<float>(-1, -1),                   "equals broken");
	static_assert(!equals<float>(-1, +1),                   "equals broken");
	static_assert( equals<float>(+1, +1.000001, 1e-5),      "equals broken");
	static_assert(!equals<float>(+1, +1.0001, 1e-5),        "equals broken");
	//static_assert( equals<float>(+1000000, +1000001, 1e-5),        "equals broken"); // TODO
	
	/*
	 Interpolate the cubic Hermite spline from:
	 point p0 with tangent m0 at t=0.
	 to
	 point p1 with tangent m1 at t=1.
	 */
	template<typename T>
	inline T hermite(T p0, T m0, T p1, T m1, float t)
	{
		float t2 = t*t;
		float t3 = t2*t;
		return (2*t3 - 3*t2 + 1)*p0 + (t3 - 2*t2 + t)*m0 + (-2*t3 + 3*t2)*p1 + (t3 - t2)*m1;
	}
	
	// For t=[0,1], returns [0,1] with a derivate of zero at both ends
	template<typename T>
	constexpr T easeInEaseOut(T t)
	{
		return 3*t*t - 2*t*t*t;
	}
	
	// normalized sinc function
	template<typename F>
	inline F sinc(F x)
	{
		STATIC_ASSERT_FLOATING_POINT(F);
		if (x==0) return 1; // Prevent singularity
		return sin(PI*x)/(PI*x);
	}
		
	// t is [0,1] between p1 and p2
	template<typename F, typename T>
	inline T catmullRom(F t, T p0, T p1, T p2, T p3) {
		STATIC_ASSERT_FLOATING_POINT(F);
		return 0.5f * (
							p0 * t*((2-t)*t-1)   +
							p1 * (t*t*(3*t-5)+2) +
							p2 * t*((4-3*t)*t+1) +
							p3 * (t-1)*t*t
							);
	}
	
	template<typename F, typename T>
	inline T catmullRom(F t, T points[4]) {
		return catmullRom(t, points[0], points[1], points[2], points[3]);
	}
	
	
	//------------------------------------------------------------------------------
	
	
	// Returns the next float greater than 'arg'.
	// if NAN, will return NAN.
	// if INF, will return INF.
	// if -INF, will return std::numeric_limit<float>::min()
	// Will allow denormal numbers.
	inline float nextFloat(float arg);
	
	//------------------------------------------------------------------------------
	
	// Tag class
	class zero_tag
	{
	public:
		static const zero_tag s_instance;
		
		zero_tag(const zero_tag&){}
	private:
		zero_tag(){}
		zero_tag& operator=(zero_tag&){return *this;}
	};
	
	/*
	 Typeless, unitless, dimensionless zero.
	 Usage:: Vec2 v = Zero; assert(v == Zero);
	*/
	extern const zero_tag Zero;
}

#endif
