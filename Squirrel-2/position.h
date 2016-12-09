#pragma once


#include "fundation.h"
#include "Bitboard.h"
#include "occupied.h"
#include "evaluate.h"
#include <string>
#include <bitset>
#include "Hash.h"
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
	int32_t bpp=Value_error,wpp=Value_error;

	//��������0 ���ꂽ��1
	Eval::BonaPiece dirtybonap_fb[2];
	Eval::BonaPiece dirtybonap_fw[2];
	Eval::UniformNumber dirtyuniform[2] = { Eval::Num_Uniform, Eval::Num_Uniform};

	void clear_stPP() {
		bpp = Value_error; wpp = Value_error;
	}

	

	friend struct Position;

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

	Thread* searcherthread;
	Eval::BonaPList list;

	uint64_t nodes;

public:

	Position() { set_hirate(); }
	void set(std::string sfen);
	void set_hirate() { set(sfen_hirate); }//����ɃZ�b�g��OK

	string make_sfen();
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
	void check_longeffect();

	Eval::BonaPList evallist() const { return list; }



	void do_move(const Move m, StateInfo* st);
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
		return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (lance_effect(occ_all(), ENEMY, to)&occ_pt(US, LANCE))
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (rook_effect(occ_all(), occ_90(), to)&occ_pt(US, ROOK))
			| (bishop_effect(occ_plus45(), occ_minus45(), to)&occ_pt(US, BISHOP))
			| (occ_pt(US, DRAGON)&dragon_effect(occ_all(), occ_90(), to))
			| (occ_pt(US, UNICORN)&unicorn_effect(occ_plus45(), occ_minus45(), to));
			
	}

	//c���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	//���������߂�ɂ�occupied ����������̂�pos��n�����ق������S���H�i���Ƃ�pos��n���Ƃ���occ�����ꂼ��n���Ƃ��Ƃő����𒲂ׂ�B�j
	bool is_effect_to(const Color US, const Square to)const {

		Color ENEMY = opposite(US);

		
		if ((rook_effect(occ_all(), occ_90(), to)&occ_pt(US, ROOK)).isNot()) { return true; }
		if ((bishop_effect(occ_plus45(), occ_minus45(), to)&occ_pt(US, BISHOP)).isNot()) { return true; }
		if ((occ_pt(US, DRAGON)&dragon_effect(occ_all(), occ_90(), to)).isNot()) { return true; }
		if ((occ_pt(US, UNICORN)&unicorn_effect(occ_plus45(), occ_minus45(), to)).isNot()) { return true; }
		if ((lance_effect(occ_all(), ENEMY, to)&occ_pt(US, LANCE)).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN)).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT)).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER)).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER))).isNot()) { return true; }
		if ((step_effect(ENEMY, KING, to)&occ_pt(US, KING)).isNot()) { return true; }
	
		
		return false;
	}

	//���̎��E��𒲂ׂ邽�߂Ɏ����̂���ꏊ��&~����occ��p���Ď��E���ǂ������ׂ�֐�
	//US���̌�����to�Ɍ����Ă��邩�ǂ������ׂ�ׂ̊֐��B
	bool is_effect_to_Removeking(const Color US, const Square to,const Square  ksq)const {

		Color ENEMY = opposite(US);
		//Bitboard ourksq= SquareBB[ksq];

		Bitboard occR = occ_all()&~SquareBB[ksq];
		Bitboard occ90R = occ_90() &~ SquareBB[sq_to_sq90(ksq)];
		Bitboard occp45R = occ_plus45() &~ SquareBB[sq_to_sqplus45(ksq)];
		Bitboard occm45R = occ_minus45() &~  SquareBB[sq_to_sqminus45(ksq)];

		if ((rook_effect(occR, occ90R, to)&occ_pt(US, ROOK)).isNot()) { return true; }
		if ((bishop_effect(occp45R, occm45R, to)&occ_pt(US, BISHOP)).isNot()) { return true; }
		if ((occ_pt(US, DRAGON)&dragon_effect(occR, occ90R, to)).isNot()) { return true; }
		if ((occ_pt(US, UNICORN)&unicorn_effect(occp45R, occm45R, to)).isNot()) { return true; }
		if ((lance_effect(occR, ENEMY, to)&occ_pt(US, LANCE)).isNot()) { return true; }
		if ((step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN)).isNot()) { return true; }
		if ((step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT)).isNot()) { return true; }
		if ((step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER)).isNot()) { return true; }
		if ((step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER))).isNot()) { return true; }
		if ((step_effect(ENEMY, KING, to)&occ_pt(US, KING)).isNot()) { return true; }


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

		if (is_drop(m)&&moved_piece(m)==PAWN) {

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
			obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
			//UP�̏ꍇ�ł�DOWN�ɒ������킯���Ȃ��̂Łidown�����ɂ͋ʂ����邽�ߑ��݂���Ă͍���j��INfrontofBB�Ƃ����Ȃ��Ă�����
			return ((LongRookEffect_tate[sq][obstacle_tate])&(occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON) | occ_pt(ENEMY, LANCE))).isNot();
			break;

		case RightUP:
			obstacle_plus45 = (occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
			return (LongBishopEffect_plus45[sq][obstacle_plus45] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case Right:
			obstacle_yoko = (occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
			return (LongRookEffect_yoko[sq][obstacle_yoko] & (occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON))).isNot();
			break;

		case RightDOWN:
			obstacle_Minus45 = (occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
			return  (LongBishopEffect_minus45[sq][(obstacle_Minus45)] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case DOWN:
			
			obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
			return ((LongRookEffect_tate[sq][obstacle_tate])&(occ_pt(ENEMY, ROOK) |occ_pt(ENEMY,DRAGON)| occ_pt(ENEMY, LANCE))).isNot();
			break;

		case LeftDOWN:
			obstacle_plus45 = (occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
			return (LongBishopEffect_plus45[sq][obstacle_plus45] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		case Left:
			obstacle_yoko = (occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
			return (LongRookEffect_yoko[sq][obstacle_yoko] & (occ_pt(ENEMY, ROOK) | occ_pt(ENEMY, DRAGON))).isNot();
			break;

		case LeftUP:
			obstacle_Minus45 = (occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
			return  (LongBishopEffect_minus45[sq][(obstacle_Minus45)] & (occ_pt(ENEMY, BISHOP) | occ_pt(ENEMY, UNICORN))).isNot();
			break;

		default:
			UNREACHABLE;
			return false;
			break;
		}

	}
	

	/*
	����ł͒x�����.....
	�����ƃX�}�[�g�ȏ������~����
	*/
	//�����Ɠ����Ă邩�`�F�b�N����I�I(�����Ɠ�����)
	bool is_uchihu(const Color us, const Square pawnsq)const {

		Square frompawn;
		Color ENEMY = opposite(us);
		us == BLACK ? frompawn = Square(-1) : frompawn = Square(1);
		Piece ENEMYKING = (us == BLACK) ? W_KING : B_KING;

		//���̑O��KING�łȂ���΂���͑ł����l�߂ł͂Ȃ��B
		if (piece_on(pawnsq + frompawn) != ENEMYKING) { return false; }

		//���̋@��͐��ł����ł������ł�����
		//��������ꏊ�͉���������͈͂Ŏ����̋���Ȃ��ꏊ
		Bitboard escape = StepEffect[BLACK][KING][pawnsq + frompawn] &~ occ(ENEMY);
		while (escape.isNot()) {

			Square to = escape.pop();
			if (!is_effect_to(ENEMY, to)) { return false; }
		}

		return true;
	}


	//to�ɑ���̌����������Ă���΂���͎��E��//OK
	bool is_king_suiside(const Color us, const Square kingto,const Square from) const{

		Color ENEMY = opposite(us);
		if (is_effect_to_Removeking(ENEMY, kingto,from)) { return true; }
		return false;

	}

	inline bool capture_or_propawn(const Move m)const {

		if (pcboard[move_to(m)] != NO_PIECE) {
			//��Ԃ̋����낤�Ƃ��Ă��Ȃ����̃`�F�b�N�͂��Ă���
			ASSERT(piece_color(pcboard[move_to(m)]) != sidetomove());
			return true;
		}
		else if (piece_type(moved_piece(m)) == PAWN&&is_promote(m)) {
			return true;
		}

		return false;
	}



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

	//zoblisthash�֘A
	void init_hash();
	inline Key key() {
		return Key(st->board_ + st->hands_);
	}

};

std::ostream& operator<<(std::ostream& os, const Position& pos);
