#pragma once


#include "fundation.h"
#include "Bitboard.h"
#include "occupied.h"
#include "evaluate.h"
#include <string>
#include <bitset>
#include "Hash.h"
#include "occupied_m256.h"
using namespace std;


#define  sfen_hirate "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
#define  sfen_maturi "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1"
#define	 sfen_max    "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g17p3n 1"


struct Thread;

/*
����ϗ����e�[�u�������̍����v�Z�Ƃ��ȒP����Ȃ��̂ł�߂�
wordboard�Ƃ������ł���悤�ɂȂ����Ȃ玝���Ă�������������Ȃ�
*/

struct StateInfo
{
	


	//zoblist hash
	Key board_=0;//�L�[�{�[�h�i�Ȃ�Ă˂��j��ԏ���board�ɓ���Ă����B
	Key hands_=0;//�������Zoblist key
	int ply_from_startpos = 0;
	Square ksq_[ColorALL];
	//Effect Eboard[ColorALL][SQ_NUM];//���ɂ����̌����������Ă��邩��ێ����邽�߂�board

	//---------------==============================================�����܂�domove�ŃR�s�[�����

	Move lastmove = MOVE_NONE;
	Piece DirtyPiece[2];//do_move�ɂ���ē�������0 ���ꂽ��P
	bool inCheck = false;

	Bitboard checker = ZeroBB;//�����ł�2�d����ɂȂ邱�Ƃ����邽��bitboard�ł����˂΂Ȃ�Ȃ��B

	Value material;
#ifdef EVAL_PP
	int32_t bpp=Value_error,wpp=Value_error;

#ifdef EVAL_PROG
	int32_t bppf = Value_error, wppf = Value_error;
#endif

#elif EVAL_KPP 1
	int sumKKP=Value_error;
	int sumBKPP=Value_error;
	int sumWKPP=Value_error;
#endif


	int32_t bkp = Value_error, wkp = Value_error;//�i�s�x

	//��������0 ���ꂽ��1
	Eval::BonaPiece dirtybonap_fb[2];
	Eval::BonaPiece dirtybonap_fw[2];
	Eval::UniformNumber dirtyuniform[2] = { Eval::Num_Uniform, Eval::Num_Uniform};

	void clear_stPP() {
#ifdef EVAL_PP
		bpp = Value_error; wpp = Value_error;
#ifdef EVAL_PROG
		 bppf = Value_error, wppf = Value_error;
#endif

#elif EVAL_KPP 1
		sumKKP = Value_error;
		sumBKPP = Value_error;
		sumWKPP = Value_error;
#endif

		bkp = Value_error; wkp = Value_error;
	}

	void clear() {
		memset(this, 0, sizeof(this));
		clear_stPP();
	}

	friend struct Position;

	StateInfo* previous = nullptr;//undo_move�ňȑO�̋ǖʂɖ߂�ׂ̒P�������X�g
};


struct Position 
{
private:

	Occ_256 occ256;

	Bitboard occupied[ColorALL];//colorALL=2�Ȃ̂Ŏ�Ԋ֌W�Ȃ�Bitboard�͗p�ӂ��Ă��Ȃ��B
	/*Bitboard occupied90;
	Bitboard occupied_plus45, occupied_minus45;*/

	Bitboard occupiedPt[ColorALL][PT_ALL];

	//���̑��݂��鏡��1�ɂȂ��Ă���bitboard�R����andnot���邱�Ƃŕ��ł���target���i���i�Q������ł���j
	Bitboard ExistPawnBB[ColorALL];
	Piece pcboard[SQ_NUM];

	StateInfo* st;
	StateInfo initSt;//st*���ŏ��Ɏw���Ă���Stateinfo
	Color sidetomove_;
	Hand hands[ColorALL];

	Thread* searcherthread;
	Eval::BonaPList list;

	uint64_t nodes;

	double prog;
	
public:
	bool packed_sfen[256];

	void change_stm() { sidetomove_ = opposite(sidetomove_); }

	int ply_from_startpos;

	inline void set_occ256(const Square sq) { occ256 ^= SquareBB256[sq]; }
	inline void remove_occ256(const Square sq) { occ256^=SquareBB256[sq]; }

	Position() { set_hirate(); }
	void set(std::string sfen);
	void set_hirate() { set(sfen_hirate); }//����ɃZ�b�g��OK

	string make_sfen()const ;
	void clear();

	void remove_piece(const Color c, const Piece pt, const Square sq);
	void put_piece(const Color c, const Piece pt, const Square sq);

	double ret_prog() const{ return prog; }

	//void put_rotate(const Square sq) {

	//	ASSERT(is_ok(Square(sq_to_sq90(sq))));
	//	occupied90 ^= SquareBB[sq_to_sq90(sq)];
	//	ASSERT(is_ok(Square(sq_to_sqplus45(sq))));
	//	occupied_plus45 ^= SquareBB[sq_to_sqplus45(sq)];
	//	ASSERT(is_ok(Square(sq_to_sqminus45(sq))));
	//	occupied_minus45^= SquareBB[sq_to_sqminus45(sq)];

	//}

	////put��remove��xor�Ȃ̂ł���Ă��邱�Ƃ͓����������O���炢�ς��Ă������ق�����������
	//void remove_rotate(const Square sq) {

	//	ASSERT(is_ok(Square(sq_to_sq90(sq))));
	//	occupied90 ^= SquareBB[sq_to_sq90(sq)];
	//	ASSERT(is_ok(Square(sq_to_sqplus45(sq))));
	//	occupied_plus45 ^= SquareBB[sq_to_sqplus45(sq)];
	//	ASSERT(is_ok(Square(sq_to_sqminus45(sq))));
	//	occupied_minus45 ^= SquareBB[sq_to_sqminus45(sq)];

	//}

	void check_effect();
	void check_effectocc256();
	void check_longeffect();
	void check_longeffect256();
	Eval::BonaPList evallist() const { return list; }



	void do_move(const Move m, StateInfo* st);
	void do_move(const Move m, StateInfo* st,const bool givescheck);
	void undo_move();

	void do_nullmove(StateInfo* newst);
	void undo_nullmove();

	inline Piece piece_on(Square sq)const { return pcboard[sq]; }
	Bitboard occ_pt(Color c, Piece pt)const { return occupiedPt[c][pt]; }
	Bitboard occ(Color c)const { return occupied[c]; }

	Bitboard occ_all() const { return (occupied[BLACK]|occupied[WHITE]); }
	/*Bitboard occ_90()const { return occupied90; }
	Bitboard occ_plus45()const { return occupied_plus45; }
	Bitboard occ_minus45()const { return occupied_minus45; }*/

	Occ_256 ret_occ_256()const { return occ256; }

	Bitboard pawnbb(const Color c)const { return ExistPawnBB[c]; }

	//������bplist���X�V����Ă��邩�m�F����
	void check_bplist() {
		Eval::BonaPList list2;
		Eval::BonaPList copyedlist;

		copyedlist = evallist();

		list2.makebonaPlist(*this);

		

		sort(copyedlist.bplist_fb,&copyedlist.bplist_fb[40], [](const Eval::BonaPiece& m1, const  Eval::BonaPiece& m2) { return m1 <m2; });
		sort(list2.bplist_fb, &list2.bplist_fb[40], [](const Eval::BonaPiece& m1, const  Eval::BonaPiece& m2) { return m1 <m2; });
		sort(copyedlist.bplist_fw, &copyedlist.bplist_fw[40], [](const Eval::BonaPiece& m1, const  Eval::BonaPiece& m2) { return m1 <m2; });
		sort(list2.bplist_fw, &list2.bplist_fw[40], [](const Eval::BonaPiece& m1, const  Eval::BonaPiece& m2) { return m1 <m2; });

		for (int i = 0; i < 40; i++) {
			ASSERT(copyedlist.bplist_fb[i] == list2.bplist_fb[i]);
			ASSERT(copyedlist.bplist_fw[i] == list2.bplist_fw[i]);
		}
	}

	Color sidetomove() const { return sidetomove_; }
	Hand hand(Color c)const { return hands[c]; }
	StateInfo* state() const { return st; }
	Square ksq(const Color c)const { return st->ksq_[c]; }
	bool is_incheck()const { return st->inCheck; }

	void check_occbitboard()const;

	void set_searcherthread(Thread *th) {
		searcherthread = th;
	}

	Thread* searcher() const { return searcherthread; }

	uint64_t searched_nodes()const { return nodes; }

	//c���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	/*
	���ו���
	��했��to�ɑ���̐F�̋��u���Ă݂��Ƃ��̌����͈̔͂ɂ��̋�킪���݂���΂���͂��̃}�X�ɗ����������Ă���Ƃ������Ƃɐ���
	*/
	Bitboard effect_toBB(const Color US, const Square to)const {

		Color ENEMY = opposite(US);

		/*
		�Տ��UNICORN,DRAGON�����邩�ǂ������m�F�����ق����f���������ł��邩�H�H�H
		*/
		//�R����ы@��ɂ��Ė���switch���ŏ������򂵂Ă��邩�玞�Ԃ����ʁI��ь����͋��ʂɗ��������߂�֐����쐬����I�I�I
		//�Ȃ�ł���Ȃ��Ƃɂ��C�Â��Ȃ������̂�(�L�G�ցG�M)
		/*return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (lance_effect(occ_all(), ENEMY, to)&occ_pt(US, LANCE))
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (rook_effect(occ_all(), occ_90(), to)&occ_pt(US, ROOK))
			| (bishop_effect(occ_plus45(), occ_minus45(), to)&occ_pt(US, BISHOP))
			| (occ_pt(US, DRAGON)&dragon_effect(occ_all(), occ_90(), to))
			| (occ_pt(US, UNICORN)&unicorn_effect(occ_plus45(), occ_minus45(), to));*/
			
		return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (lance_effect(occ256, ENEMY, to)&occ_pt(US, LANCE))
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (rook_effect(occ256, to)&occ_pt(US, ROOK))
			| (bishop_effect(occ256, to)&occ_pt(US, BISHOP))
			| (occ_pt(US, DRAGON)&dragon_effect(occ256, to))
			| (occ_pt(US, UNICORN)&unicorn_effect(occ256, to));
	}

	Bitboard effect_toBB_withouteffectking(const Color US, const Square to)const {

		Color ENEMY = opposite(US);

		return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (lance_effect(occ256, ENEMY, to)&occ_pt(US, LANCE))
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			//| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (rook_effect(occ256, to)&occ_pt(US, ROOK))
			| (bishop_effect(occ256, to)&occ_pt(US, BISHOP))
			| (occ_pt(US, DRAGON)&dragon_effect(occ256, to))
			| (occ_pt(US, UNICORN)&unicorn_effect(occ256, to));
	}
	//c���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	//���������߂�ɂ�occupied ����������̂�pos��n�����ق������S���H�i���Ƃ�pos��n���Ƃ���occ�����ꂼ��n���Ƃ��Ƃő����𒲂ׂ�B�j
	bool is_effect_to(const Color US, const Square to)const {

		Color ENEMY = opposite(US);

		if ((rook_effect(occ256, to)&occ_pt(US, ROOK)).isNot()) { return true; }
		if ((bishop_effect(occ256, to)&occ_pt(US, BISHOP)).isNot()) { return true; }
		if ((occ_pt(US, DRAGON)&dragon_effect(occ256, to)).isNot()) { return true; }
		if ((occ_pt(US, UNICORN)&unicorn_effect(occ256, to)).isNot()) { return true; }
		if ((lance_effect(occ256, ENEMY, to)&occ_pt(US, LANCE)).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN)).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT)).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER)).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER))).isNot()) { return true; }
		if ((step_effect(ENEMY, KING, to)&occ_pt(US, KING)).isNot()) { return true; }

		return false;
	}

	//���̎��E��𒲂ׂ邽�߂Ɏ����̂���ꏊ��&~����occ��p���Ď��E���ǂ������ׂ�֐�
	//US���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	bool is_effect_to_removepiece(const Color US, const Square to,const Square  ksq)const {

		Color ENEMY = opposite(US);
		//Bitboard ourksq= SquareBB[ksq];

		Occ_256 occ256_ = occ256^SquareBB256[ksq];


		if ((rook_effect(occ256_, to)&occ_pt(US, ROOK)).isNot()) { return true; }
		if ((bishop_effect(occ256_, to)&occ_pt(US, BISHOP)).isNot()) { return true; }
		if ((occ_pt(US, DRAGON)&dragon_effect(occ256_, to)).isNot()) { return true; }
		if ((occ_pt(US, UNICORN)&unicorn_effect(occ256_, to)).isNot()) { return true; }
		if ((lance_effect(occ256_, ENEMY, to)&occ_pt(US, LANCE)).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN)).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT)).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER)).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER))).isNot()) { return true; }
		if ((step_effect(ENEMY, KING, to)&occ_pt(US, KING)).isNot()) { return true; }

		return false;
	}

	//������Ŏ��i�߂��㑊��̉��ɉ���������邱�Ƃ��ł��邩
	/*
	���̎�������do_move�Ɠ������Ƃ��J��Ԃ��Ă���̂Ŗ��ʂȂ悤�ȋC������....
	*/
	bool is_effect_to_move(const Color US, const Square to, const Move m)const {



		const Color ENEMY = opposite(US);
		Occ_256 occ256_ = ret_occ_256();
		const Square moveto = move_to(m);
		const Piece movedpiece = piece_type(moved_piece(m));//������pt����Ȃ��Ƃ����Ȃ�

		//�K�v�Ȃ͎̂�ԑ���occ_pt�Ȃ̂Ŏ�ԑ�����copy�ł��Ȃ����H�Hoccupied_pt+15�ȂǂőΉ��ł��邩�H
#if 0
		Bitboard occupiedPt_[ColorALL][PT_ALL];
		memcpy(occupiedPt_, occupiedPt, sizeof(occupiedPt));
#else
		Bitboard occupiedPt_[PT_ALL];
		if (US == BLACK) {	memcpy(occupiedPt_, occupiedPt[BLACK], sizeof(occupiedPt_));}
		else {	memcpy(occupiedPt_, occupiedPt[WHITE], sizeof(occupiedPt_));}
#endif

		if (is_drop(m)) {
			ASSERT(piece_on(moveto) == NO_PIECE);
			occ256_ ^= SquareBB256[moveto];
#if 0
			occupiedPt_[US][movedpiece] ^= SquareBB[moveto];
#else
			occupiedPt_[movedpiece] ^= SquareBB[moveto];
#endif
		}
		else {
			Square movefrom = move_from(m);
			ASSERT(piece_on(movefrom) != NO_PIECE);
			occ256_ |= SquareBB256[moveto];
			occ256_ ^= SquareBB256[movefrom];
#if 0
			occupiedPt_[US][movedpiece] ^= SquareBB[movefrom];
			if (is_promote(m)) {
				occupiedPt_[US][promotepiece(movedpiece)] ^= SquareBB[moveto];
			}
			else {
				occupiedPt_[US][movedpiece] ^= SquareBB[moveto];
			}
#else
			occupiedPt_[movedpiece] ^= SquareBB[movefrom];
			if (is_promote(m)) {
				occupiedPt_[promotepiece(movedpiece)] ^= SquareBB[moveto];
			}
			else {
				occupiedPt_[movedpiece] ^= SquareBB[moveto];
			}
#endif
			//capture�̏ꍇ�ł�����̋��occupiedpt�������K�v�͂Ȃ��ƍl������i������occ_pt��������肾����j
		}
#if 0
		if ((rook_effect(occ256_, to)&occupiedPt_[US][ROOK]).isNot()) { return true; }
		if ((bishop_effect(occ256_, to)&occupiedPt_[US][ BISHOP]).isNot()) { return true; }
		if ((occupiedPt_[US][ DRAGON]&dragon_effect(occ256_, to)).isNot()) { return true; }
		if ((occupiedPt_[US][ UNICORN]&unicorn_effect(occ256_, to)).isNot()) { return true; }
		if ((lance_effect(occ256_, ENEMY, to)&occupiedPt_[US][LANCE]).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occupiedPt_[US][ PAWN]).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occupiedPt_[US][KNIGHT]).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occupiedPt_[US][SILVER]).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occupiedPt_[US][ GOLD] | occupiedPt_[US][ PRO_PAWN] | occupiedPt_[US][ PRO_LANCE] | occupiedPt_[US][ PRO_NIGHT] | occupiedPt_[US][ PRO_SILVER])).isNot()) { return true; }
		//if ((step_effect(ENEMY, KING, to)&occupiedPt_[US][ KING]).isNot()) { return true; }//���ŉ���������邱�Ƃ͂ł��Ȃ�
#else
		if ((rook_effect(occ256_, to)&occupiedPt_[ROOK]).isNot()) { return true; }
		if ((bishop_effect(occ256_, to)&occupiedPt_[BISHOP]).isNot()) { return true; }
		if ((occupiedPt_[DRAGON] & dragon_effect(occ256_, to)).isNot()) { return true; }
		if ((occupiedPt_[UNICORN] & unicorn_effect(occ256_, to)).isNot()) { return true; }
		if ((lance_effect(occ256_, ENEMY, to)&occupiedPt_[LANCE]).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occupiedPt_[PAWN]).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occupiedPt_[KNIGHT]).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occupiedPt_[SILVER]).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occupiedPt_[GOLD] | occupiedPt_[PRO_PAWN] | occupiedPt_[PRO_LANCE] | occupiedPt_[PRO_NIGHT] | occupiedPt_[PRO_SILVER])).isNot()) { return true; }
		//if ((step_effect(ENEMY, KING, to)&occupiedPt_[ KING]).isNot()) { return true; }//���ŉ���������邱�Ƃ͂ł��Ȃ�
#endif
		return false;
	}

	void check_effecttoBB()const {
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
		ASSERT((ExistPawnBB[c] & FileBB[sqtofile(sq)]).isNot() == true);
		ExistPawnBB[c]=andnot(ExistPawnBB[c],FileBB[sqtofile(sq)]);
	}
	void add_existpawnBB(const Color c, const Square sq) {
		ASSERT((ExistPawnBB[c] & FileBB[sqtofile(sq)]).isNot() == false);
		ExistPawnBB[c] |=FileBB[sqtofile(sq)];
	}

	void print_existpawnBB() const{
		cout << " print existpawn BB " << endl;
		for (Color c = BLACK; c < ColorALL; c++) {
			cout << " color " << c << endl;
			cout << ExistPawnBB[c] << endl;
		}
	}


	bool check_nihu(const Move m) const{

		if (is_drop(m)&&piece_type(moved_piece(m))==PAWN) {

			if ((occ_pt(sidetomove(), PAWN)&FileBB[sqtofile(move_to(m))]).isNot()) {
				return true;
			}
		}

		return false;

	}
/*
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;

	obstacle_plus45 = (occ_p45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
	obstacle_Minus45 = (occ_m45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
	return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];


	uint8_t obstacle_yoko;

	obstacle_tate = (occ_tate.b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������i�ŏ\�����H�H�H�H�j
	obstacle_yoko = (occ_yoko.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
*/

	//UP�͎�ԂɊ֌W�Ȃ������
	/*
	Direction��������̔�ь��������Ă��邩�ǂ���
	*/
	bool is_longeffectdirection(const Color c,const Square sq,const Direction d) const{

		Color ENEMY = opposite(c);
		uint8_t obstacle_tate;
		uint8_t obstacle_plus45;
		uint8_t obstacle_Minus45;
		uint8_t obstacle_yoko;
		switch (d)
		{

		case UP:
			//obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
			obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
			//UP�̏ꍇ�ł�DOWN�ɒ������킯���Ȃ��̂Łidown�����ɂ͋ʂ����邽�ߑ��݂���Ă͍���j��INfrontofBB�Ƃ����Ȃ��Ă�����
			return ((LongRookEffect_tate[sq][obstacle_tate])&(occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON) | occ_pt(ENEMY, LANCE))).isNot();
			break;

		case RightUP:
			//obstacle_plus45 = (occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
			obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
			return (LongBishopEffect_plus45[sq][obstacle_plus45] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case Right:
			//obstacle_yoko = (occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
			obstacle_yoko = (occ256.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
			return (LongRookEffect_yoko[sq][obstacle_yoko] & (occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON))).isNot();
			break;

		case RightDOWN:
			//obstacle_Minus45 = (occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
			obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
			return  (LongBishopEffect_minus45[sq][(obstacle_Minus45)] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case DOWN:
			//obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
			obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
			return ((LongRookEffect_tate[sq][obstacle_tate])&(occ_pt(ENEMY, ROOK) |occ_pt(ENEMY,DRAGON)| occ_pt(ENEMY, LANCE))).isNot();
			break;

		case LeftDOWN:
			//obstacle_plus45 = (occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
			obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
			return (LongBishopEffect_plus45[sq][obstacle_plus45] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case Left:
			//obstacle_yoko = (occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
			obstacle_yoko = (occ256.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
			return (LongRookEffect_yoko[sq][obstacle_yoko] & (occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON))).isNot();
			break;

		case LeftUP:
			//obstacle_Minus45 = (occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
			obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
			return  (LongBishopEffect_minus45[sq][(obstacle_Minus45)] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		default:
			UNREACHABLE;
			return false;
			break;
		}

	}
	

	//see
	Value see(const Move m) const;
	Value see_sign(const Move m) const;
	bool see_ge(const Move m, const Value v)const;


	//Bitboard attackers_to(Color stm, Square to, Bitboard& occupied, Bitboard& oc90, Bitboard& ocm45, Bitboard& ocp45)const;
	Bitboard attackers_to(Color stm, Square to, Occ_256& occ)const;
	Bitboard attackers_to_all(const Square to, const Occ_256& occ) const;
	Piece min_attacker_pt(const Color stm,const Square to, const Bitboard& stmattacker, Bitboard& allattackers, Occ_256& occ,Bitboard& occupied)const ;

	/*
	����ł͒x�����.....
	�����ƃX�}�[�g�ȏ������~����
	*/
	//�����Ɠ����Ă邩�`�F�b�N����I�I(�����Ɠ�����)
	//�������ς肱�̊֐���������
	//����̖ڂ̑O�ɕ���ł����őł����l�߂ɐ����Ă��܂��Ă���I�I�I
	/*
	pawn��ł��Ă��A�����̋�ł���pawn������̂Ȃ�ł����l�߂ł͂Ȃ��I�I�I
	���������̋pin����Ă����ł���΂���͑ł����l��....
	�R�����C�����Ă��K�v������I
	*/
	bool is_uchihu(const Color us, const Square pawnsq)const {

		Square frompawn;
		Color ENEMY = opposite(us);
		us == BLACK ? frompawn = Square(-1) : frompawn = Square(1);
		Piece ENEMYKING = (us == BLACK) ? W_KING : B_KING;
		
		//���̑O��KING�łȂ���΂���͑ł����l�߂ł͂Ȃ��B
		if (piece_on(pawnsq + frompawn) != ENEMYKING) { return false; }
		ASSERT((pawnsq + frompawn) == state()->ksq_[ENEMY]);

		//���̋@��͐��ł����ł������ł�����
		//��������ꏊ�͉���������͈͂Ŏ����̋���Ȃ��ꏊ
		//enemy���ł����l�߂��ꂻ���ɐ����Ă����ɂƂ��Č���
		Bitboard escape = StepEffect[BLACK][KING][pawnsq + frompawn] &~ occ(ENEMY);
		while (escape.isNot()) {

			Square to = escape.pop();
			if (!is_effect_to(us, to)) { return false; }
		}

		//�����̋��pawn�����邩�H����̂Ȃ炻��𓮂������Ƃ��ʂɗ����͂��Ԃ�Ȃ���

		//pawn��������BB
		Bitboard garder = effect_toBB_withouteffectking(ENEMY, pawnsq);

		while (garder.isNot()) {
			Square sq = garder.pop();
			//sq�̋���Ȃ��Ȃ����Ƃ��Ă�king�ɗ����͂��Ԃ�Ȃ���
			//bool is_effect_to_Removeking(const Color US, const Square to,const Square  ksq)
			if (is_effect_to_removepiece(us, (pawnsq + frompawn), sq) == false) {
				return false;
			}
		}

		return true;
	}


	//to�ɑ���̌����������Ă���΂���͎��E��//OK
	bool is_king_suiside(const Color us, const Square kingto,const Square from) const{

		Color ENEMY = opposite(us);
		if (is_effect_to_removepiece(ENEMY, kingto,from)) { return true; }
		return false;

	}

	bool capture_or_propawn(const Move m)const;

	bool capture(const Move m)const;

	//���̊֐��őł����l�߁A���̎��E����Ȃ��B
	/*
	�w���肪�Ȃ����m���͔��ɒႢ����
	�w�����loop�ł��̊֐����Ăяo���̂͌��̕��ł�����������Ȃ��iStockfish�����j
	��������������������������������������������������������������������������PIN����Ă����𓮂����Ȃ��悤�ɂ��鏈��������I�I�I�I�I�I�I�I


	���̊֐���pin��̏����Ȃ��Ȃ��ǂ��Ȃ��̂Ō�ŉ��ǂ���

	�����Ɠ������m�F����

	�Q�̏󋵂ł����m�F�����������Ɠ����Ă�
	���������ׂĂ̏󋵂ɑ΂��Ă����Ɠ������ǂ����͕�����Ȃ��̂Ń����_���v���C���[�ŋǖʂ������߂ăe�X�g����֐���p�ӂ���
	*/
	bool is_legal(const Move m)const;
	bool pseudo_legal(const Move m) const;
	//zoblisthash�֘A
	void init_hash();
	inline Key key() {
		return Key(st->board_ + st->hands_);
	}

	/*
	����ʂɓ��������R�}���̑���R�}��u�����Ƃ��̌�������������ɂ��Ԃ��Ă���΂����Ă������Ă���
	�����͋󂫉���Ȃǂ������Ă���̂ł���قǊȒP�ł͂Ȃ��I


	�Ƃ�����do_move�̏����Ƃ��Ԃ��Ă��܂��̂ł���͕K�v�Ȃ����H�H
	do_move���������ɋǖʂɉ��肪�������Ă���΍��������������݂����Ȋ����ł����̂ł́H�H
	������givescheck�Ŏ}�؂�Ȃǂ��l����Ƃ�do_move�����Ă���}�؂���l����ƁAdo_move�ɂ͎��Ԃ������邵�ǖʂ�߂���������Ă���}�؂�����Ȃ���΂Ȃ�Ȃ��̂�
	���givescheck�����āA�����Ă��̒l��do_move�ɗp�������������̂�������Ȃ�....


	do_move������O��givescheck���m�F���Ă����Ă����ō�������������邩�ǂ������f����B
	do_move���ɂ�givescheck�Ŋm�F������񂩂炱�̎w���肪givescheck���ǂ����̏��������Ă���B

	*/
	bool is_gives_check(const Move m)const {

		return is_effect_to_move(sidetomove(),ksq(opposite(sidetomove_)),m);

	}//end of is_givescheck

	 /*
	 �^����ꂽ������ŋǖʂ𓮂��������hashkey���v�Z����A
	 �����prefetch�ɕK�v�ɂȂ�

	 prefetch�͂ł��邾���������Ă����K�v������̂�,,,��ɋǖʂ𓮂�����hashkey��p�ӂ��Ă܂�!!!
	 */
	Key key_after_move(const Move m);



	//�n�t�}��������
	string pack_haffman_sfen();
	string	unpack_haffman_sfen(bool *sfen);

	//���l
	Move mate1ply();
	bool cancapture_checkpiece(Square to);
	void slider_blockers(const Color stm, const Square s, Bitboard& dc_candicate, Bitboard& pinned) const;


	string random_startpos();
};

std::ostream& operator<<(std::ostream& os, const Position& pos);
