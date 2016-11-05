#include "Bitboard.h"
#include "bitop.h"
#include "occupied.h"

#include <bitset>
#include <algorithm>
using namespace std;



Bitboard SquareBB[SQ_NUM];//OK
Bitboard FileBB[File_Num];//OK
Bitboard RankBB[Rank_Num];//OK
Bitboard ZeroBB, ALLBB;//OK
Bitboard canPromoteBB[ColorALL];//OK

Bitboard StepEffect[ColorALL][PT_ALL][SQ_NUM];//OK

//127�E�E7bit���ׂĂP�̏ꍇ 
//��Ԃ̌����Ɗp�̗����͑O��Ώ�
//�������ꍇ��longeffect�ɉ��̋@��𑫂��΂����B
/*
roteted bitboard���g���Ă���bonanza�͗����͏c���ʂŎ����Ă����̂ł���ɏK��
*/
Bitboard LongRookEffect_yoko[SQ_NUM][128];//OK
Bitboard LongRookEffect_tate[SQ_NUM][128];//OK(���Ԃ��R�����g���R�������Ԃł��g�����߂ɂ�infrontBB��p�ӂ��Ȃ���΂Ȃ�Ȃ�)
Bitboard LongBishopEffect_plus45[SQ_NUM][128];//OK
Bitboard LongBishopEffect_minus45[SQ_NUM][128];//OK

//Rank�ł͂Ȃ���square�Ŏ��Ƃ����Ǝv�������ǃ������̐ߖ�ƁA�������烉���N�ɕϊ�����̊ȒP������rank�Ŏ����Ƃɂ���B
Bitboard InFront_BB[ColorALL][Rank_Num];//OK



Square Bitboard::pop()
{
	return (b[0] != 0) ? Square(pop_lsb(b[0])) : Square(pop_lsb(b[1]));
}

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


int additional_plus45(Square sq) {

	File f = sqtofile(sq);
	Rank r = sqtorank(sq);

	return std::max(int(f - r), 0);

}

void bitboard_init()
{
	ZeroBB = Bitboard(0, 0);


	//SquareBB���̂ɂ�����ƒ��ӂ��K�v�Ȃ�X�˂�(�L�E�ցE�M)�Ƃ肠����1ui64���g���΂����炵���B���Ԃ񑼂̃R���p�C���ł͎g���Ȃ��悤�ȋC������B
	/*�܂���squareBB�̍쐬�B
	����𗘗p����Rank,file,Effect,Bitween������Ă���
	*/
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		//31bit���ꂽ�Ƃ���ɂ������bit�������Ă��܂��Ă���I�I�I�I�I�I�I�i���������j
		if (sq <= SQ5I) {
			SquareBB[sq] = Bitboard(uint64_t(1ui64 << sq), 0);
		}
		else {
			SquareBB[sq] = Bitboard(0, uint64_t(1ui64 << (sq - SQ6A)));
		}
		ALLBB ^= SquareBB[sq];
	}
	//rankBB�̍쐬
	for (Square sq = SQ1A; sq < SQ9I; sq = sq + 9) {
		RankBB[RankA] ^= SquareBB[sq];
		RankBB[RankB] ^= SquareBB[sq + 1];
		RankBB[RankC] ^= SquareBB[sq + 2];
		RankBB[RankD] ^= SquareBB[sq + 3];
		RankBB[RankE] ^= SquareBB[sq + 4];
		RankBB[RankF] ^= SquareBB[sq + 5];
		RankBB[RankG] ^= SquareBB[sq + 6];
		RankBB[RankH] ^= SquareBB[sq + 7];
		RankBB[RankI] ^= SquareBB[sq + 8];
	}
	//FileBB�̍쐬
	for (Square sq = SQ1A; sq <= SQ1I; sq = sq + 1) {
		FileBB[File1] ^= SquareBB[sq];
		FileBB[File2] ^= SquareBB[sq + 9];
		FileBB[File3] ^= SquareBB[sq + 18];
		FileBB[File4] ^= SquareBB[sq + 27];
		FileBB[File5] ^= SquareBB[sq + 36];
		FileBB[File6] ^= SquareBB[sq + 45];
		FileBB[File7] ^= SquareBB[sq + 54];
		FileBB[File8] ^= SquareBB[sq + 63];
		FileBB[File9] ^= SquareBB[sq + 72];
	}

	//infrontofBB
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (Rank rr = RankA; rr < r; rr++) {
			InFront_BB[BLACK][r] ^= RankBB[rr];
		}
	}
	for (Rank r = RankI; r >= RankA; r--) {
		for (Rank rr = RankI; rr > r; rr--) {
			InFront_BB[WHITE][r] ^= RankBB[rr];
		}
	}



	//canpromote
	canPromoteBB[BLACK] = RankBB[RankA] ^ RankBB[RankB] ^ RankBB[RankC];
	canPromoteBB[WHITE] = RankBB[RankG] ^ RankBB[RankH] ^ RankBB[RankI];

	//====================
	//�ߐڋ�̗�����bitboard
	//====================
	/*
	B_PAWN=1, B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING,
	B_PRO_PAWN, B_PRO_LANCE, B_PRO_NIGHT, B_PRO_SILVER, B_UNICORN, B_DRAGON,
	W_PAWN=17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING,
	W_PRO_PAWN, W_PRO_LANCE, W_PRO_NIGHT, W_PRO_SILVER, W_UNICORN, W_DRAGON,PC_ALL,
	*/
	//12���������P�̋�͂W�����܂łɂ��������Ȃ����ߔz��̃T�C�Y�͂W�ł����Ǝv��
	//���Ԃ��猩�������x�N�g�����g���B
	//��Ԋp��stepeffect�����Ă����傤���Ȃ��񂾂��ǂȂ�(�L�E�ցE�M)
	//idea from shogi686
	const int stepdirec[PT_ALL][8] = {

		{0,0,0,0,0,0,0,0},
		{-1,0,0,0,0,0,0,0},//��
		{-1,0,0,0,0,0,0,0},//����
		{-11,7,0,0,0,0,0,0},//�j�n
		{8,-1,-10,-8,10,0,0,0},//��
		{-10,-8,8,10,0,0,0,0},//�p
		{-1,1,9,-9,0,0,0,0},//���
		{9,8,-1,-10,-9,1,0,0},//��(���ꂪ�Ȃ񂩂�������)�i�����ς݁j
		{-1,-10,-9,-8,1,10,9,8},//��
		{ 9,8,-1,-10,-9,1,0,0 },//��
		{ 9,8,-1,-10,-9,1,0,0 },//����
		{ 9,8,-1,-10,-9,1,0,0 },//���j
		{ 9,8,-1,-10,-9,1,0,0 },//����
		{ -1,-10,-9,-8,1,10,9,8 },//�n
		{ -1,-10,-9,-8,1,10,9,8 },//���

	};

	//StepEffect
	Square to;
	for (Color c = BLACK; c <= WHITE; c++) {

		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

				Bitboard b = ZeroBB;

				for (int direc = 0; direc < 8; direc++) {
					c == BLACK ? to = sq + stepdirec[pt][direc] : to = sq - stepdirec[pt][direc];
					//idea from mermo 686
					//��ь����ȊO�ŏc�ɂR���ȏ㓮����͂Ȃ�
					if ((to != sq) && is_ok(to) && std::abs(int(sqtorank(sq) - int(sqtorank(to)))) <= 2) {
						b ^= SquareBB[to];
					}
				}
				StepEffect[c][pt][sq] = b;
			}
		}
	}
	//LongEffect
	/*
	long effect�͂�����roteted bitboard�������Ă���������������������..
	�e�[�u���̗p�ӂ����Ȃ獡�ł���̂ł��܂��Ă�������...
	*/

	/*
	�c�Ɖ��𕪂��Ċi�[����̂��H�H
	*/

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {
			int obstacle2 = obstacle<<1;//��V�t�g���Ă��Ȃ��Ƃ����Ȃ��I�I�I
			int direc_rook_yoko[2] = { 9,-9 };
			
			File tofile;
			for (int i = 0; i < 2; i++) {
				to = sq;
				do {
					to += Square(direc_rook_yoko[i]);
					tofile = sqtofile(to);
					if (is_ok(to)&&(to != sq)) {
						LongRookEffect_yoko[sq][obstacle] ^= SquareBB[to];
					}
					//����c�̊֌W�ł���΂��̕��@�͎g���邯�ǎ΂߂ł͒ʗp���Ȃ���...
				} while ((!(obstacle2&(1 << tofile))) && is_ok(to));
			}

		}
	}
	//�c�B���[����肭�����Ă��Ȃ��Ȃ����낤�H�����ς�
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {
			int direc_rook_yoko[2] = { 1,-1 };
			int obstacle2 = obstacle<<1;//��V�t�g���Ă��Ȃ��Ƃ����Ȃ��I�I�I
			Rank torank;
			File tofile;
			for (int i = 0; i < 2; i++) {
				to = sq;
				do {
					to += Square(direc_rook_yoko[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq) && (tofile == sqfile)) {
						LongRookEffect_tate[sq][obstacle] ^= SquareBB[to];
					}
					//����c�̊֌W�ł���΂��̕��@�͎g���邯�ǎ΂߂ł͒ʗp���Ȃ���...
				} while ((!(obstacle2&(1 << torank))) && is_ok(to)&&(tofile==sqfile));//�c�̏ꍇ��file���������ǂ��������ׂȂ���΂Ȃ�Ȃ��B
			}
		}
	}



	/*
	�悭�l����Ǝ΂߂�rank�Ɏˉe���Ă��܂��Ή��Ɠ����悤�ɏ����ł��邩(�L�E�ցE�M)
	���̂ق����������ȒP�Ȃ̂ŉ��Ōv�Z����B
	*/
	//�΂߃v���X�S�T�x
	//���̕��@�ł͏ォ��p�̌������L�тĂ��Ă��܂��Ƃ������Ƃ��N���肤��I�I(�����ς�)

	/*
	���̕��@�Ńe�[�u�����̂͂܂����I�I�I�l�������I�I�I�I�I
	*/
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			int direc_bishop_p45[2] = { -10, + 10 };
			int obstacle2 = obstacle << (1+additional_plus45(sq));//1bit�V�t�g������B

			Rank torank;
			File tofile;//�������ւ̎ˉe�B
			Square oldto;
			Rank oldrank;
			for (int i = 0; i < 2; i++) {
				to = sq;
				//tofile = sqtofile(to);
				do {
					/*
					oldto��ێ�����oldto��newto��rank���Q�ȏ㗣��Ȃ��悤�ɂ���B
					*/
					oldto = to;
					oldrank = sqtorank(oldto);
					to += Square(direc_bishop_p45[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq)&&(abs(torank-oldrank)<2)) {
						LongBishopEffect_plus45[sq][obstacle] ^= SquareBB[to];
					}
				} while (!(obstacle2&(1 << (tofile))) && is_ok(to) && (abs(torank - oldrank)<2));
			}
		}
	}
	//�΂�-45�x
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			int direc_bishop_m45[2] = { -8, +8 };
			int obstacle2 = obstacle << 1;//1bit�V�t�g������B

			Rank torank;
			File tofile;//�������ւ̎ˉe�B
			Square oldto;
			Rank oldrank;
			for (int i = 0; i < 2; i++) {
				to = sq;
				//tofile = sqtofile(to);
				do {
					/*
					oldto��ێ�����oldto��newto��rank���Q�ȏ㗣��Ȃ��悤�ɂ���B
					*/
					oldto = to;
					oldrank = sqtorank(oldto);
					to += Square(direc_bishop_m45[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq) && (abs(torank - oldrank)<2)) {
						LongBishopEffect_minus45[sq][obstacle] ^= SquareBB[to];
					}
				} while ((!(obstacle2&(1 << (tofile)))) && is_ok(to) && (abs(torank - oldrank)<2));
			}
		}
	}

	//index�̍쐬
	make_indexplus45();
	make_shiftplus45();

	bitboard_debug();
}


void bitboard_debug()
{
	/*cout << ZeroBB << endl;*/
	//cout << ALLBB << endl;
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout << "Square " << sq << endl;
		cout << SquareBB[sq] << endl;

	}*/
	/*for (Rank r = RankA; r < Rank_Num; r++) {
		cout << RankBB[r] << endl;
	}
	for (File f = File1; f < File_Num; f++) {
		cout << FileBB[f] << endl;
	}*/
	/*for (Color c = BLACK; c <= WHITE; c++) {
		cout << canPromoteBB[c] << endl;
	}*/
/*
	for (Color c = BLACK; c < ColorALL; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
				std::cout << "color " << c << " psq " << sq << " pt " << pt << std::endl;
				std::cout << StepEffect[c][pt][sq] << std::endl << std::endl;
			}
		}
	}
*/
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "ROOK sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongRookEffect_yoko[sq][obstacle] << endl;

		}
	}*/
	//
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "ROOK sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongRookEffect_tate[sq][obstacle] << endl;

		}
	}*/
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "BIshop sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongBishopEffect_plus45[sq][obstacle] << endl;

		}
	}*/
	/*for (Square sq = SQ9I; sq >= SQ1A; sq--) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "BISHOP sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongBishopEffect_minus45[sq][obstacle] << endl;

		}
	}*/

	/*for (Color c = BLACK; c <= WHITE; c++) {

		for (Rank r = RankA; r < Rank_Num; r++) {
			cout << " color " << c << " rank " << r << endl;
			cout << InFront_BB[c][r] << endl;
		}
	}*/

	//�e�[�u���������ƍ��Ă��Ȃ�����
	cout << LongBishopEffect_plus45[18][(0b1100010)] << endl;

}
