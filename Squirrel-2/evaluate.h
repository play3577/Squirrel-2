#pragma once
#include "fundation.h"

#include <algorithm>
#include <unordered_map>



using namespace std;

struct Position;


namespace Eval {
	//あまり大きすぎるとパラメーターが変わりにくいのでbonanza6の値で合わせる
	const int32_t FV_SCALE = 32;

	//コマ割はやねうら王（やねうら王のコマ割はbonanza6）
	enum :int16_t {
		PawnValue = 86,
		LanceValue = 227,
		KnightValue = 256,
		SilverValue = 365,
		GoldValue = 439,
		BishopValue = 563,
		RookValue = 629,
		ProPawnValue = 540,
		ProLanceValue = 508,
		ProKnightValue = 517,
		ProSilverValue = 502,
		UnicornValue = 826,
		DragonValue = 942,
		KingValue = 15000,
	};

	extern int16_t piece_value[PC_ALL];
	//盤上の駒を取った場合、評価値は相手が駒を失った分＋自分が駒を手に入れた分変動する。
	//評価値は反転されるので符号はこれでいい
	extern int16_t capture_value[PC_ALL];
	//駒が成った分の差分
	extern int16_t diff_promote[GOLD];

	void komawari_check();

	//変数名はやねうら王に合わせた（やねうら王はbonanzaに合わせている）
	enum BonaPiece :/* int16_t*/int32_t
	{
		
		BONA_PIECE_ZERO = 0, 

		 // --- 手駒
		 f_hand_pawn = BONA_PIECE_ZERO + 1,
		 e_hand_pawn = f_hand_pawn + 18,
		 f_hand_lance = e_hand_pawn + 18,
		 e_hand_lance = f_hand_lance + 4,
		 f_hand_knight = e_hand_lance + 4,
		 e_hand_knight = f_hand_knight + 4,
		 f_hand_silver = e_hand_knight + 4,
		 e_hand_silver = f_hand_silver + 4,
		 f_hand_gold = e_hand_silver + 4,
		 e_hand_gold = f_hand_gold + 4,
		 f_hand_bishop = e_hand_gold + 4,
		 e_hand_bishop = f_hand_bishop + 2,
		 f_hand_rook = e_hand_bishop + 2,
		 e_hand_rook = f_hand_rook + 2,
		 fe_hand_end = e_hand_rook + 2,

		 f_pawn = fe_hand_end,
		 e_pawn = f_pawn + 81,
		 f_lance = e_pawn + 81,
		 e_lance = f_lance + 81,
		 f_knight = e_lance + 81,
		 e_knight = f_knight + 81,
		 f_silver = e_knight + 81,
		 e_silver = f_silver + 81,
		 f_gold = e_silver + 81,//これpro_pawnとかで分けたほうが正確になっていいか？？
		 e_gold = f_gold + 81,
		 f_bishop = e_gold + 81,
		 e_bishop = f_bishop + 81,
		 f_unicorn = e_bishop + 81,
		 e_unicorn = f_unicorn + 81,
		 f_rook = e_unicorn + 81,
		 e_rook = f_rook + 81,
		 f_dragon = e_rook + 81,
		 e_dragon = f_dragon + 81,
		 fe_end = e_dragon + 81,

		 f_king = fe_end,
		 e_king = f_king + 81,
		 fe_end2 = e_king + 81, 
		 
	};
	inline BonaPiece operator++(BonaPiece& d, int) { BonaPiece prev = d; d = BonaPiece(int(d) + 1); return prev; }

	constexpr BonaPiece bpindices[] = {
		f_hand_pawn,
		e_hand_pawn,
		f_hand_lance,
		e_hand_lance,
		f_hand_knight,
		e_hand_knight,
		f_hand_silver,
		e_hand_silver,
		f_hand_gold,
		e_hand_gold,
		f_hand_bishop,
		e_hand_bishop,
		f_hand_rook,
		e_hand_rook,
		//fe_hand_end = e_hand_rook + 2,
		f_pawn,
		e_pawn,
		f_lance,
		e_lance,
		f_knight,
		e_knight,
		f_silver,
		e_silver,
		f_gold,//これpro_pawnとかで分けたほうが正確になっていいか？？
		e_gold,
		f_bishop,
		e_bishop,
		f_unicorn,
		e_unicorn,
		f_rook,
		e_rook,
		f_dragon,
		e_dragon,
		//fe_end,
		f_king,
		e_king,
		//fe_end2 = e_king + 81,
	};

	class Bp2Piece :public std::unordered_map<BonaPiece, Piece> {

	public:
		Bp2Piece() {
			//先手から見た駒を返す。（後手から見たbpに対して使ってしまわないように注意）
			(*this)[f_hand_pawn] = B_PAWN;
			(*this)[e_hand_pawn] = W_PAWN;
			(*this)[f_hand_lance] = B_LANCE;
			(*this)[e_hand_lance] = W_LANCE;
			(*this)[f_hand_knight] = B_KNIGHT;
			(*this)[e_hand_knight] = W_KNIGHT;
			(*this)[f_hand_silver] = B_SILVER;
			(*this)[e_hand_silver] = W_SILVER;
			(*this)[f_hand_gold] = B_GOLD;
			(*this)[e_hand_gold] = W_GOLD;
			(*this)[f_hand_bishop] = B_BISHOP;
			(*this)[e_hand_bishop] = W_BISHOP;
			(*this)[f_hand_rook] = B_ROOK;
			(*this)[e_hand_rook] = W_ROOK;

			(*this)[f_pawn] = B_PAWN;
			(*this)[f_lance] = B_LANCE;
			(*this)[f_knight] = B_KNIGHT;
			(*this)[f_silver] = B_SILVER;
			(*this)[f_bishop] = B_BISHOP;
			(*this)[f_rook] = B_ROOK;
			(*this)[f_gold] = B_GOLD;
			(*this)[f_king] = B_KING;
			(*this)[f_unicorn] = B_UNICORN;
			(*this)[f_dragon] = B_DRAGON;

			(*this)[e_pawn] = W_PAWN;
			(*this)[e_lance] = W_LANCE;
			(*this)[e_knight] = W_KNIGHT;
			(*this)[e_silver] = W_SILVER;
			(*this)[e_bishop] = W_BISHOP;
			(*this)[e_rook] = W_ROOK;
			(*this)[e_gold] = W_GOLD;
			(*this)[e_king] = W_KING;
			(*this)[e_unicorn] = W_UNICORN;
			(*this)[e_dragon] = W_DRAGON;
		}

		bool is_ok(const BonaPiece psuedo_piece) const {
			return (this->find(psuedo_piece) != this->end());
		}

		Piece bp_to_piece(BonaPiece piece) const {
			if (is_ok(piece) == false) { return NO_PIECE; }//見つからなかった。
			return this->find(piece)->second;
		}

	};
	extern Bp2Piece bp2piece;


	//bonapieceからsq成分を抜き出す(Apery参考)
	inline Square bp2sq(const BonaPiece bp) {
		if (bp < f_pawn) { return Error_SQ; }
		//bpよりも大きな純粋bonapieceの配列要素を返す
		const auto bp_upper = upper_bound(begin(bpindices), end(bpindices), bp);
		return (Square)(bp - *(bp_upper - 1));
	}
	inline int bp2numhand(const BonaPiece bp) {

		if (bp >= f_pawn) { ASSERT(0); }
		//bpよりも大きな純粋bonapieceの配列要素を返す
		const auto bp_upper = upper_bound(begin(bpindices), end(bpindices), bp);
		return (int)(bp - *(bp_upper - 1));
	}
	//bpからsqを抜く
	inline BonaPiece bpwithoutsq(const BonaPiece bp) {
		const auto bp_upper = upper_bound(begin(bpindices), end(bpindices), bp);
		return *(bp_upper-1);
		//return *lower_bound(begin(bpindices), end(bpindices), bp);
	}
	//bpから色を取り出す
	inline Color bp2color(const BonaPiece bp) {
		const auto bp_upper = upper_bound(begin(bpindices), end(bpindices), bp);
		return ((bp_upper - 1 - begin(bpindices)) & 1) == 1 ? WHITE : BLACK;
	}

	inline BonaPiece inversebonapiece(const BonaPiece bp) {

		const Square sq = bp2sq(bp);

		const auto bp_upper = upper_bound(begin(bpindices), end(bpindices), bp);
		if (sq != Error_SQ) {
			const Square inverseSQ = hihumin_eye(sq);
			if (bp2color(bp) == BLACK) { return BonaPiece(*(bp_upper)+inverseSQ); }
			else { return  BonaPiece(*(bp_upper - 2) + inverseSQ); }
		}
		else {
			
			int num = bp2numhand(bp);
			if (bp2color(bp) == BLACK) { return BonaPiece(*(bp_upper)+num); }
			else { return  BonaPiece(*(bp_upper - 2) + num); }
		}
	}



	//駒落ちに対応するためにBP_ZEROを許す
	inline bool is_ok(BonaPiece bp) { return (BONA_PIECE_ZERO <= bp&&bp < fe_end2); }
	//inline std::ostream& operator<<(std::ostream& os, const BonaPiece bp);
	//駒の背番号
	enum UniformNumber :int8_t{
		//no_uniform,
		//成ったとしても背番号はなる前のものでいいと考えられる
		pawn1, pawn2, pawn3, pawn4, pawn5, pawn6, pawn7, pawn8, pawn9, pawn10, pawn11, pawn12, pawn13, pawn14, pawn15, pawn16, pawn17, pawn18,
		lance1, lance2,lance3,lance4,
		knight1, knight2,knight3,knight4,
		silver1,silver2,silver3,silver4,
		bishop1,bishop2,
		rook1,rook2,
		gold1,gold2,gold3,gold4,
		king1, king2,
		Num_Uniform,
	};
	inline UniformNumber& operator++(UniformNumber& d) { return d = UniformNumber(int(d) + 1); }
	inline UniformNumber operator++(UniformNumber& d, int) { UniformNumber prev = d; d = UniformNumber(int(d) + 1); return prev; }

	inline bool is_ok(UniformNumber un) { return (pawn1 <= un&&un <= Num_Uniform); }

	//黒番から見たbonapieceを返す。
	BonaPiece bonapiece(const Square sq, const Piece pc);
	//黒番から見たbonapieceを返す。
	BonaPiece bonapiece(const Color c, const Piece pt, const int num);


	//bonapieceを追跡するためのリスト。これを用いれば毎回make listする必要がなくなる。
	//しかしこれ自分で実装できるだろうか....(どうぶつしょうぎで出来たので本将棋でもできるっしょ(´･_･`))
	//本将棋では駒は40枚
	struct BonaPList {

		//ppはかぶらないように計算されるのでｆとeの場所が入れ違っても大丈夫だと考えられる。
		BonaPiece bplist_fb[Num_Uniform];
		BonaPiece bplist_fw[Num_Uniform];
		//sqにいる駒のUniformnumverを返す。この方法には無駄が多いようなきがするけどコレしか思いつかなかったのでしゃーない
		UniformNumber sq2Uniform[SQ_NUM];
		// 手番駒種枚数に対応するUniformNumberを返す。
		//駒を打つときは枚数が多い方のUniformNumberから使っていく。(１８まで用意するのホントに無駄だなぁ)（もっとうまくいく方法を思いつけばそれで実装する）
		UniformNumber hand2Uniform[ColorALL][KING][18];

		void init() {
			//コレで初期化出来ているはず
			memset(bplist_fb, 0, sizeof(bplist_fb));
			memset(bplist_fw, 0, sizeof(bplist_fw));
			//memset(sq2Uniform, 0, sizeof(sq2Uniform));
			//memset(hand2Uniform, 0, sizeof(hand2Uniform));
			for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
				sq2Uniform[sq] = Num_Uniform;
			}
			for (Color c = BLACK; c < ColorALL; c++) {
				for (Piece pt = NO_PIECE; pt < KING; pt++) {
					for (int num = 0; num < 3; num++) {
						hand2Uniform[c][pt][num] = Num_Uniform;
					}
				}
			}
		}

		void makebonaPlist(const Position &pos);

		//bplistの内容を表示
		void print_bplist();
		void list_check();
	};


	//コマ割全計算
	Value eval_material(const Position& pos);

	//評価値計算
	Value eval(const Position& pos);

	//bonapieceの左右を反転させる関数
	BonaPiece sym_rightleft(const BonaPiece bp);

#ifdef EVAL_PP
	Value eval_PP(const Position& pos);

	Value eval_diff_PP(const Position& pos);


	Value eval_allPP_AVX2(const Position& pos);

	//２駒関係(32bitの精度で持っておいたほうが強くなると思う)
	extern int32_t PP[fe_end2][fe_end2];



	void read_PP();
	void write_PP();

	void param_sym_ij();



	inline void init() { read_PP(); }
#endif

#ifdef  EVAL_KPP

	const int FV_SCALE_KKP = 512;

	extern int16_t kpp[82][fe_end][fe_end];
	extern int32_t kkp[82][82][fe_end + 1];

	Value eval_KPP(const Position& pos);
	Value eval_diff_KPP(const Position& pos);

	void read_KPP();
	void write_KPP();



	inline void init() { read_KPP(); }

	void param_sym_ij();
#endif //  EVAL_KPP

	constexpr Value tempo = Value(40);
};
