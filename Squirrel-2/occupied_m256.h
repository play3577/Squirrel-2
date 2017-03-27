#pragma once

/*
 shogi686が行っていた手法、
 端っこの筋を除くと升は6*9升あるのでoccupiedは64bitに収まる
 つまり4つのoccupiedは256bitに収まる！
 https://twitter.com/merom686/status/804856797479636992
 https://twitter.com/merom686/status/804865276210647041
 https://twitter.com/merom686/status/804869491372888065
 http://d.hatena.ne.jp/LS3600/20091119
 
 AVX命令なのでウチでは使えないか....???
 http://kawa0810.hateblo.jp/entry/20120304/1330852197
 */
	

#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if defined(__GNUC__) 
#include <immintrin.h>

#endif
#include "fundation.h"

//[0]縦 [1]横 [2] plus45 [3] minus45
//__m256iを使えるのはVC++だけだったのか(´･ω･｀)
struct Occ_256 {

	__m256i b256;

	uint64_t b64(const int i)const{


#if defined(_MSC_VER)
		return b256.m256i_u64[i];
#endif
#if defined(__GNUC__) 
		return uint64_t(_mm256_extract_epi64(b256, i));
#endif
	}

	void set(const uint64_t value, const int index) {

#if defined(_MSC_VER)
		b256.m256i_u64[index]=value;
#endif
#if defined(__GNUC__) 
		//_mm256_insert_epi64(b256, value, index);
		UNREACHABLE;
#endif
		
	}
/*
Integer 256-bit vector logical operations.
			
　　　これAVX2命令ってマジ(´･ω･｀)？？
extern __m256i __cdecl _mm256_and_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_andnot_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_or_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_xor_si256(__m256i, __m256i);
*/

/*
https://twitter.com/daruma3940/status/812538092049420288
このツイートの関連ツイートを見るべし

*/

#ifdef HAVE_AVX2
Occ_256& operator^=(const Occ_256& b1) { b256 = _mm256_xor_si256(b256, b1.b256); return *this; }
Occ_256& operator|=(const Occ_256& b1) { b256 = _mm256_or_si256(b256, b1.b256); return *this; }
Occ_256& operator&=(const Occ_256& b1) { b256 = _mm256_and_si256(b256, b1.b256); return *this; }
Occ_256& andnot(const Occ_256& b1) { b256 = _mm256_andnot_si256(b256, b1.b256); return *this; }
#else
//4回 operationをしなければならないので早くならない....
//しかしattacker_toなどでoccを一つだけ渡せばいいので渡すところだけ早くなる？？
	Occ_256& operator^=(Occ_256& b1) { 

#if defined(_MSC_VER)
		b256.m256i_u64[0] = b256.m256i_u64[0] ^ b1.b256.m256i_u64[0];
		b256.m256i_u64[1] = b256.m256i_u64[1] ^ b1.b256.m256i_u64[1];
		b256.m256i_u64[2] = b256.m256i_u64[2] ^ b1.b256.m256i_u64[2];
		b256.m256i_u64[3] = b256.m256i_u64[3] ^ b1.b256.m256i_u64[3];
#endif
#if defined(__GNUC__) 
		
		b256 = _mm256_set_epi64x((b64(3) ^ b1.b64(3)), (b64(2) ^ b1.b64(2)), (b64(1) ^ b1.b64(1)), (b64(0) ^ b1.b64(0)));
#endif
	
		return *this; 
	}
	Occ_256& operator|=(const Occ_256& b1) { 

#if defined(_MSC_VER)
		b256.m256i_u64[0] = b256.m256i_u64[0] | b1.b256.m256i_u64[0];
		b256.m256i_u64[1] = b256.m256i_u64[1] | b1.b256.m256i_u64[1];
		b256.m256i_u64[2] = b256.m256i_u64[2] | b1.b256.m256i_u64[2];
		b256.m256i_u64[3] = b256.m256i_u64[3] | b1.b256.m256i_u64[3];
#endif
#if defined(__GNUC__) 
	
		b256 = _mm256_set_epi64x((b64(3) | b1.b64(3)), (b64(2) | b1.b64(2)), (b64(1) | b1.b64(1)), (b64(0) | b1.b64(0)));
#endif
	

		return *this;
	}
	Occ_256& operator&=(const Occ_256& b1){ 

#if defined(_MSC_VER)
		b256.m256i_u64[0] = b256.m256i_u64[0] & b1.b256.m256i_u64[0];
		b256.m256i_u64[1] = b256.m256i_u64[1] & b1.b256.m256i_u64[1];
		b256.m256i_u64[2] = b256.m256i_u64[2] & b1.b256.m256i_u64[2];
		b256.m256i_u64[3] = b256.m256i_u64[3] & b1.b256.m256i_u64[3];
#endif
#if defined(__GNUC__) 
	
		b256 = _mm256_set_epi64x((b64(3) & b1.b64(3)), (b64(2) & b1.b64(2)), (b64(1) & b1.b64(1)), (b64(0) & b1.b64(0)));
#endif
	

		return *this; 
	}
//Occ_256& andnot(const Occ_256& b1) {
//	
//	b256.m256i_u64[0] = b256.m256i_u64[0] &~ b1.b256.m256i_u64[0];
//	b256.m256i_u64[1] = b256.m256i_u64[1] &~ b1.b256.m256i_u64[1];
//	b256.m256i_u64[2] = b256.m256i_u64[2] &~ b1.b256.m256i_u64[2];
//	b256.m256i_u64[3] = b256.m256i_u64[3] &~ b1.b256.m256i_u64[3];
//	return *this; 
//}
#endif
};

inline Occ_256& operator^(const Occ_256& b1,const Occ_256& b2) {
	static Occ_256 occ_;//もっといい解決策はないか？？

#if defined(_MSC_VER)
	
	occ_.b256.m256i_u64[0] = b2.b256.m256i_u64[0] ^ b1.b256.m256i_u64[0];
	occ_.b256.m256i_u64[1] = b2.b256.m256i_u64[1] ^ b1.b256.m256i_u64[1];
	occ_.b256.m256i_u64[2] = b2.b256.m256i_u64[2] ^ b1.b256.m256i_u64[2];
	occ_.b256.m256i_u64[3] = b2.b256.m256i_u64[3] ^ b1.b256.m256i_u64[3];
#endif
#if defined(__GNUC__) 
	
	occ_.b256 = _mm256_set_epi64x((b2.b64(3) ^ b1.b64(3)), (b2.b64(2) ^ b1.b64(2)), (b2.b64(1) ^ b1.b64(1)), (b2.b64(0) ^ b1.b64(0)));
#endif
	
	return occ_;
	
}


inline Occ_256& operator|(const Occ_256& b1, const Occ_256& b2) {
	static Occ_256 occ_;//もっといい解決策はないか？？

#if defined(_MSC_VER)

	occ_.b256.m256i_u64[0] = b2.b256.m256i_u64[0] | b1.b256.m256i_u64[0];
	occ_.b256.m256i_u64[1] = b2.b256.m256i_u64[1] | b1.b256.m256i_u64[1];
	occ_.b256.m256i_u64[2] = b2.b256.m256i_u64[2] | b1.b256.m256i_u64[2];
	occ_.b256.m256i_u64[3] = b2.b256.m256i_u64[3] | b1.b256.m256i_u64[3];
#endif
#if defined(__GNUC__) 

	occ_.b256 = _mm256_set_epi64x((b2.b64(3) | b1.b64(3)), (b2.b64(2) | b1.b64(2)), (b2.b64(1) | b1.b64(1)), (b2.b64(0) | b1.b64(0)));
#endif

	return occ_;

}

std::ostream& operator<<(std::ostream& os, const Occ_256& board);


extern Occ_256 ZeroBB256;
extern Occ_256 ALLBB256;
extern Occ_256 SquareBB256[SQ_NUM];


extern int occ256_shift_table_tate[SQ_NUM];
extern int occ256_shift_table_yoko[SQ_NUM];
extern int occ256_shift_table_p45[SQ_NUM];
extern int occ256_shift_table_m45[SQ_NUM];




void init_occ256();


