#ifndef DCE_VEC3_HPP
#define DCE_VEC3_HPP

#include <cmath>
#include <dce/Vec2.hpp>

namespace dce
{	
	/* Tagging is a way to get typesafe (strong) typedefs:
	 
	 struct RGBAf_tag{};
	 typedef Vec4T<float, RGBA_tag> RGBAf;
	 */
	template<typename T, class Tag = scalar_tag>
	class Vec3T
	{
	public:
		typedef T element_type;
		typedef T* iterator;
		typedef const T* const_iterator;
		
		union {
			T m_v[3];
			struct { T x, y, z; };
			struct { T r, g, b; };
			struct { Vec2T<T> xy; T z_; };
		};
		
		
		static const Vec3T Axes[3];/* = {
			{1,0,0}, {0,1,0}, {0,0,1}
		};*/
		
		//------------------------------------------------------------------------------
		
		Vec3T() = default; // Fast - no initialization!
		Vec3T(zero_tag) : x(0), y(0), z(0) { }
		explicit Vec3T(T v);
		Vec3T(const Vec2& v, T z);
		
#if 1
		Vec3T(T x, T y, T z);
#else
		template<typename T2>
		explicit Vec3T(T2 x_, T2 y_, T2 z_);
#endif
		
		//Vec3T(const Vec3T& v) = default;
		
		template<typename OtherTag>
		explicit Vec3T(Vec3T<T, OtherTag> v) : x(v.x), y(v.y), z(v.z) { }
		
		// Explicit cast:
		template<typename F, typename OtherTag>
		explicit Vec3T(const Vec3T<F, OtherTag>& v) : x((T)v.x), y((T)v.y), z((T)v.z) { }
		
		//------------------------------------------------------------------------------
		
		T*        data()        { return m_v; }
		const T*  data() const  { return m_v; }
		
		static constexpr unsigned  size()   { return 3; }
		iterator                   begin()        { return data(); }
		const_iterator             begin() const  { return data(); }
		iterator                   end()          { return data() + size(); }
		const_iterator             end() const    { return data() + size(); }
		
		
		//------------------------------------------------------------------------------
		
		//const Vec2 xy const { return Vec2(x,y); }
		
		T  operator[](int i) const { return m_v[i]; }
		T& operator[](int i)       { return m_v[i]; }
		
		T  operator[](unsigned i) const { return m_v[i]; }
		T& operator[](unsigned i)       { return m_v[i]; }
		
		//------------------------------------------------------------------------------
		
		T lenSq() const { return x*x + y*y + z*z; }
		T len()   const { return std::sqrt((T)lenSq()); }
		bool isNormalized() const { return equals<T>(lenSq(), 1); }
		
		// Returns length
		T normalize() {
			T l = len();
			if (l != 0) {
				*this *= 1.0f/l;
			}
			return l;
		}
		
		T volume() const { return x*y*z; }
		
		T min() const { return std::min({x,y,z}); }
		T max() const { return std::max({x,y,z}); }
		T minAbs() const { return std::min({std::abs(x), std::abs(y), std::abs(z)}); }
		T maxAbs() const { return std::max({std::abs(x), std::abs(y), std::abs(z)}); }
		
		unsigned minAxis() const { return (x<=y && x<=z ? 0 :
													  y<=x && y<=z ? 1 : 2); }
		unsigned maxAxis() const { return (x>=y && x>=z ? 0 :
													  y>=x && y>=z ? 1 : 2); }
		unsigned maxAbsAxis() const { return abs(*this).maxAxis(); }
		
		//------------------------------------------------------------------------------
		
		//Vec3T& operator =  (const Vec3T& v) = default;
		
		Vec3T  operator +  (const Vec3T& v) const;
		Vec3T& operator += (const Vec3T& v);
		
		Vec3T  operator -  (const Vec3T& v) const;
		Vec3T& operator -= (const Vec3T& v);
		
#if 1
		inline friend Vec3T operator * (const Vec3T& v, T s) { return {v.x*s, v.y*s, v.z*s}; }
#else
		template<typename F>
		inline friend auto operator * (const Vec3T& v, F s) -> Vec3T<decltype(T(0)*F(0))> {
			return Vec3T<decltype(v.x*s)>{v.x*s, v.y*s, v.z*s};
		}
#endif
		
		inline friend Vec3T operator * (T s, const Vec3T& v) { return {v.x*s, v.y*s, v.z*s}; }
		Vec3T& operator *= (T v);
		
		Vec3T  operator *  (const Vec3T& v) const;
		Vec3T& operator *= (const Vec3T& v);
		
		Vec3T  operator /  (T v) const;
		Vec3T& operator /= (T v);
		
		bool operator == (const Vec3T& v) const;
		bool operator != (const Vec3T& v) const;
		
		constexpr       Vec3T  operator -() const { return Vec3T(-x, -y, -z); }
		constexpr const Vec3T& operator +() const { return *this; }
		
		//------------------------------------------------------------------------------
		
		friend T dot(const Vec3T& a, const Vec3T& b) {
			return a.x*b.x + a.y*b.y + a.z*b.z;
		}
		
		friend Vec3T cross(const Vec3T& lhs, const Vec3T& rhs) {
			return {
				lhs[1]*rhs[2] - lhs[2]*rhs[1],
				lhs[2]*rhs[0] - lhs[0]*rhs[2],
				lhs[0]*rhs[1] - lhs[1]*rhs[0]
			};
		}
		
		// Scalar multiplication
		friend Vec3T mul(const Vec3T& lhs, const Vec3T& rhs)
		{
			return Vec3T(lhs[0]*rhs[0], lhs[1]*rhs[1], lhs[2]*rhs[2]);
		}
		
		friend Vec3T div(const Vec3T& lhs, const Vec3T& rhs) {
			return {
				lhs[0] / rhs[0],
				lhs[1] / rhs[1],
				lhs[2] / rhs[2]
			};
		}
		
		friend T dist(const Vec3T& a, const Vec3T& b) {
			return (a-b).len();
		}
		
		friend T distSq(const Vec3T& a, const Vec3T& b) {
			return (a-b).lenSq();
		}
		
		//------------------------------------------------------------------------------
		
		Vec3T<T> untag() const { return Vec3T<T>{x,y,z}; }
	};
	
	
	//------------------------------------------------------------------------------
	
	
	template<typename T>
	inline std::ostream& operator<<(std::ostream& os, const Vec3T<T>& v)
	{
		return os << "<" << v.x << ", " << v.y << ", " << v.z << ">";
	}
	
	
	
	template<typename T, class Tag>
	const Vec3T<T,Tag> Vec3T<T,Tag>::Axes[3] = {
		{1,0,0}, {0,1,0}, {0,0,1}
	};
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>::Vec3T(T v)
	:x(v), y(v), z(v)
	{
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>::Vec3T(T X, T Y, T Z)
	:x(X), y(Y), z(Z)
	{
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>::Vec3T(const Vec2& v, T z)
	:x(v.x), y(v.y), z(z)
	{
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>  Vec3T<T, Tag>::operator +  (const Vec3T& v) const
	{
		return Vec3T(x+v.x, y+v.y, z+v.z);
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>& Vec3T<T, Tag>::operator += (const Vec3T& v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		
		return *this;
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>  Vec3T<T, Tag>::operator -  (const Vec3T& v) const
	{
		return Vec3T(x-v.x, y-v.y, z-v.z);
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>& Vec3T<T, Tag>::operator -= (const Vec3T& v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		
		return *this;
	}
	
	/*
	template<typename T, class Tag>
	inline Vec3T<T, Tag> operator * (const Vec3T<T, Tag>& v, T s) {
		return {v.x*s, v.y*s, v.z*s};
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag> operator * (T s, const Vec3T<T, Tag>& v) {
		return {v.x*s, v.y*s, v.z*s};
	}
	 */
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>& Vec3T<T, Tag>::operator *= (T v)
	{
		x *= v;
		y *= v;
		z *= v;
		
		return *this;
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>  Vec3T<T, Tag>::operator *  (const Vec3T& v) const
	{
		return Vec3T(x*v.x, y*v.y, z*v.z);
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>& Vec3T<T, Tag>::operator *= (const Vec3T& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		
		return *this;
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>  Vec3T<T, Tag>::operator /  (T v) const
	{
		return Vec3T(x/v, y/v, z/v);
	}
	
	template<typename T, class Tag>
	inline Vec3T<T, Tag>& Vec3T<T, Tag>::operator /= (T v)
	{
		x /= v;
		y /= v;
		z /= v;
		
		return *this;
	}
	
	template<typename T, class Tag>
	inline bool Vec3T<T, Tag>::operator == (const Vec3T& v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}
	
	template<typename T, class Tag>
	inline bool Vec3T<T, Tag>::operator != (const Vec3T& v) const
	{
		return !((*this) == v);
	}
	
	//------------------------------------------------------------------------------
	
	typedef unsigned char byte;
	
	typedef Vec3T<real>      Vec3;
	typedef Vec3T<float>     Vec3f;
	typedef Vec3T<double>    Vec3d;
	typedef Vec3T<int>       Vec3i;
	typedef Vec3T<unsigned>  Vec3u;
	typedef Vec3T<uint16_t>  Vec3u16;
	typedef Vec3T<uint8_t>   Vec3u8;  // e.g. RGB
	typedef Vec3T<int8_t>    Vec3s8;  // e.g. normal
	
	typedef std::vector<Vec3> Vec3List;
	
	static_assert(sizeof(Vec3u8)   == 3*sizeof(byte),      "Pack");
	static_assert(sizeof(Vec3u16)  == 3*sizeof(uint16_t),  "Pack");
	static_assert(sizeof(Vec3)     == 3*sizeof(real),      "Pack");
	
	
	static_assert(std::is_pod<Vec3>::value,                 "is_pod");
	static_assert(std::is_standard_layout<Vec3>::value,     "is_standard_layout");
	static_assert(std::is_trivial<Vec3>::value,             "is_trivial");
	static_assert(std::is_trivially_copyable<Vec3>::value,  "is_trivially_copyable");
	
	static_assert(std::is_pod<Vec3u8>::value,                 "is_pod");
	static_assert(std::is_standard_layout<Vec3u8>::value,     "is_standard_layout");
	static_assert(std::is_trivial<Vec3u8>::value,             "is_trivial");
	static_assert(std::is_trivially_copyable<Vec3u8>::value,  "is_trivially_copyable");
	
	
	
	//------------------------------------------------------------------------------
	// Utilities
	
	inline Vec3 normalized(const Vec3& v)
	{
		auto len = v.len();
		if (len == 0) {
			return Zero;
		} else {
			return v / len;
		}
	}
	
	// Safe: normalize 0, return zero.
	inline Vec3 normalizedOrZero(const Vec3& v)
	{
		auto len = v.len();
		if (isZero(len))
			return Vec3(0);
		else
			return v / len;
	}
		
	inline Vec3 round(Vec3 v) {
		return Vec3(std::round(v.x), std::round(v.y), std::round(v.z));
	}
	
	inline Vec3i roundI(Vec3 v) {
		return Vec3i(roundI(v.x), roundI(v.y), roundI(v.z));
	}
	
	inline Vec3i floorI(Vec3 v) {
		return Vec3i(floorI(v.x), floorI(v.y), floorI(v.z));
	}
	
	inline Vec3i ceilI(Vec3 v) {
		return Vec3i(ceilI(v.x), ceilI(v.y), ceilI(v.z));
	}
	
	inline Vec3i sign(Vec3 v) {
		return Vec3i(sign(v.x), sign(v.y), sign(v.z));
	}
	
	// Works like in glsl
	inline Vec3 reflect(const Vec3& d, const Vec3& n) {
		return d - 2*dot(d, n)*n;
	}
	
	inline bool isFinite(Vec3 v)
	{
		return isFinite(v.x) && isFinite(v.y) && isFinite(v.z);
	}
		
	template<typename T, class Tag>
	inline Vec3T<T,Tag> abs(const Vec3T<T,Tag>& v) {
		return {std::abs(v.x), std::abs(v.y), std::abs(v.z)};
	}
	
	template<typename T, class Tag>
	inline Vec3T<T,Tag> clamp(Vec3T<T,Tag> v, Vec3T<T,Tag> mn, Vec3T<T,Tag> mx) {
		Vec3T<T,Tag> ret;
		for (int d=0; d<3; ++d)
			ret[d] = clamp(v[d], mn[d], mx[d]);
		return ret;
	}
	
	// e.g. up is [0,0,1], we return things like [x,y,0]
	inline Vec3 projectOnto(Vec3 v, Vec3 up) {
		return v - up * dot(v, up);
	}
}
#endif
