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

struct StateInfo
{
	int ply_from_root = 0;
	Square ksq_[ColorALL];
	uint8_t Eboard[ColorALL][SQ_NUM];//���ɂ����̌����������Ă��邩��ێ����邽�߂�board
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

	Color sidetomove() const { return sidetomove_; }
	Hand hand(Color c)const { return hands[c]; }
	StateInfo* state() const { return st; }


	void check_occbitboard();



};

std::ostream& operator<<(std::ostream& os, const Position& pos);
