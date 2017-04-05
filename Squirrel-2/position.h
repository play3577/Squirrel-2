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
やっぱ利きテーブルを持つの差分計算とか簡単じゃないのでやめる
wordboardとか理解できるようになったなら持ってもいいかもしれない
*/

struct StateInfo
{
	


	//zoblist hash
	Key board_=0;//キーボード（なんてねっ）手番情報もboardに入れておく。
	Key hands_=0;//持ち駒のZoblist key
	int ply_from_startpos = 0;
	Square ksq_[ColorALL];
	//Effect Eboard[ColorALL][SQ_NUM];//升にいくつの効きが聞いているかを保持するためのboard

	//---------------==============================================ここまでdomoveでコピーされる

	Move lastmove = MOVE_NONE;
	Piece DirtyPiece[2];//do_moveによって動いた駒0 取られた駒１
	bool inCheck = false;

	Bitboard checker = ZeroBB;//将棋では2重王手になることがあるためbitboardでもたねばならない。

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


	int32_t bkp = Value_error, wkp = Value_error;//進行度

	//動いた駒0 取られた駒1
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

	StateInfo* previous = nullptr;//undo_moveで以前の局面に戻る為の単方向リスト
};


struct Position 
{
private:

	Occ_256 occ256;

	Bitboard occupied[ColorALL];//colorALL=2なので手番関係ないBitboardは用意していない。
	/*Bitboard occupied90;
	Bitboard occupied_plus45, occupied_minus45;*/

	Bitboard occupiedPt[ColorALL][PT_ALL];

	//歩の存在する升が1になっているbitboardコレをandnotすることで歩打ちのtargetを絞れる（２歩判定できる）
	Bitboard ExistPawnBB[ColorALL];
	Piece pcboard[SQ_NUM];

	StateInfo* st;
	StateInfo initSt;//st*が最初に指しておくStateinfo
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
	void set_hirate() { set(sfen_hirate); }//平手にセットはOK

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

	////putもremoveもxorなのでやっていることは同じだが名前ぐらい変えておいたほうがいいだろ
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

	//ちゃんとbplistが更新されているか確認する
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

	//c側の効きがtoに効いているかどうか調べる為の関数。
	/*
	調べ方は
	駒種毎にtoに相手の色の駒を置いてみたときの効きの範囲にその駒種が存在すればそれはそのマスに利きが効いているということに成る
	*/
	Bitboard effect_toBB(const Color US, const Square to)const {

		Color ENEMY = opposite(US);

		/*
		盤上にUNICORN,DRAGONがいるかどうかも確認したほうが素早く処理できるか？？？
		*/
		//コレ飛び機器について毎回switch文で条件分岐しているから時間が無駄！飛び効きは駒種別に利きを求める関数を作成する！！！
		//なんでこんなことにも気づかなかったのか(´；ω；｀)
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
	//c側の効きがtoに効いているかどうか調べる為の関数。
	//利きを求めるにはoccupied が複数いるのでposを渡したほうが安全か？（あとでposを渡すときとoccをそれぞれ渡すときとで速さを調べる。）
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

	//王の自殺手を調べるために自王のいる場所を&~したoccを用いて自殺かどうか調べる関数
	//US側の効きがtoに効いているかどうか調べる為の関数。
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

	//差し手で手を進めた後相手の王に王手をかけることができるか
	/*
	この実装だとdo_moveと同じことを繰り返しているので無駄なような気がする....
	*/
	bool is_effect_to_move(const Color US, const Square to, const Move m)const {



		const Color ENEMY = opposite(US);
		Occ_256 occ256_ = ret_occ_256();
		const Square moveto = move_to(m);
		const Piece movedpiece = piece_type(moved_piece(m));//ここはptじゃないといけない

		//必要なのは手番側のocc_ptなので手番側だけcopyできないか？？occupied_pt+15などで対応できるか？
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
			//captureの場合でも相手の駒のoccupiedptを消す必要はないと考えられる（自分のocc_ptだけが問題だから）
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
		//if ((step_effect(ENEMY, KING, to)&occupiedPt_[US][ KING]).isNot()) { return true; }//王で王手をかけることはできない
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
		//if ((step_effect(ENEMY, KING, to)&occupiedPt_[ KING]).isNot()) { return true; }//王で王手をかけることはできない
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

	obstacle_tate = (occ_tate.b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bitしか必要ないのでintでいいか（で十分か？？？？）
	obstacle_yoko = (occ_yoko.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
	return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
*/

	//UPは手番に関係なく上方向
	/*
	Direction方向からの飛び効きが来ているかどうか
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
			//UPの場合でもDOWNに跳駒がいるわけがないので（down方向には玉が入るため存在されては困る）＆INfrontofBBとかしなくてもいい
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
	これでは遅いよね.....
	もっとスマートな処理が欲しい
	*/
	//ちゃんと動いてるかチェックする！！(ちゃんと動いた)
	//いややっぱりこの関数おかしい
	//相手の目の前に歩を打つだけで打ち歩詰めに成ってしまっている！！！
	/*
	pawnを打っても、味方の駒でそのpawnを取れるのなら打ち歩詰めではない！！！
	しかしその駒がpinされている駒であればそれは打ち歩詰め....
	コレを修正してやる必要がある！
	*/
	bool is_uchihu(const Color us, const Square pawnsq)const {

		Square frompawn;
		Color ENEMY = opposite(us);
		us == BLACK ? frompawn = Square(-1) : frompawn = Square(1);
		Piece ENEMYKING = (us == BLACK) ? W_KING : B_KING;
		
		//歩の前がKINGでなければそれは打ち歩詰めではない。
		if (piece_on(pawnsq + frompawn) != ENEMYKING) { return false; }
		ASSERT((pawnsq + frompawn) == state()->ksq_[ENEMY]);

		//王の機器は先手でも後手でも同じでも同じ
		//逃げられる場所は王が動ける範囲で自分の駒がいない場所
		//enemyが打ち歩詰めされそうに成っている駒にとって見方
		Bitboard escape = StepEffect[BLACK][KING][pawnsq + frompawn] &~ occ(ENEMY);
		while (escape.isNot()) {

			Square to = escape.pop();
			if (!is_effect_to(us, to)) { return false; }
		}

		//味方の駒でpawnを取れるか？取れるのならそれを動かしたとき玉に利きはかぶらないか

		//pawnを取れる駒のBB
		Bitboard garder = effect_toBB_withouteffectking(ENEMY, pawnsq);

		while (garder.isNot()) {
			Square sq = garder.pop();
			//sqの駒がいなくなったとしてもkingに利きはかぶらないか
			//bool is_effect_to_Removeking(const Color US, const Square to,const Square  ksq)
			if (is_effect_to_removepiece(us, (pawnsq + frompawn), sq) == false) {
				return false;
			}
		}

		return true;
	}


	//toに相手の効きが効いていればそれは自殺手//OK
	bool is_king_suiside(const Color us, const Square kingto,const Square from) const{

		Color ENEMY = opposite(us);
		if (is_effect_to_removepiece(ENEMY, kingto,from)) { return true; }
		return false;

	}

	bool capture_or_propawn(const Move m)const;

	bool capture(const Move m)const;

	//この関数で打ち歩詰め、王の自殺手を省く。
	/*
	指し手が省かれる確率は非常に低いため
	指し手のloopでこの関数を呼び出すのは後ろの方でいいかもしれない（Stockfish方式）
	＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝PINされている駒を動かさないようにする処理もいる！！！！！！！！


	この関数のpin駒の処理なかなか良くないので後で改良する

	ちゃんと動くか確認する

	２つの状況でだけ確認したがちゃんと動いてた
	しかしすべての状況に対してちゃんと動くかどうかは分からないのでランダムプレイヤーで局面をすすめてテストする関数を用意する
	*/
	bool is_legal(const Move m)const;
	bool pseudo_legal(const Move m) const;
	//zoblisthash関連
	void init_hash();
	inline Key key() {
		return Key(st->board_ + st->hands_);
	}

	/*
	相手玉に動かしたコマ酒の相手コマを置いたときの効きが動かす先にかぶっていればおうてをかけている
	ここは空き王手なども入ってくるのでそれほど簡単ではない！


	というかdo_moveの処理とかぶってしまうのでこれは必要ないか？？
	do_moveをした時に局面に王手がかかっていれば差し手を延長するみたいな感じでいいのでは？？
	しかしgivescheckで枝切りなどを考えるときdo_moveをしてから枝切りを考えると、do_moveには時間がかかるし局面を戻す操作もしてから枝切りをしなければならないので
	先にgivescheckを見て、そしてその値をdo_moveに用いた方がいいのかもしれない....


	do_moveをする前にgivescheckを確認しておいてそこで差し手を延長するかどうか判断する。
	do_move時にはgivescheckで確認した情報からこの指し手がgivescheckかどうかの情報を持ってくる。

	*/
	bool is_gives_check(const Move m)const {

		return is_effect_to_move(sidetomove(),ksq(opposite(sidetomove_)),m);

	}//end of is_givescheck

	 /*
	 与えられた差し手で局面を動かした後のhashkeyを計算する、
	 これはprefetchに必要になる

	 prefetchはできるだけ早くしておく必要があるのか,,,先に局面を動かしたhashkeyを用意してまで!!!
	 */
	Key key_after_move(const Move m);



	//ハフマン符号化
	string pack_haffman_sfen();
	string	unpack_haffman_sfen(bool *sfen);

	//一手詰
	Move mate1ply();
	bool cancapture_checkpiece(Square to);
	void slider_blockers(const Color stm, const Square s, Bitboard& dc_candicate, Bitboard& pinned) const;


	string random_startpos();
};

std::ostream& operator<<(std::ostream& os, const Position& pos);
