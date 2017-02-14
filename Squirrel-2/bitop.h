#pragma once


#if defined(_MSC_VER)
#endif
#if defined(__GNUC__) 
#endif

#include "fundation.h"


#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if defined(__GNUC__) 
#include <immintrin.h>
#endif

uint64_t find_lsb(uint64_t &bb);
uint64_t pop_lsb(uint64_t &bb);
uint64_t find_msb(uint64_t b);
inline int popcount64(uint64_t x) {	return _mm_popcnt_u64(x);}