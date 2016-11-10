#pragma once


#include "fundation.h"
#include "Bitboard.h"
#include "occupied.h"
#include <string>
#include <bitset>
using namespace std;


#define  sfen_hirate "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
#define  sfen_maturi "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1"
#define	 sfen_max    "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g17p3n 1"



/*
����ϗ����e�[�u�������̍����v�Z�Ƃ��ȒP����Ȃ��̂ł�߂�
wordboard�Ƃ������ł���悤�ɂȂ����Ȃ玝���Ă�������������Ȃ�
*/

struct StateInfo
{
	int ply_from_root = 0;
	Square ksq_[ColorALL];
	//Effect Eboard[ColorALL][SQ_NUM];//���ɂ����̌����������Ă��邩��ێ����邽�߂�board
	//---------------�����܂�domove�ŃR�s�[

	Move lastmove = MOVE_NONE;
	Piece DirtyPiece[2];//do_move�ɂ���ē�������0 ���ꂽ��P
	bool inCheck = false;

	Bitboard checker = ZeroBB;//�����ł�2�d����ɂȂ邱�Ƃ����邽��bitboard�ł����˂΂Ȃ�Ȃ��B

	Value material;
	Value pp;

	StateInfo* previous = nullptr;//undo_move�ňȑO�̋ǖʂɖ߂�ׂ̒P�������X�g
};


struct Position 
{
private:

	Bitboard occupied[ColorALL];//colorALL=2�Ȃ̂Ŏ�Ԋ֌W�Ȃ�Bitboard�͗p�ӂ��Ă��Ȃ��B
	Bitboard occupied90;
	Bitboard occupied_plus45, occupied_minus45;

	Bitboard occupiedPt[ColorALL][PT_ALL];

	//���̑��݂��鏡��1�ɂȂ��Ă���bitboard�R����andnot���邱�Ƃŕ��ł���target���i���i�Q������ł���j
	Bitboard ExistPawnBB[ColorALL];
	Piece pcboard[SQ_NUM];

	StateInfo* st;
	StateInfo initSt;//st*���ŏ��Ɏw���Ă���Stateinfo
	Color sidetomove_;
	Hand hands[ColorALL];

	uint64_t nodes;

public:

	Position() { set_hirate(); }
	void set(std::string sfen);
	void set_hirate() { set(sfen_hirate); }//����ɃZ�b�g��OK

	void clear();

	void remove_piece(const Color c, const Piece pt, const Square sq);
	void put_piece(const Color c, const Piece pt, const Square sq);

	void put_rotate(const Square sq) {

		ASSERT(is_ok(Square(sq_to_sq90(sq))));
		occupied90 ^= SquareBB[sq_to_sq90(sq)];
		ASSERT(is_ok(Square(sq_to_sqplus45(sq))));
		occupied_plus45 ^= SquareBB[sq_to_sqplus45(sq)];
		ASSERT(is_ok(Square(sq_to_sqminus45(sq))));
		occupied_minus45^= SquareBB[sq_to_sqminus45(sq)];

	}

	//put��remove��xor�Ȃ̂ł���Ă��邱�Ƃ͓����������O���炢�ς��Ă������ق�����������
	void remove_rotate(const Square sq) {

		ASSERT(is_ok(Square(sq_to_sq90(sq))));
		occupied90 ^= SquareBB[sq_to_sq90(sq)];
		ASSERT(is_ok(Square(sq_to_sqplus45(sq))));
		occupied_plus45 ^= SquareBB[sq_to_sqplus45(sq)];
		ASSERT(is_ok(Square(sq_to_sqminus45(sq))));
		occupied_minus45 ^= SquareBB[sq_to_sqminus45(sq)];

	}

	void check_effect();


	void do_move(Move m, StateInfo* st);
	void undo_move();

	inline Piece piece_on(Square sq)const { return pcboard[sq]; }
	Bitboard occ_pt(Color c, Piece pt)const { return occupiedPt[c][pt]; }
	Bitboard occ(Color c)const { return occupied[c]; }

	Bitboard occ_all() const { return (occupied[BLACK]|occupied[WHITE]); }
	Bitboard occ_90()const { return occupied90; }
	Bitboard occ_plus45()const { return occupied_plus45; }
	Bitboard occ_minus45()const { return occupied_minus45; }

	Bitboard pawnbb(const Color c)const { return ExistPawnBB[c]; }

	Color sidetomove() const { return sidetomove_; }
	Hand hand(Color c)const { return hands[c]; }
	StateInfo* state() const { return st; }
	Square ksq(const Color c)const { return st->ksq_[c]; }
	bool is_incheck()const { return st->inCheck; }

	void check_occbitboard()const;





	//c���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	/*
	���ו���
	��했��to�ɑ���̐F�̋��u���Ă݂��Ƃ��̌����͈̔͂ɂ��̋�킪���݂���΂���͂��̃}�X�ɗ����������Ă���Ƃ������Ƃɐ���
	*/
	Bitboard effect_toBB(const Color US, const Square to) {

		Color ENEMY = opposite(US);

		/*
		�Տ��UNICORN,DRAGON�����邩�ǂ������m�F�����ق����f���������ł��邩�H�H�H
		*/
		return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (long_effect(*this, ENEMY, LANCE, to)&occ_pt(US, LANCE))//���������߂�ɂ�occupied ����������̂�pos��n�����ق������S���H�i���Ƃ�pos��n���Ƃ���occ�����ꂼ��n���Ƃ��Ƃő����𒲂ׂ�B�j
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (long_effect(*this, ENEMY, ROOK, to)&occ_pt(US, ROOK))
			| (long_effect(*this, ENEMY, BISHOP, to)&occ_pt(US, BISHOP))
			| (long_effect(*this, ENEMY, DRAGON, to)&occ_pt(US, DRAGON))
			| (long_effect(*this, ENEMY, UNICORN, to)&occ_pt(US, UNICORN));
	}

	//c���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	//���������߂�ɂ�occupied ����������̂�pos��n�����ق������S���H�i���Ƃ�pos��n���Ƃ���occ�����ꂼ��n���Ƃ��Ƃő����𒲂ׂ�B�j
	bool is_effect_to(const Color US, const Square to) {

		Color ENEMY = opposite(US);

		if ((long_effect(*this, ENEMY, ROOK, to)&occ_pt(US, ROOK)).isNot()) { return true; }
		if ((long_effect(*this, ENEMY, BISHOP, to)&occ_pt(US, BISHOP)).isNot()) { return true; }
		if ((long_effect(*this, ENEMY, DRAGON, to)&occ_pt(US, DRAGON)).isNot()) { return true; }
		if ((long_effect(*this, ENEMY, UNICORN, to)&occ_pt(US, UNICORN)).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN)).isNot()) { return true; }
		if ((long_effect(*this, ENEMY, LANCE, to)&occ_pt(US, LANCE)).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT)).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER)).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER))).isNot()) { return true; }
		if ((step_effect(ENEMY, KING, to)&occ_pt(US, KING)).isNot()) { return true; }
	
		
		return false;
	}

	void check_effecttoBB() {
		for (Color c = BLACK; c <= ColorALL; c++) {
			cout << c << endl;
			for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
				cout << "Square " << sq << endl;
				cout << effect_toBB(c, sq) << endl;
			}
		}
	}

	void init_existpawnBB() {
		for (Color c = BLACK; c < ColorALL; c++) {
			Bitboard occ = occ_pt(c, PAWN);
			while (occ.isNot()) {
				Square sq = occ.pop();
				ExistPawnBB[c] |= FileBB[sqtofile(sq)];
			}
		}
	}
	void remove_existpawnBB(const Color c,const Square sq) {
		ExistPawnBB[c]=ExistPawnBB[c] & ~FileBB[sqtofile(sq)];
	}
	void add_existpawnBB(const Color c, const Square sq) {
		ExistPawnBB[c] |=FileBB[sqtofile(sq)];
	}

	void print_existpawnBB() {
		cout << " print existpawn BB " << endl;
		for (Color c = BLACK; c < ColorALL; c++) {
			cout << " color " << c << endl;
			cout << ExistPawnBB[c] << endl;
		}
	}


	

	/*
	����ł͒x�����.....
	�����ƃX�}�[�g�ȏ������~����
	*/
	bool is_uchihu(const Color us, const Square pawnsq) {

		Square frompawn;
		Color ENEMY = opposite(us);
		us == BLACK ? frompawn = Square(-1) : frompawn = Square(1);

		//���̑O��KING�łȂ���΂���͑ł����l�߂ł͂Ȃ��B
		if (piece_on(pawnsq + frompawn) != KING) { return false; }

		//���̋@��͐��ł����ł������ł�����
		//��������ꏊ�͉���������͈͂Ŏ����̋���Ȃ��ꏊ
		Bitboard escape = StepEffect[BLACK][KING][pawnsq + frompawn] &~ occ(ENEMY);
		while (escape.isNot()) {

			Square to = escape.pop();
			if (is_effect_to(ENEMY, to)) { return true; }
		}

		return false;
	}


	//to�ɑ���̌����������Ă���΂���͎��E��
	bool is_king_suiside(const Color us, const Square kingto) {

		Color ENEMY = opposite(us);
		if (is_effect_to(ENEMY, kingto)) { return true; }
		return false;

	}

	//���̊֐��őł����l�߁A���̎��E����Ȃ��B
	/*
	�w���肪�Ȃ����m���͔��ɒႢ����
	�w�����loop�ł��̊֐����Ăяo���̂͌��̕��ł�����������Ȃ��iStockfish�����j
	��������������������������������������������������������������������������PIN����Ă����𓮂����Ȃ��悤�ɂ��鏈��������I�I�I�I�I�I�I�I
	*/
	bool is_legal(const Move m) {

		Piece movedpiece = moved_piece(m);

		if (piece_type(movedpiece) == KING) {
			if (is_king_suiside(sidetomove_, move_to(m))) { return false; }
		}

		bool  isDrop = is_drop(m);
		if (isDrop&& piece_type(movedpiece) == PAWN) {
			if (is_uchihu(sidetomove_, move_to(m))) { return false; }
		}

		//pin����Ă����𓮂����Ȃ��悤�ɂ���



		//��������͎����̋�
		ASSERT(piece_color(movedpiece) == c);
		//from�ɂ����Ɠ��������Ƃ��Ă����͓���
		ASSERT(!isDrop&&piece_on(move_from(m)) == movedpiece);
		//��낤�Ƃ��Ă����͎����̋�ł͂Ȃ�
		ASSERT(piece_on(move_to(m)) == NO_PIECE || piece_color(piece_on(move_to(m))) != c);

		return true;
	}


};

std::ostream& operator<<(std::ostream& os, const Position& pos);
