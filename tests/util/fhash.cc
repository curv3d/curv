// c++ -std=c++14 fhash.cc

#include <cstdint>
#include <cstdio>
#include <random>
#include <cassert>

// Hash a float number f to n, where 0 <= n < 1, using all the bits in f.
// The result is uniformly distributed between 0 and 1.
//
// The result is highly correlated with the mantissa of f.
// This is not a good hash function, it is a low level operation for
// constructing hash functions, and is intended to replace the use of `frac`
// for reducing a number to the range 0...1 as the final step in computing
// a hash.
float fhash(float f)
{
    union {
        float f;
        uint32_t u;
    } data;
    data.f = f;
//    printf("fhash: %g 0x%x", data.f, data.u);
    data.u =
        ((data.u & 0x007F'FFFF) // strip sign bit and exponent
        | 0x3F80'0000) // set sign to 0 and exponent to 0
        ^ (data.u >> 9) // xor exponent and sign on top of mantissa
        ;
    // If f is normalized, then 1 <= data.f < 2.
    // If f is denormalized, then 0 <= data.f < 1. This includes f==0.
//    printf(" -> %g 0x%x\n", data.f, data.u);
    return data.f - floorf(data.f);
}
double dhash(double f)
{
    union {
        double f;
        uint64_t u;
    } data;
    data.f = f;
//    printf("dhash: %g 0x%lx", data.f, data.u);
    data.u =
        ((data.u & 0x000F'FFFF'FFFF'FFFF) // strip sign bit and exponent
        | 0x3FF0'0000'0000'0000) // set sign to 0 and exponent to 0
        ^ (data.u >> 12) // xor exponent and sign on top of mantissa
        ;
    // If f is normalized, then 1 <= data.f < 2.
    // If f is denormalized, then 0 <= data.f < 1. This includes f==0.
//    printf(" -> %g 0x%lx\n", data.f, data.u);
    return data.f - floor(data.f);
}

void fmain()
{
    /* Initialise. Do this once (not for every random number). */
    std::random_device rd;
    std::mt19937_64 gen(rd());

    /* This is where you define the number generator */
    std::uniform_int_distribution<uint32_t> dis;

    static unsigned buckets[256];

    /* A few random numbers: */    
    for (int n=0; n<1000000; ++n) {
        union {
            float f;
            uint32_t u;
        } data;
        data.u = dis(gen);
        if (data.f == data.f) {
            float h = fhash(data.f);
            unsigned i = unsigned(h * 256.0);
            assert(i >= 0 && i <= 255);
            ++buckets[i];
//            printf("%g -> %g\n", data.f, h);
        }
    }

    for (int i = 0; i < 256; ++i)
        printf("%d ", buckets[i]);
    printf("\n");
}
void dmain()
{
    /* Initialise. Do this once (not for every random number). */
    std::random_device rd;
    std::mt19937_64 gen(rd());

    /* This is where you define the number generator */
    std::uniform_int_distribution<uint64_t> dis;

    static unsigned buckets[256];

    /* A few random numbers: */    
    for (int n=0; n<1000000; ++n) {
        union {
            double f;
            uint64_t u;
        } data;
        data.u = dis(gen);
        if (data.f == data.f) {
            float h = dhash(data.f);
            unsigned i = unsigned(h * 256.0);
            assert(i >= 0 && i <= 255);
            ++buckets[i];
//            printf("%g -> %g\n", data.f, h);
        }
    }

    for (int i = 0; i < 256; ++i)
        printf("%d ", buckets[i]);
    printf("\n");
}
int main()
{
    dmain();
    return 0;
}
