//
//  Int.hpp
//  EmiLib 3.0
//
//  Created by emilk on 2012-11-03.

#ifndef DCE_INT_HPP
#define DCE_INT_HPP

namespace dce
{
	// Returns the next power-of-two HIGHER OR EQUAL to k.
	inline size_t nextPowerOfTwo(size_t k)
	{
		if (k==0)
			return 1;
		
		k--;
		for (size_t i=1; i<sizeof(size_t)*8; i=i*2)
			k = (k | (k >> i));
		return k+1;
	}
	
	inline constexpr bool isPowerOfTwo(size_t k)
	{
		return (k & (k-1))==0;
	}
	
	// Returns v if v%N==0
	inline constexpr int nextMultipleOfN(int v, int N)
	{
		return (v==0 ? 0 : ((v-1)/N + 1)*N);
	}
	
	// Returns v if v%N==0
	inline constexpr int prevMultipleOfN(int v, int N)
	{
		return (v/N)*N;
	}
	
	inline constexpr bool isMultipleOfN(int v, int N)
	{
		return (v/N)*N == v;
	}
	
	// Good for hashing
	constexpr unsigned HUGE_PRIME_0 = 0x8da6b343;
	constexpr unsigned HUGE_PRIME_1 = 0xd8163841;
	constexpr unsigned HUGE_PRIME_2 = 0xcb1ab31f;
}

#endif
