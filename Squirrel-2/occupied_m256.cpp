#include "occupied_m256.h"
#include "occupied.h"
#include "Bitboard.h"


using namespace std;

 Occ_256 ZeroBB256;
 Occ_256 ALLBB256;
 Occ_256 SquareBB256[SQ_NUM];

 // [0] ïÅí [1] 90[2] plus45[3] minus45
void init_occ256()
{
	ZeroBB256.b256 = _mm256_setzero_si256();


	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		if (sqtofile(sq)==File1||sqtofile(sq)==File9) { continue; }

		SquareBB256[sq].b256.m256i_u64[0];
		SquareBB256[sq].b256.m256i_u64[1];
		SquareBB256[sq].b256.m256i_u64[2];
		SquareBB256[sq].b256.m256i_u64[3];
		cout << sq << endl;
	}


}
