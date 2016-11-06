#include "makemove.h"

using namespace std;


//指し手の移動の生成関数は駒種毎に特殊化して駒種の性質を考えながら高速化の工夫を図っていく。
template<Move_type mt>
ExtMove* make_move_PAWN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	Bitboard target2;
	bool canpromotefrom=false;
	bool canpromoteto = false;

	while (occ_us.isNot()) {
		//fromのループ

		Square sq = occ_us.pop();
		ASSERT(is_ok(sq));
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pc = add_color(PAWN, US));

		target2 = target&StepEffect[c][pt][sq];
		if (mt==Cap_Propawn) {
			canpromotefrom = (SquareBB[sq] & canPromoteBB[US]).isNot();
		}
		//fromとpcはここで一気にbitshiftしておける。そのほうが毎回シフトしなくていいので高速化できるはず。
		int from = sq << 6;
		int pc2 = pc << 14;

		while (target2.isNot()) {

			Square to = target2.pop();
			if (mt==Cap_Propawn) {
				//駒のcaptureと駒得
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
				//なれる場合は必ず成る。（成らないことによるメリットはない）
				if (canpromoteto || canpromotefrom) {
					movelist++->move = make_movepromote2(from, to, pc2);
				}
				else {
					//rank A Iへのならずの移動は許されない
					if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
						movelist++->move = make_move2(from, to, pc2);
					}
				}

			}
			else {
				//rank A Iへのならずの移動は許されない
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}

//香車の移動の生成関数
template<Move_type mt>
ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, LANCE);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;
	//bool canpromotefrom = false;//ランスは前にしか進めないのでfromで成の判定する必要はない
	bool canpromoteto = false;
	
	while (occ_us.isNot()) {

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		//Piece pt = piece_type(pc);
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));
		
		int obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bitしか必要ないのでintでいいか（uint8_tで十分か？？？？）
		target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int from = sq << 6;
		int pc2 = pc << 14;

		while (target2.isNot())
		{
			Square to = target2.pop();
			ASSERT(is_ok(to));
			canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

			if (canpromoteto) {
				//なれるなら成る。
				movelist++->move = make_movepromote2(from, to, pc2);
				//黒版における3段目、白番における７番目にはならずで移動することを許す。（成らないことによるメリットがある）
				if (sqtorank(to) == Cango) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
			else {
				//成れない
				//rank A Iへのならずの移動は許されない
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}

//桂馬の移動の生成関数
template<Move_type mt>
ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, KNIGHT);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;//なれる場所であっても３段目への移動のならずであれば許す
	bool canpromoteto = false;//前にしか進めないのでtoだけでいい

	while (occ_us.isNot())
	{
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		int from = sq << 6;
		int pc2 = pc << 14;
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));

		target2 = target&StepEffect[c][pt][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			ASSERT(is_ok(to));
			canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

			if (canpromoteto) {
				//なれるなら成る。
				movelist++->move = make_movepromote2(from, to, pc2);
				//黒版における3段目、白番における７番目にはならずで移動することを許す。（成らないことによるメリットがある）
				if (sqtorank(to) == Cango) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
			else {
				//成れない
				//rank AB IHへのならずの移動は許されないここもうちょっとなんとか成らないか
				//targetから移動できないbitboardを除いておくのが一番か
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankB)&& (sqtorank(to) != RankI) && (sqtorank(to) != RankH)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}


	}


}











//movelist配列の先頭ポインタを受け取って、最終ポインタを返すようにする。
//templateを使うことで条件分岐を減らして高速化
/*
これの実態を書かないとリンクエラーに成る！””””」
*/
template<Move_type mt>
ExtMove * move_generation(const Position& pos, ExtMove * movelist)
{
	//EVERSIONは別の関数を用意する
	ASSERT(mt != Eversion);

	Color US = pos.sidetomove();
	Color ENEMY = opposite(US);


	if (mt != Drop) {

		const Bitboard target_nonPAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) :
			(mt == Quiet) ? ~pos.occ_all()&ALLBB ://このALLBBはいる？
			(mt == Recapture) ? SquareBB[move_to(pos.state()->lastmove)] :
			ALLBB;

		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | (canPromoteBB[US] & ~pos.occ(US)) :
			(mt == Quiet) ? (~pos.occ_all()&ALLBB)&~canPromoteBB[US];



	}
	else {
		const Bitboard target_drop = ~pos.occ_all()&ALLBB;//ALLBBをマスクするのは番外にも１が立ってしまっている場所があるから

	}


}