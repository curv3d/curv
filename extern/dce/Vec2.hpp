//  Vec2.hpp
//  EmiLib 4
//
//  Created by Emil Ernerfeldt on 2012-05-27.

#ifndef DCE_VEC2_HPP
#define DCE_VEC2_HPP

#include <dce/Math.hpp>
#include <dce/Int.hpp> // isPowerOfTwo, HUGE_PRIME_0
#include <vector>
#include <ostream>
#include <cassert>


#if __APPLE__
#   include <CoreGraphics/CGGeometry.h> // CGPoint & CGSize
#endif


#ifndef QTC_BUILD
	// warning: anonymous structs are a GNU extension [-pedantic,-Wgnu]
	//#pragma clang diagnostic ignored "-Wpedantic"
	#pragma clang diagnostic ignored "-Wgnu"
#else
	#pragma GCC diagnostic ignored "-pedantic"
#endif


namespace dce
{
	struct scalar_tag{};
	
	
	//------------------------------------------------------------------------------
	
	
	template<typename T>
	class Vec2T
	{
	public:
		typedef T element_type;
		
		union {
			T m_v[2];
			struct { T x, y; };
			struct { T width, height; };
			//std::complex<T> cplx; // not POD
		};
		
		//------------------------------------------------------------------------------
		
		Vec2T() = default; // Fast - no initialization!
		Vec2T(zero_tag) : x(0), y(0) { }
		Vec2T(T x_, T y_) {
			x = x_;
			y = y_;
		}
		
		explicit Vec2T(T a) {
			x = a;
			y = a;
		}
		
		template<typename F>
		explicit Vec2T(const Vec2T<F>& v) {
			x = (T)v.x;
			y = (T)v.y;
		}
		
#if __APPLE__
		explicit Vec2T(const CGPoint& p) {
			x = (T)p.x;
			y = (T)p.y;
		}
		explicit Vec2T(const CGSize& s) {
			x = (T)s.width;
			y = (T)s.height;
		}
		
		operator CGPoint() const
		{
			return CGPoint{x,y};
		}
		
		operator CGSize() const
		{
			return CGSize{x,y};
		}
#endif // __APPLE__
		
		//------------------------------------------------------------------------------
		// Static constructors:
		
		// Returns the unit-vector of a certain angle (angle=0, vector=[1,0], angle=Pi/2, vector=[0,1])
		// The invert of Vec2T::angle()
		static const Vec2T angled(real a) {
			Vec2T ret = Vec2T(std::cos(a), std::sin(a));
			ret = idealizedNormal(ret);
			return ret;
		}
		
		/* Given a unit-vector, if the angle is VERY close to being a factor of 45∞
		 the the normal will be modified to be perfect.
		 */
		static const Vec2T idealizedNormal(Vec2T vec)
		{
			if (isZero(abs(vec.x) - abs(vec.y)))
			{
				// N * 45 deg
				vec.x = signF(vec.x);
				vec.y = signF(vec.y);
				vec *= std::sqrt((T)2) / 2;
			}
			else
			{
				for (unsigned a=0; a<2; ++a)
				{
					if (isZero(abs(vec[a])))
					{
						vec[ a ] = 0;
						vec[1-a] = (T)sign(vec[1-a]);
						break;
					}
				}
			}
			return vec;
		}
		
		//------------------------------------------------------------------------------
		
		T  X() const { return x; }
		T& X()       { return x; }
		T  Y() const { return y; }
		T& Y()       { return y; }
		
		T  operator[](int i) const { assert(0<=i && i<2); return m_v[i]; }
		T& operator[](int i)       { assert(0<=i && i<2); return m_v[i]; }
		T  operator[](unsigned i) const { assert(0<=i && i<2); return m_v[i]; }
		T& operator[](unsigned i)       { assert(0<=i && i<2); return m_v[i]; }
		
		//------------------------------------------------------------------------------
		
		T lenSq() const     { return x*x + y*y; }
		T sq() const        { return x*x + y*y; }
		//real len() const  { return std::sqrt((real)lenSq()); }
		real len() const    { return std::hypot(x,y); }
		
		// Returns length
		T normalize() {
			T l = len();
			if (l != 0) {
				*this *= 1.0f/l;
			}
			return l;
		}
		
		// The angle of the vector. Vec2T(1,0).GetAngle() == 0 and Vec2T(0,1).GetAngle() == Pi/2
		// Returns an angle in [-pi, +pi]
		// The invert of TVector2::Angled
		real angle() const
		{
			if (x == 0 && y == 0)
				return 0;
			return std::atan2(y, x);
		}
		
		T area() const { return x*y; }
		
		T min() const { return std::min(x,y); }
		T max() const { return std::max(x,y); }
		T minAbs() const { return std::min(std::abs(x), std::abs(y)); }
		T maxAbs() const { return std::max(std::abs(x), std::abs(y)); }
		
		//------------------------------------------------------------------------------
		
		constexpr Vec2T operator-() const {
			return Vec2T(-x, -y);
		}
		
		constexpr const Vec2T& operator+() const {
			return *this;
		}
		
		//------------------------------------------------------------------------------
		
		Vec2T& operator+=(const Vec2T& b) {
			x += b.x;
			y += b.y;
			return *this;
		}
		
		Vec2T& operator-=(const Vec2T& b) {
			x -= b.x;
			y -= b.y;
			return *this;
		}
		
		Vec2T& operator*=(T s) {
			x *= s;
			y *= s;
			return *this;
		}
		
		//------------------------------------------------------------------------------
		
		friend constexpr const Vec2T operator+(const Vec2T& a, const Vec2T& b) {
			return Vec2T(a.x+b.x, a.y+b.y);
		}
		friend constexpr const Vec2T operator-(const Vec2T& a, const Vec2T& b) {
			return Vec2T(a.x-b.x, a.y-b.y);
		}
		friend constexpr const Vec2T operator*(T s, const Vec2T& a) {
			return Vec2T(a.x*s, a.y*s);
		}
		friend constexpr const Vec2T operator*(const Vec2T& a, T s) {
			return Vec2T(a.x*s, a.y*s);
		}
		friend constexpr const Vec2T operator/(const Vec2T& a, T s) {
			return Vec2T(a.x/s, a.y/s);
		}
		
		//------------------------------------------------------------------------------
		
		friend constexpr bool operator==(const Vec2T& a, const Vec2T& b) {
			return a.x==b.x && a.y==b.y;
		}
		friend constexpr bool operator!=(const Vec2T& a, const Vec2T& b) {
			return a.x!=b.x || a.y!=b.y;
		}
		friend constexpr bool operator<(const Vec2T& a, const Vec2T& b) {
			return (a.x!=b.x ? a.x<b.x : a.y<b.y);
		}
		
		//------------------------------------------------------------------------------
		
		friend T dot(const Vec2T& a, const Vec2T& b) {
			return a.x*b.x + a.y*b.y;
		}
		
		friend T cross(const Vec2T& a, const Vec2T& b) {
			return a.x * b.y - a.y * b.x;
		}
		
		friend Vec2T cross(const Vec2T& a, T z) {
			return TVector2(z*a.y, -z*a.x);
		}
		
		friend Vec2T cross(T z, const Vec2T& a) {
			return TVector2(-z*a.y, z*a.x);
		}
		
		//------------------------------------------------------------------------------
				
		friend T dist(const Vec2T& a, const Vec2T& b) {
			return (a-b).len();
		}
		
		friend T distSq(const Vec2T& a, const Vec2T& b) {
			return (a-b).lenSq();
		}
		
		// Element-wise
		static Vec2T min(const Vec2T& a, const Vec2T& b) {
			return Vec2T(std::min(a.x, b.x), std::min(a.y, b.y));
		}
		
		// Element-wise
		static Vec2T max(const Vec2T& a, const Vec2T& b) {
			return Vec2T(std::max(a.x, b.x), std::max(a.y, b.y));
		}
	};
	
	typedef Vec2T<real> Vec2;
	typedef Vec2T<float> Vec2f;
	typedef Vec2T<double> Vec2d;
	typedef Vec2T<int> Vec2i;
	typedef Vec2T<unsigned> Vec2u;
	typedef Vec2T<uint16_t> Vec2u16;
	typedef Vec2T<uint8_t> Vec2u8;
	
	typedef std::vector<Vec2> Vec2List;
	
	static_assert(sizeof(Vec2) == 2*sizeof(real), "Pack");
	
	static_assert(std::is_pod<Vec2>::value,                "is_pod");
	static_assert(std::is_standard_layout<Vec2>::value,    "is_standard_layout");
	static_assert(std::is_trivial<Vec2>::value,            "is_trivial");
	static_assert(std::is_trivially_copyable<Vec2>::value, "is_trivially_copyable");
	
	//------------------------------------------------------------------------------
	// utils
	
	template<typename T>
	inline const Vec2T<T> rot90CCW(const Vec2T<T>& v)
	{
		return Vec2T<T>(-v.y, v.x);
	}
	
	template<typename T>
	inline const Vec2T<T> rot90CW(const Vec2T<T>& v)
	{
		return Vec2T<T>(v.y, -v.x);
	}
	
	//------------------------------------------------------------------------------
	
	inline Vec2 normalized(const Vec2& v)
	{
		auto len = v.len();
		if (len == 0)
			return Vec2(0,0);
		else
			return v / len;
	}
	
	// Safe: normalize 0, return zero.
	inline Vec2 normalizedOrZero(const Vec2& v)
	{
		auto len = v.len();
		if (isZero(len))
			return Vec2(0,0);
		else
			return v / len;
	}
	
	/* Quickly calculated the difference in angle between two vectors b and a. */
	inline real vec2AngleDiff(const Vec2& b, const Vec2& a)
	{
		return b.angle() - a.angle(); // TODO: optimize
	}
	
	inline bool isFinite(Vec2 v)
	{
		return isFinite(v.x) && isFinite(v.y);
	}
	
	template<typename T>
	inline Vec2T<T> div(T t, const Vec2T<T>& vec)
	{
		return Vec2T<T>(t/vec.x, t/vec.y);
	}
	
	template<typename T>
	inline Vec2T<T> div(Vec2T<T> a, const Vec2T<T>& b)
	{
		return Vec2T<T>(a.x/b.x, a.y/b.y);
	}
	
	template<typename T>
	inline Vec2T<T> mul(Vec2T<T> a, const Vec2T<T>& b)
	{
		return Vec2T<T>(a.x*b.x, a.y*b.y);
	}
	
	// Works like in glsl
	inline Vec2 reflect(const Vec2& d, const Vec2& n) {
		return d - 2*dot(d, n)*n;
	}
	
	
	//------------------------------------------------------------------------------
	
	
	inline bool isPowerOfTwo(Vec2u vec) {
		return isPowerOfTwo(vec.x)
		&&
		isPowerOfTwo(vec.y);
	}
	
	inline Vec2 round(Vec2 v) {
		return Vec2(std::round(v.x), std::round(v.y));
	}
	
	inline Vec2i roundI(Vec2 v) {
		return Vec2i(roundI(v.x), roundI(v.y));
	}
	
	inline Vec2i floorI(Vec2 v) {
		return Vec2i(floorI(v.x), floorI(v.y));
	}
	
	inline Vec2i ceilI(Vec2 v) {
		return Vec2i(ceilI(v.x), ceilI(v.y));
	}
	
	inline Vec2i sign(Vec2 v) {
		return Vec2i(sign(v.x), sign(v.y));
	}
	
	template<typename T>
	inline Vec2T<T> abs(Vec2T<T> v) {
		return {abs(v.x), abs(v.y)};
	}
	
	// ensure [0,size)
	inline Vec2i clampToSize(Vec2i p, Vec2i size) {
		return Vec2i::min(size-Vec2i(1), Vec2i::max(Vec2i(0), p));
	}
	
	template<typename T>
	inline Vec2T<T> clamp(Vec2T<T> v, Vec2T<T> mn, Vec2T<T> mx) {
		Vec2T<T> ret;
		for (int d=0; d<2; ++d)
			ret[d] = clamp(v[d], mn[d], mx[d]);
		return ret;
	}
	
	template<typename T>
	inline constexpr T sqr(Vec2T<T> v) {
		return v.sq();
	}
	
	//------------------------------------------------------------------------------
	
#if 1
	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, const Vec2T<T>& v)
	{
		return os << "<" << v.x << ", " << v.y << ">";
	}
#endif
}

namespace std
{
	template<typename T> struct hash<dce::Vec2T<T>> {
		size_t operator()(const dce::Vec2T<T>& val) const {
			return std::hash<T>()(val.x) + std::hash<T>()(val.y) * dce::HUGE_PRIME_0;
		}
	};
}

#endif
