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
やっぱ利きテーブルを持つの差分計算とか簡単じゃないのでやめる
wordboardとか理解できるようになったなら持ってもいいかもしれない
*/

struct StateInfo
{
	int ply_from_root = 0;
	Square ksq_[ColorALL];
	//Effect Eboard[ColorALL][SQ_NUM];//升にいくつの効きが聞いているかを保持するためのboard
	//---------------ここまでdomoveでコピー

	Move lastmove = MOVE_NONE;
	Piece DirtyPiece[2];//do_moveによって動いた駒0 取られた駒１
	bool inCheck = false;

	Bitboard checker = ZeroBB;//将棋では2重王手になることがあるためbitboardでもたねばならない。

	Value material;
	Value pp;

	StateInfo* previous = nullptr;//undo_moveで以前の局面に戻る為の単方向リスト
};


struct Position 
{
private:

	Bitboard occupied[ColorALL];//colorALL=2なので手番関係ないBitboardは用意していない。
	Bitboard occupied90;
	Bitboard occupied_plus45, occupied_minus45;

	Bitboard occupiedPt[ColorALL][PT_ALL];

	//歩の存在する升が1になっているbitboardコレをandnotすることで歩打ちのtargetを絞れる（２歩判定できる）
	Bitboard ExistPawnBB[ColorALL];
	Piece pcboard[SQ_NUM];

	StateInfo* st;
	StateInfo initSt;//st*が最初に指しておくStateinfo
	Color sidetomove_;
	Hand hands[ColorALL];

	uint64_t nodes;

public:

	Position() { set_hirate(); }
	void set(std::string sfen);
	void set_hirate() { set(sfen_hirate); }//平手にセットはOK

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

	//putもremoveもxorなのでやっていることは同じだが名前ぐらい変えておいたほうがいいだろ
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





	//c側の効きがtoに効いているかどうか調べる為の関数。
	/*
	調べ方は
	駒種毎にtoに相手の色の駒を置いてみたときの効きの範囲にその駒種が存在すればそれはそのマスに利きが効いているということに成る
	*/
	Bitboard effect_toBB(const Color US, const Square to) {

		Color ENEMY = opposite(US);

		/*
		盤上にUNICORN,DRAGONがいるかどうかも確認したほうが素早く処理できるか？？？
		*/
		return
			(step_effect(ENEMY, PAWN, to)&occ_pt(US, PAWN))
			| (long_effect(*this, ENEMY, LANCE, to)&occ_pt(US, LANCE))//利きを求めるにはoccupied が複数いるのでposを渡したほうが安全か？（あとでposを渡すときとoccをそれぞれ渡すときとで速さを調べる。）
			| (step_effect(ENEMY, KNIGHT, to)&occ_pt(US, KNIGHT))
			| (step_effect(ENEMY, SILVER, to)&occ_pt(US, SILVER))
			| (step_effect(ENEMY, GOLD, to)&(occ_pt(US, GOLD) | occ_pt(US, PRO_PAWN) | occ_pt(US, PRO_LANCE) | occ_pt(US, PRO_NIGHT) | occ_pt(US, PRO_SILVER)))
			| (step_effect(ENEMY, KING, to)&occ_pt(US, KING))
			| (long_effect(*this, ENEMY, ROOK, to)&occ_pt(US, ROOK))
			| (long_effect(*this, ENEMY, BISHOP, to)&occ_pt(US, BISHOP))
			| (long_effect(*this, ENEMY, DRAGON, to)&occ_pt(US, DRAGON))
			| (long_effect(*this, ENEMY, UNICORN, to)&occ_pt(US, UNICORN));
	}

	//c側の効きがtoに効いているかどうか調べる為の関数。
	//利きを求めるにはoccupied が複数いるのでposを渡したほうが安全か？（あとでposを渡すときとoccをそれぞれ渡すときとで速さを調べる。）
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
	これでは遅いよね.....
	もっとスマートな処理が欲しい
	*/
	bool is_uchihu(const Color us, const Square pawnsq) {

		Square frompawn;
		Color ENEMY = opposite(us);
		us == BLACK ? frompawn = Square(-1) : frompawn = Square(1);

		//歩の前がKINGでなければそれは打ち歩詰めではない。
		if (piece_on(pawnsq + frompawn) != KING) { return false; }

		//王の機器は先手でも後手でも同じでも同じ
		//逃げられる場所は王が動ける範囲で自分の駒がいない場所
		Bitboard escape = StepEffect[BLACK][KING][pawnsq + frompawn] &~ occ(ENEMY);
		while (escape.isNot()) {

			Square to = escape.pop();
			if (is_effect_to(ENEMY, to)) { return true; }
		}

		return false;
	}


	//toに相手の効きが効いていればそれは自殺手
	bool is_king_suiside(const Color us, const Square kingto) {

		Color ENEMY = opposite(us);
		if (is_effect_to(ENEMY, kingto)) { return true; }
		return false;

	}

	//この関数で打ち歩詰め、王の自殺手を省く。
	/*
	指し手が省かれる確率は非常に低いため
	指し手のloopでこの関数を呼び出すのは後ろの方でいいかもしれない（Stockfish方式）
	＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝PINされている駒を動かさないようにする処理もいる！！！！！！！！
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

		//pinされている駒を動かさないようにする



		//動かす駒は自分の駒
		ASSERT(piece_color(movedpiece) == c);
		//fromにいる駒と動かそうとしている駒は同じ
		ASSERT(!isDrop&&piece_on(move_from(m)) == movedpiece);
		//取ろうとしている駒は自分の駒ではない
		ASSERT(piece_on(move_to(m)) == NO_PIECE || piece_color(piece_on(move_to(m))) != c);

		return true;
	}


};

std::ostream& operator<<(std::ostream& os, const Position& pos);
