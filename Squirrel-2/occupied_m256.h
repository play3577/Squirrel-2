#pragma once

/*
shogi686���s���Ă�����@�A
�[�����̋؂������Ə���6*9������̂�occupied��64bit�Ɏ��܂�
�܂�4��occupied��256bit�Ɏ��܂�I
https://twitter.com/merom686/status/804856797479636992
https://twitter.com/merom686/status/804865276210647041
https://twitter.com/merom686/status/804869491372888065
http://d.hatena.ne.jp/LS3600/20091119

AVX���߂Ȃ̂ŃE�`�ł͎g���Ȃ���....???
http://kawa0810.hateblo.jp/entry/20120304/1330852197
*/

#include <intrin.h>
#include "fundation.h"

//[0] ���� [1] 90 [2] plus45 [3] minus45
struct Occ_256 {
	__m256i b256;
	uint64_t b64(const int i) {
		return b256.m256i_u64[i];
	}

	/*
	Integer 256-bit vector logical operations.

	extern __m256i __cdecl _mm256_and_si256(__m256i, __m256i);
	extern __m256i __cdecl _mm256_andnot_si256(__m256i, __m256i);
	extern __m256i __cdecl _mm256_or_si256(__m256i, __m256i);
	extern __m256i __cdecl _mm256_xor_si256(__m256i, __m256i);
	*/

	Occ_256& operator^=(const Occ_256& b1) { b256 = _mm256_xor_si256(b256, b1.b256); return *this; }
	Occ_256& operator|=(const Occ_256& b1) { b256 = _mm256_or_si256(b256, b1.b256); return *this; }
	Occ_256& operator&=(const Occ_256& b1) { b256 = _mm256_and_si256(b256, b1.b256); return *this; }
	Occ_256& andnot(const Occ_256& b1) { b256 = _mm256_andnot_si256(b256, b1.b256); return *this; }
};



extern Occ_256 ZeroBB256;
extern Occ_256 ALLBB256;
extern Occ_256 SquareBB256[SQ_NUM];


void init_occ256();