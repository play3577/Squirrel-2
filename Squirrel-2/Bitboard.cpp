#include "Bitboard.h"
#include "bitop.h"
#include "occupied.h"
#include "position.h"

#include <bitset>
#include <algorithm>
using namespace std;



Bitboard SquareBB[SQ_NUM];//OK
Bitboard FileBB[File_Num];//OK
Bitboard RankBB[Rank_Num];//OK
Bitboard ZeroBB, ALLBB;//OK
Bitboard canPromoteBB[ColorALL];//OK

Bitboard StepEffect[ColorALL][PT_ALL][SQ_NUM];//OK

//SQ1��SQ2�̊Ԃ�1�ɂȂ��Ă���bitboard
//�ԂƂ����̂�(SQ1,SQ2) �܂�J��ԂƂ���
Bitboard BetweenBB[SQ_NUM][SQ_NUM];

//127�E�E7bit���ׂĂP�̏ꍇ 
//��Ԃ̌����Ɗp�̗����͑O��Ώ�
//�������ꍇ��longeffect�ɉ��̋@��𑫂��΂����B
/*
roteted bitboard���g���Ă���bonanza�͗����͏c���ʂŎ����Ă����̂ł���ɏK��
*/
Bitboard LongRookEffect_yoko[SQ_NUM][128];//OK
Bitboard LongRookEffect_tate[SQ_NUM][128];//OK
Bitboard LongBishopEffect_plus45[SQ_NUM][128];//OK
Bitboard LongBishopEffect_minus45[SQ_NUM][128];//OK

Bitboard LanceEffect[ColorALL][SQ_NUM][128];//���Ԃ̗��������߂�̂ɖ���tate&Infront������͖̂��ʂȂ̂Ńe�[�u���Ƃ��ĕێ����Ă����B

//Rank�ł͂Ȃ���square�Ŏ��Ƃ����Ǝv�������ǃ������̐ߖ�ƁA�������烉���N�ɕϊ�����̊ȒP������rank�Ŏ����Ƃɂ���B
Bitboard InFront_BB[ColorALL][Rank_Num];//OK

//��ړ��ł��Ȃ��̈悪�P�ɂȂ���bitboard
Bitboard CantGo_PAWNLANCE[ColorALL];
Bitboard CantGo_KNIGHT[ColorALL];


//directtable
//[from][to]
//from���猩��to�̈ʒu�֌W�B
Direction direct_table[SQ_NUM][SQ_NUM];


Bitboard GivesCheckStepBB[ColorALL][PT_ALL][SQ_NUM];
Bitboard GivesCheckRookBB[ColorALL][SQ_NUM][128];
Bitboard GivesCheckBishopBB[ColorALL][SQ_NUM][128];
Bitboard GivesCheckLanceBB[ColorALL][SQ_NUM][128];


//pin�S�}�̈ʒu�e�[�u�����쐬���邽�߂Ɏg��occupied���l�����Ȃ��Ƃь����e�[�u���B
Bitboard RookPsuedoAttack[SQ_NUM];//OK
Bitboard BishopPsuedoAttack[SQ_NUM];//OK
Bitboard LancePsuedoAttack[ColorALL][SQ_NUM];//OK

void checkbb();

Square Bitboard::pop()
{
	return (b[0] != 0) ? Square(pop_lsb(b[0])) : Square(pop_lsb(b[1])+45);
}

Square Bitboard::pop_fromb0()
{
	ASSERT(b[0] != 0);
	return Square(pop_lsb(b[0]));
}

Square Bitboard::pop_fromb1()
{
	ASSERT(b[1] != 0);
	return Square(pop_lsb(b[1]) + 45);
}

std::ostream & operator<<(std::ostream & os, const Bitboard & board)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {

			Square sq = make_square(r, f);
			if (sq <= 44) {
				if ((board.b[0])&(1ULL << sq)) {
					os << "*";
				}
				else {
					os << ".";
				}
			}
			else {
				if ((board.b[1])&(1ULL << (sq-45))) {
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

/*
bit����ʉ��ʓ���ւ��邽�߂̊֐�
�����ł͓����Ă���bit��7bit�ł���Ɖ��肷��B
*/
int change_indian(int i) {

	//i��7bit�ȉ��łȂ���΂Ȃ�Ȃ�
	ASSERT((i >> 8) == 0);

	int ret = 0;
	for (int b = 0; b < 7; b++) {
		if (i&(1 << b)) {
			ret |= 1 << (6 - b);
		}
	}

	return ret;
}


/*
�V�t�g�O�p�s��
*/

int additional_plus45(Square sq) {

	File f = sqtofile(sq);
	Rank r = sqtorank(sq);

	return std::max(int(f - r), 0);

}

int additional_minus45(Square sq) {

	File f = sqtofile(sq);
	Rank r = sqtorank(sq);

	return std::max(int(f+r-8), 0);

}

void bitboard_init()
{
	ZeroBB = Bitboard(0, 0);
	ALLBB = ZeroBB;

	//SquareBB���̂ɂ�����ƒ��ӂ��K�v�Ȃ�X�˂�(�L�E�ցE�M)�Ƃ肠����1ui64���g���΂����炵���B���Ԃ񑼂̃R���p�C���ł͎g���Ȃ��悤�ȋC������B
	/*�܂���squareBB�̍쐬�B
	����𗘗p����Rank,file,Effect,Bitween������Ă���
	*/
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		//31bit���ꂽ�Ƃ���ɂ������bit�������Ă��܂��Ă���I�I�I�I�I�I�I�i���������j
		if (sq <= SQ5I) {
			SquareBB[sq] = Bitboard(uint64_t(1ULL << sq), 0);
		}
		else {
			SquareBB[sq] = Bitboard(0, uint64_t(1ULL << (sq - SQ6A)));
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

	//�j�n�ƍ��Ԃƕ��̂Ȃ炸�ňړ��ł��Ȃ������N��bitboard
	CantGo_PAWNLANCE[BLACK] = RankBB[RankA];
	CantGo_PAWNLANCE[WHITE] = RankBB[RankI];

	CantGo_KNIGHT[BLACK] = RankBB[RankA] ^ RankBB[RankB];
	CantGo_KNIGHT[WHITE] = RankBB[RankI] ^ RankBB[RankH];

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
	//���Ԃ̌����e�[�u���̍쐬
	for (Color c = BLACK; c < ColorALL; c++) {
		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
			for (int obstacle = 0; obstacle < 128; obstacle++) {
				LanceEffect[c][sq][obstacle] = (LongRookEffect_tate[sq][obstacle] & InFront_BB[c][sqtorank(sq)]);
			}
		}
	}


	/*
	�悭�l����Ǝ΂߂�rank�Ɏˉe���Ă��܂��Ή��Ɠ����悤�ɏ����ł��邩(�L�E�ցE�M)
	���̂ق����������ȒP�Ȃ̂ŉ��Ōv�Z����B
	*/
	//�΂߃v���X�S�T�x
	//���̕��@�ł͏ォ��p�̌������L�тĂ��Ă��܂��Ƃ������Ƃ��N���肤��I�I(�����ς�)
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		/*File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);*/
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
		/*File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);*/
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			//int obstacle_ = change_indian(obstacle);
			int direc_bishop_m45[2] = { -8, +8 };
			int obstacle2 = obstacle << (1+additional_minus45(sq));//

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
						//obstacle��bit��1��7��7��1�ɂƂ����悤�ɓ���ւ��Ȃ���΂Ȃ�Ȃ��B
						LongBishopEffect_minus45[sq][(obstacle)] ^= SquareBB[to];
					}
				} while ((!( obstacle2&(1 << (tofile)))) && is_ok(to) && (abs(torank - oldrank)<2));
			}
		}
	}

	//index�̍쐬
	make_shifttate();
	make_indextate();
	make_shiftyoko();
	make_indexyoko();
	make_indexplus45();
	make_shiftplus45();
	//indexminus45�͒��ڒl�����ď���������Ă���̂ő��v
	make_shiftMinus45();

	//===================
	//direction table�̍쐬�ibitboard�ł͂Ȃ�����ǂ����ō쐬���Ă��܂��j
	//===================
	for (Square from = SQ_ZERO; from < SQ_NUM; from++) {

		Direction d;
		for (int i = 0; i < Direct_NUM; i++) {

			Square to,oldto;
			d = direct[i];
			to = from;
			do {
				oldto = to;
				to += Square(d);

				if (is_ok(to) &&to!=from&& abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2) {
					direct_table[from][to] = d;
				}

			} while (is_ok(to) && abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2);
		}
	}


	//============================bitweenBB�̍쐬
	//��ō쐬����directiontable��p����ΊȒP�ɍ쐬�ł���ƍl������
	for (Square from = SQ_ZERO; from < SQ_NUM; from++) {
		for (Square to = SQ_ZERO; to < SQ_NUM; to++) {

			Direction d = direct_table[from][to];

			if (d != Direction(0)) {

				Square beteen = from + Square(d);

				//rank������Ă邩�ǂ����Ƃ��m�F���ĂȂ�����direction_table�ł��̂�����͊m�F�ς݂ł��邽�ߑ��v���ƍl������B
				while(beteen!=to){
				
					BetweenBB[from][to] |= SquareBB[beteen];

					beteen += Square(d);
				}
			}
		}
	}


	//-----------------------------psuedo attack

	//��
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		int direc_rook_yoko[2] = { 9,-9 };
		Square oldto;
		File tofile;
		for (int i = 0; i < 2; i++) {
			to = sq;
			oldto = sq;
			do {
				oldto = to;
				to += Square(direc_rook_yoko[i]);
				//tofile = sqtofile(to);
				if (is_ok(to) && (to != sq)&& abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2) {
					RookPsuedoAttack[sq] ^= SquareBB[to];
				}
				//����c�̊֌W�ł���΂��̕��@�͎g���邯�ǎ΂߂ł͒ʗp���Ȃ���...
			} while (abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2 && is_ok(to));
		}	
	}
	//�c
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		int direc_rook_yoko[2] = { 1,-1 };
		Square oldto;
		File tofile;
		for (int i = 0; i < 2; i++) {
			to = sq;
			oldto = sq;
			do {
				oldto = to;
				to += Square(direc_rook_yoko[i]);
				//tofile = sqtofile(to);
				if (is_ok(to) && (to != sq) && abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2) {
					RookPsuedoAttack[sq] ^= SquareBB[to];
				}
				//����c�̊֌W�ł���΂��̕��@�͎g���邯�ǎ΂߂ł͒ʗp���Ȃ���...
			} while (abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2 && is_ok(to));
		}
	}
/*
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout<<sq<<endl<<RookPsuedoAttack[sq]<<endl;
	}
*/
	//�΂߃v���X�S�T�x
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		/*File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);*/
		
		int direc_bishop_p45[2] = { -10, +10 };

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
				if (is_ok(to) && (to != sq) && (abs(torank - oldrank)<2)) {
					BishopPsuedoAttack[sq] ^= SquareBB[to];
				}
			} while (is_ok(to) && (abs(torank - oldrank)<2));
		}
	}

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		/*File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);*/

		int direc_bishop_p45[2] = { -8, +8 };

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
				if (is_ok(to) && (to != sq) && (abs(torank - oldrank)<2)) {
					BishopPsuedoAttack[sq] ^= SquareBB[to];
				}
			} while (is_ok(to) && (abs(torank - oldrank)<2));
		}
	}

	//���Ԃ̃e�[�u���̍쐬
	for (Color c = BLACK; c < ColorALL; c++) {
		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
				LancePsuedoAttack[c][sq] = (RookPsuedoAttack[sq] & InFront_BB[c][sqtorank(sq)]);
				
		}
	}

	/*for (Color c = BLACK; c < ColorALL; c++) {
		for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

			cout << sq << endl << LancePsuedoAttack[c][sq] << endl;
			cout <<"more than one"<< more_than_one(LancePsuedoAttack[c][sq]) << endl;
		}
	}*/


	//check_between();
	//check_directtable();
	//bitboard_debug();
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
	}
	
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
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
	//cout << LongBishopEffect_minus45[39][(0b0100010)] << endl;

}

//��ї����łȂ��ꍇ��pos��n���͖̂��ʂɂȂ��Ă��܂��̂Ŕ�ї����p�Ɣ�ї����łȂ��p�Ŋ֐��𕪂��������������H
Bitboard effectBB(const Position &pos,const Piece pt, const Color c, const Square sq) {

	//Bitboard effect;

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;


	switch (pt)
	{
	case PAWN:
		return StepEffect[c][pt][sq];
		break;
	case KNIGHT:
		return StepEffect[c][pt][sq];
		break;
	case SILVER:
		return StepEffect[c][pt][sq];
		break;
	case GOLD:case PRO_PAWN:case PRO_LANCE:case PRO_NIGHT:case PRO_SILVER:
		return StepEffect[c][GOLD][sq];
		break;
	case KING:
		return StepEffect[c][KING][sq];
		break;
	case LANCE:
		obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		return LanceEffect[c][sq][obstacle_tate];
		break;
	case BISHOP:
		obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45];
		break;
	case ROOK:
		obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case UNICORN:
		obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45] | StepEffect[BLACK][KING][sq];
		break;
	case DRAGON:
		obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[BLACK][KING][sq];
		break;
	default:
		cout << pos << endl;
		ASSERT(0);
		return ALLBB;
		break;
	}

}

//�ߐڗ����p�ŕ����Ă݂��B
Bitboard step_effect(const Color c, const Piece pt, const Square sq) {

	return StepEffect[c][pt][sq];
	
}

//
//
////��ї����łȂ��ꍇ��pos��n���͖̂��ʂɂȂ��Ă��܂��̂Ŕ�ї����p�Ɣ�ї����łȂ��p�Ŋ֐��𕪂��������������H
//Bitboard long_effect(const Position &pos, const Color c, const Piece pt, const Square sq) {
//
//	//Bitboard effect;
//
//	uint8_t obstacle_tate;
//	uint8_t obstacle_yoko;
//	uint8_t obstacle_plus45;
//	uint8_t obstacle_Minus45;
//
//	switch (pt)
//	{
//	case LANCE:
//		obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������i�ŏ\�����H�H�H�H�j
//		return  LanceEffect[c][sq][obstacle_tate];  //LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
//		break;
//	case BISHOP:
//		// �̕������������ł��邩�H�H
//		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
//		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
//		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
//		break;
//	case ROOK:
//		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
//		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
//		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
//		break;
//	case UNICORN:
//		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
//		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
//		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
//		break;
//	case DRAGON:
//		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
//		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
//		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
//		break;
//	default:
//		ASSERT(0);
//		return ALLBB;
//		break;
//	}
//
//}


/*


occ256 ��p������������

*/


Bitboard long_effect(const Occ_256 & occ, const Color c, const Piece pt, const Square sq)
{

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;
	switch (pt)
	{
	
	case LANCE:
		obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		return LanceEffect[c][sq][obstacle_tate];
		break;
	case BISHOP:
		obstacle_plus45 = (occ.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		obstacle_Minus45 = (occ.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45];
		break;
	case ROOK:
		obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		obstacle_yoko = (occ.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case UNICORN:
		obstacle_plus45 = (occ.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		obstacle_Minus45 = (occ.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45]| StepEffect[BLACK][KING][sq];
		break;
	case DRAGON:
		obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		obstacle_yoko = (occ.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko]|StepEffect[BLACK][KING][sq];
		break;
	default:
		UNREACHABLE;
		return ALLBB;
		break;
	}
}

Bitboard lance_effect(const Occ_256 & occ, const Color c, const Square sq)
{
	const uint8_t obstacle_tate=(occ.b64(0)>>occ256_shift_table_tate[sq])&effectmask;
	return LanceEffect[c][sq][obstacle_tate];
}

Bitboard rook_effect(const Occ_256 & occ, const Square sq)
{
	const uint8_t obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
	const uint8_t obstacle_yoko = (occ.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
}

Bitboard bishop_effect(const Occ_256 & occ, const Square sq)
{
	const uint8_t obstacle_plus45 = (occ.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
	const uint8_t obstacle_Minus45 = (occ.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
	return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45];
}

Bitboard dragon_effect(const Occ_256 & occ, const Square sq)
{
	const uint8_t obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
	const uint8_t obstacle_yoko = (occ.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[BLACK][KING][sq];
}

Bitboard unicorn_effect(const Occ_256 & occ, const Square sq)
{
	const uint8_t obstacle_plus45 = (occ.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
	const uint8_t obstacle_Minus45 = (occ.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
	return LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][obstacle_Minus45] | StepEffect[BLACK][KING][sq];
}


/*
Bitboard ��p������������
*/

Bitboard lance_effect(const Bitboard& occ, const Color c, const Square sq) {

	uint8_t obstacle_tate;

	obstacle_tate = (occ.b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������i�ŏ\�����H�H�H�H�j
	return LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
}

Bitboard rook_effect(const Bitboard& occ_tate, const Bitboard& occ_yoko, const Square sq) {

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;

	obstacle_tate = (occ_tate.b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������i�ŏ\�����H�H�H�H�j
	obstacle_yoko = (occ_yoko.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
}

Bitboard bishop_effect(const Bitboard& occ_p45, const Bitboard& occ_m45, const Square sq) {

	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;

	obstacle_plus45 = (occ_p45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
	obstacle_Minus45 = (occ_m45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
	return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];

}

Bitboard dragon_effect(const Bitboard& occ_tate, const Bitboard& occ_yoko, const Square sq) {

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;

	obstacle_tate = (occ_tate.b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������i�ŏ\�����H�H�H�H�j
	obstacle_yoko = (occ_yoko.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[BLACK][KING][sq];
}

Bitboard unicorn_effect(const Bitboard& occ_p45, const Bitboard& occ_m45, const Square sq) {

	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;

	obstacle_plus45 = (occ_p45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
	obstacle_Minus45 = (occ_m45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
	return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[BLACK][KING][sq];

}

void check_directtable()
{
	cout << "check direct table " << endl;

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout << "from " << sq << endl;
		for (Rank r = RankA; r < Rank_Num; r++) {
			for (File f = File9; f >= File1; f--) {
				Square to = make_square(r, f);
				cout << direct_table[sq][to];
			}
			cout << endl;
		}
	}
	cout << endl;


}

void check_between()
{
	for (Square from = SQ_ZERO; from < SQ_NUM; from++) {
		for (Square to = SQ_ZERO; to < SQ_NUM; to++) {

			cout << "from "<<from << " to "<<to<< endl;
			cout << BetweenBB[from][to] << endl;


		}
	}


}
