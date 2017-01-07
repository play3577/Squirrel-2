#include "occupied_m256.h"
//#include "occupied.h"
//#include "Bitboard.h"


using namespace std;

Occ_256 ZeroBB256;
Occ_256 ALLBB256;
Occ_256 SquareBB256[SQ_NUM];



int sq2sq_256[SQ_NUM];
int sq2sq90_256[SQ_NUM];
int sq2sq_p45_256[SQ_NUM];
int sq2sq_m45_256[SQ_NUM];

//OK
void make_sq2sq_256() {

	for (File f = File1; f < File_Num; f++) {

		for (Rank r = RankA; r < Rank_Num; r++) {

			Square sq=make_square(r, f);

			if (r == RankA || r == RankI) {sq2sq_256[sq] = Error_SQ;}
			else {

				sq2sq_256[sq] = (sq - (2 * f + 1));
			}
		}

	}

}

//OK
void make_sq2sq90_256() {

	for (File f = File1; f < File_Num; f++) {
		for (Rank r = RankA; r < Rank_Num; r++) {


			Square sq = make_square(r, f);

			if (f == File1 || f == File9) { sq2sq90_256[sq] = Error_SQ; }
			else {

				if (r == RankA) { sq2sq90_256[sq] = (f-1); }
				if (r == RankB) { sq2sq90_256[sq] = 7+(f - 1); }
				if (r == RankC) { sq2sq90_256[sq] = 14 + (f - 1); }
				if (r == RankD) { sq2sq90_256[sq] = 21 + (f - 1); }
				if (r == RankE) { sq2sq90_256[sq] = 28 + (f - 1); }
				if (r == RankF) { sq2sq90_256[sq] = 35 + (f - 1); }
				if (r == RankG) { sq2sq90_256[sq] = 42 + (f - 1); }
				if (r == RankH) { sq2sq90_256[sq] = 49 + (f - 1); }
				if (r == RankI) { sq2sq90_256[sq] = 56 + (f - 1); }
			}


		}
	}



}

void make_sq2sqp45_256() {

	for (File f = File1; f < File_Num; f++) {
		for (Rank r = RankA; r < Rank_Num; r++) {

			Square sq = make_square(r, f);

			if (f == File1 || f == File9) { sq2sq_p45_256[sq] = Error_SQ; }
			else if (r == RankA || r == RankI) { sq2sq_p45_256[sq] = Error_SQ; }
			else {
				if (sq % 10 == 0) { sq2sq_p45_256[sq] = (f - 1); }
				if (sq % 10 == 9) { sq2sq_p45_256[sq] = 7 + (f - 2); }
				if (sq % 10 == 8) { sq2sq_p45_256[sq] = 14 + (f - 3); }
				if (sq % 10 == 7) { sq2sq_p45_256[sq] = 21 + (f - 4); }
				if (sq % 10 == 6) { if (sq == 16) { sq2sq_p45_256[sq] = 13; } else { sq2sq_p45_256[sq] = 28 + (f - 5); } }
				if (sq == 15) { sq2sq_p45_256[sq] = 19; }
				if (sq == 25) { sq2sq_p45_256[sq] = 20; }
				if (sq == 55) { sq2sq_p45_256[sq] = 35; }
				if (sq == 65) { sq2sq_p45_256[sq] = 36; }
				if (sq % 10 == 4) { if (sq == 64) { sq2sq_p45_256[sq] = 42; } else { sq2sq_p45_256[sq] = 25 + (f - 1); } }
				if (sq % 10 == 3) { sq2sq_p45_256[sq] = 31 + (f - 1); }
				if (sq % 10 == 2) { sq2sq_p45_256[sq] = 37 + (f - 1); }
				if (sq % 10 == 1) { sq2sq_p45_256[sq] = 43 + (f - 1); }
			}
		}
	}
}



void make_sq2sqm45_256() {

	for (File f = File1; f < File_Num; f++) {
		for (Rank r = RankA; r < Rank_Num; r++) {

			Square sq = make_square(r, f);

			if (f == File1 || f == File9) { sq2sq_m45_256[sq] = Error_SQ; }
			else if (r == RankA || r == RankI) { sq2sq_m45_256[sq] = Error_SQ; }
			else 
			{
				if (sq % 8 == 0) { sq2sq_m45_256[sq] = (f - 1); }

				if (sq % 8 == 1) { 
					sq2sq_m45_256[sq] = 7 + (f - 2); 
				}
				if (sq % 8 == 2) {
					if (sq == 10) {sq2sq_m45_256[sq] = 13;}
					else {sq2sq_m45_256[sq] = 14 + (f - 3);}
				}
				if (sq % 8 == 3) {
					if (sq <= 19) { sq2sq_m45_256[sq] = 18 + f; }
					else { sq2sq_m45_256[sq] = 21 + (f - 4); }
				}
				if (sq % 8 == 4) { if (sq <=28) { sq2sq_m45_256[sq] = 24+f; } else { sq2sq_m45_256[sq] = 28 + (f - 5); } }
				if (sq % 8 == 5) { if (sq <= 37) { sq2sq_m45_256[sq] = 30 + f; } else { sq2sq_m45_256[sq] = 35 + (f - 6); } }
				if (sq % 8 == 6) { if (sq == 70) { sq2sq_m45_256[sq] = 42; } else { sq2sq_m45_256[sq] = 36 + (f); } }
				if (sq % 8 == 7) { sq2sq_m45_256[sq] = 42 + (f); }
			}
		}
	}
}


int occ256_shift_table_tate[SQ_NUM];
int occ256_shift_table_yoko[SQ_NUM];
int occ256_shift_table_p45[SQ_NUM];
int occ256_shift_table_m45[SQ_NUM];


/*
inline void make_shifttate() {

	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		shift_table_tate[sq]= int(1 + 9 * (sqtofile(sq) % 5));
	}
}
*/


void make_occ256_shifttate() {

	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		File f = sqtofile(sq);

		occ256_shift_table_tate[sq] = int(7 * f);

	}
}


void make_occ256_shiftyoko() {

	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		Rank r = sqtorank(sq);

		occ256_shift_table_yoko[sq] = int(7 * r);

	}
}

//外周の場合を考えていなかった！
void make_occ256_shiftp45() {

	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {


		if (sq % 10 == 0) { occ256_shift_table_p45[sq] = 0;}
		else if (sq % 10 == 9) { occ256_shift_table_p45[sq] = 7; }
		else if (sq % 10 == 8) { occ256_shift_table_p45[sq] = 14 ; }
		else if (sq % 10 == 7) { occ256_shift_table_p45[sq] = 21; }

		else if (sq % 10 == 6) { if (sq <= 26) { occ256_shift_table_p45[sq] = 13; } else { occ256_shift_table_p45[sq] = 28; } }

		else if (sq % 10 == 5) { if (sq <= 35) { occ256_shift_table_p45[sq] = 19; } else { occ256_shift_table_p45[sq] = 35; } }
		else if (sq % 10 == 4) { if (sq >=54) { occ256_shift_table_p45[sq] = 42; } else { occ256_shift_table_p45[sq] = 25; } }
		else if (sq % 10 == 3) { occ256_shift_table_p45[sq] = 31; }
		else if (sq % 10 == 2) { occ256_shift_table_p45[sq] = 37; }
		else if (sq % 10 == 1) { occ256_shift_table_p45[sq] = 43; }
	}
}


void make_occ256_shiftm45() {

	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		if (sq % 8 == 0) { occ256_shift_table_m45[sq] = 0; }
		if (sq % 8 == 1) {occ256_shift_table_m45[sq] = 7;}
		if (sq % 8 == 2) {
			if (sq <= 18) { occ256_shift_table_m45[sq] = 13; }
			else { occ256_shift_table_m45[sq] = 14 ; }
		}
		if (sq % 8 == 3) {
			if (sq <= 27) { occ256_shift_table_m45[sq] = 19; }
			else { occ256_shift_table_m45[sq] = 21; }
		}
		if (sq % 8 == 4) { if (sq <= 36) { occ256_shift_table_m45[sq] = 25; } else { occ256_shift_table_m45[sq] = 28; } }
		if (sq % 8 == 5) { if (sq <= 45) { occ256_shift_table_m45[sq] = 31; } else { occ256_shift_table_m45[sq] = 35; } }
		if (sq % 8 == 6) { if (sq >= 62) { occ256_shift_table_m45[sq] = 42; } else { occ256_shift_table_m45[sq] = 37; } }
		if (sq % 8 == 7) { occ256_shift_table_m45[sq] = 43; }
		
	}
}





//Long effectはBitboardの時に作ったものでいいと思う。

// [0] 縦 [1] 90　　[2] plus45　　[3] minus45
void init_occ256()
{
	ZeroBB256.b256 = _mm256_setzero_si256();

	make_sq2sq_256();
	make_sq2sq90_256();
	make_sq2sqp45_256();
	make_sq2sqm45_256();

	make_occ256_shifttate();
	make_occ256_shiftyoko();
	make_occ256_shiftp45();
	make_occ256_shiftm45();
	//OK
	for (Square sq = SQ1A; sq <= SQ9I; sq++) {

		//初期化
		SquareBB256[sq] = ZeroBB256;


		Square sq2 = sq - 9;

		/*SquareBB256[sq].b256.m256i_u64[0] = (sq2sq_256[sq] != Error_SQ) ? 1ULL << sq2sq_256[sq]:0;
		SquareBB256[sq].b256.m256i_u64[1] = (sq2sq90_256[sq] != Error_SQ) ? 1ULL << sq2sq90_256[sq] : 0;
		SquareBB256[sq].b256.m256i_u64[2] = (sq2sq_p45_256[sq] != Error_SQ) ? 1ULL << sq2sq_p45_256[sq] : 0;
		SquareBB256[sq].b256.m256i_u64[3] = (sq2sq_m45_256[sq] != Error_SQ) ? 1ULL << sq2sq_m45_256[sq] : 0;*/
		SquareBB256[sq].set((sq2sq_256[sq] != Error_SQ) ? 1ULL << sq2sq_256[sq] : 0,0);
		SquareBB256[sq].set((sq2sq90_256[sq] != Error_SQ) ? 1ULL << sq2sq90_256[sq] : 0,1);
		SquareBB256[sq].set((sq2sq_p45_256[sq] != Error_SQ) ? 1ULL << sq2sq_p45_256[sq] : 0,2);
		SquareBB256[sq].set((sq2sq_m45_256[sq] != Error_SQ) ? 1ULL << sq2sq_m45_256[sq] : 0,3);


		/*cout << int(sq) << endl;
		cout << SquareBB256[sq] << endl;*/
	}
	






	return;

}

/*=========================================================================
std::ostream & operator<<(std::ostream & os, const Bitboard & board)
{
for (Rank r = RankA; r < Rank_Num; r++) {
for (File f = File9; f >= File1; f--) {

Square sq = make_square(r, f);
if (sq <= 44) {
if ((board.b[0])&(1ui64 << sq)) {
os << "*";
}
else {
os << ".";
}
}
else {
if ((board.b[1])&(1ui64 << (sq-45))) {
os << "*";
}
else {
os << ".";
}

}
}
os << std::endl;
}
return os;
}
=============================================================================*/


std::ostream & operator<<(std::ostream & os, const Occ_256 & board)
{
	const uint64_t occ_tate[4] = { board.b64(0), board.b64(1),board.b64(2),board.b64(3) };


	for (int i = 0; i < 4; i++) {

		os << "occupied " << i << endl << endl;;

		uint64_t occ = occ_tate[i];
		
		for (int i = 0; i < 7; i++) {
			for (int sq = 56; sq >= 0; sq = sq - 7) {
				if (occ&(1ULL << (sq+i))) {
					/*os << sq;*/os << "*";
				}
				else { os << "."; }
			}
				os << endl;
		}
		os << endl;
	}
	return os;
}
	