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

//[0]�c [1]�� [2] plus45 [3] minus45
struct Occ_256 {
__m256i b256;
uint64_t b64(const int i) const {
	return b256.m256i_u64[i];
	
}

/*
Integer 256-bit vector logical operations.
			
�@�@�@����AVX2���߂��ă}�W(�L��֥�M)�H�H
extern __m256i __cdecl _mm256_and_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_andnot_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_or_si256(__m256i, __m256i);
extern __m256i __cdecl _mm256_xor_si256(__m256i, __m256i);
*/

/*
https://twitter.com/daruma3940/status/812538092049420288
���̃c�C�[�g�̊֘A�c�C�[�g������ׂ�

*/

#ifdef HAVE_AVX2
Occ_256& operator^=(const Occ_256& b1) { b256 = _mm256_xor_si256(b256, b1.b256); return *this; }
Occ_256& operator|=(const Occ_256& b1) { b256 = _mm256_or_si256(b256, b1.b256); return *this; }
Occ_256& operator&=(const Occ_256& b1) { b256 = _mm256_and_si256(b256, b1.b256); return *this; }
Occ_256& andnot(const Occ_256& b1) { b256 = _mm256_andnot_si256(b256, b1.b256); return *this; }
#else
//4�� operation�����Ȃ���΂Ȃ�Ȃ��̂ő����Ȃ�Ȃ�....
//������attacker_to�Ȃǂ�occ��������n���΂����̂œn���Ƃ��낾�������Ȃ�H�H
Occ_256& operator^=(const Occ_256& b1) { 
	b256.m256i_i64[0] = b256.m256i_i64[0] ^ b1.b256.m256i_i64[0];  
	b256.m256i_i64[1] = b256.m256i_i64[1] ^ b1.b256.m256i_i64[1];
	b256.m256i_i64[2] = b256.m256i_i64[2] ^ b1.b256.m256i_i64[2];
	b256.m256i_i64[3] = b256.m256i_i64[3] ^ b1.b256.m256i_i64[3];
	return *this; 
}
Occ_256& operator|=(const Occ_256& b1) { 
	b256.m256i_i64[0] = b256.m256i_i64[0] | b1.b256.m256i_i64[0];
	b256.m256i_i64[1] = b256.m256i_i64[1] | b1.b256.m256i_i64[1];
	b256.m256i_i64[2] = b256.m256i_i64[2] | b1.b256.m256i_i64[2];
	b256.m256i_i64[3] = b256.m256i_i64[3] | b1.b256.m256i_i64[3];

	return *this;
}
Occ_256& operator&=(const Occ_256& b1) { 
	b256.m256i_i64[0] = b256.m256i_i64[0] & b1.b256.m256i_i64[0];
	b256.m256i_i64[1] = b256.m256i_i64[1] & b1.b256.m256i_i64[1];
	b256.m256i_i64[2] = b256.m256i_i64[2] & b1.b256.m256i_i64[2];
	b256.m256i_i64[3] = b256.m256i_i64[3] & b1.b256.m256i_i64[3];

	return *this; 
}
//Occ_256& andnot(const Occ_256& b1) {
//	
//	b256.m256i_i64[0] = b256.m256i_i64[0] &~ b1.b256.m256i_i64[0];
//	b256.m256i_i64[1] = b256.m256i_i64[1] &~ b1.b256.m256i_i64[1];
//	b256.m256i_i64[2] = b256.m256i_i64[2] &~ b1.b256.m256i_i64[2];
//	b256.m256i_i64[3] = b256.m256i_i64[3] &~ b1.b256.m256i_i64[3];
//	return *this; 
//}
#endif
};

inline Occ_256& operator^(const Occ_256& b1,const Occ_256& b2) {
	Occ_256 occ_;

	occ_.b256.m256i_i64[0] = b2.b256.m256i_i64[0] ^ b1.b256.m256i_i64[0];
	occ_.b256.m256i_i64[1] = b2.b256.m256i_i64[1] ^ b1.b256.m256i_i64[1];
	occ_.b256.m256i_i64[2] = b2.b256.m256i_i64[2] ^ b1.b256.m256i_i64[2];
	occ_.b256.m256i_i64[3] = b2.b256.m256i_i64[3] ^ b1.b256.m256i_i64[3];
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


