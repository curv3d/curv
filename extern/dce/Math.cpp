//
//  Math.cpp
//  EmiLib 3.0
//
//  Created by emilk on 2012-10-15.

#include <dce/Math.hpp>
#include <cmath>
#include <cassert>


namespace dce
{
	static_assert(std::numeric_limits<float>::has_denorm,      "has_denorm");
	/* See
	 http://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/
	 for the potential portability problems with the union and bit-fields below.
	 */
	union Float_t
	{
		Float_t(float num = 0.0f) : f(num) {}
		// Portable extraction of components.
		bool Negative() const { return (i >> 31) != 0; }
		int32_t RawMantissa() const { return i & ((1 << 23) - 1); }
		int32_t RawExponent() const { return (i >> 23) & 0xFF; }
		
		int32_t i;
		float f;
#ifdef _DEBUG
		struct
		{   // Bitfields for exploration. Do not use in production code.
			uint32_t mantissa : 23;
			uint32_t exponent : 8;
			uint32_t sign : 1;
		} parts;
#endif
	};
	static_assert(sizeof(Float_t)==sizeof(float), "Pack");
	
	float nextFloat(float arg)
	{
		if (std::isnan(arg)) return arg;
		if (arg==+INF)  return +INF; // Can't go higher.
		if (arg==0)     return std::numeric_limits<float>::denorm_min();
		
		Float_t f = arg;
		f.i += 1;
		assert(f.f > arg);
		return f.f;
	}
	
		
	//------------------------------------------------------------------------------
	
	const zero_tag zero_tag::s_instance;
	const zero_tag Zero(zero_tag::s_instance);
}
