#include "makemove.h"

using namespace std;


//指し手の移動の生成関数は駒種毎に特殊化して駒種の性質を考えながら高速化の工夫を図っていく。
/*==========================================================================================
そういえば歩の利きを求めるのは縦型bitboardであればビットシフトでできるんだった！
後で試す。
============================================================================================*/
template<Move_type mt>
ExtMove* make_move_PAWN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	Bitboard target2;
	//bool canpromotefrom=false;
	bool canpromoteto = false;

	//この処理をb[0]とb[1]で並列化させたい！！！！
	while (occ_us.isNot()) {
		//fromのループ

		Square sq = occ_us.pop();
		ASSERT(is_ok(sq));
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pc = add_color(PAWN, US));

		target2 = target&StepEffect[US][pt][sq];
		
		//fromとpcはここで一気にbitshiftしておける。そのほうが毎回シフトしなくていいので高速化できるはず。
		int from = sq << 7;
		int pc2 = pc << 17;

		while (target2.isNot()) {

			Square to = target2.pop();
			if (mt==Cap_Propawn) {

				//駒のcaptureと駒得(ここでまたなれるかどうか判定するのはかなり無駄だと思う)
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

				//なれる場合は必ず成る。（成らないことによるメリットはない）
				if (canpromoteto) {
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
//成りの場合とならずの場合で関数を分けた方がいいかもしれない。
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
		
		int obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bitしか必要ないのでintでいいか（uint8_tで十分か？？？？）
		target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int from = sq << 7;
		int pc2 = pc << 17;

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

//桂馬の移動の生成関数 多分これは遅い
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
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));

		target2 = target&StepEffect[US][pt][sq];

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
				//targetから移動できないbitboardを除いておくのが一番か（それではだめだ）
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankB)&& (sqtorank(to) != RankI) && (sqtorank(to) != RankH)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}


	}

	return movelist;
}


ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, SILVER);
	Bitboard target2;
	bool canpromote= false;

	while (occ_us.isNot())
	{

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		ASSERT(piece_type(pos.piece_on(sq)) == SILVER);
		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}
		target2 = target&StepEffect[US][SILVER][sq];
		int from = sq << 7;
		int pc2 = pc << 17;
		while (target2.isNot()) {

			Square to = target2.pop();
			if (canpromote == false)
				if((SquareBB[sq] & canPromoteBB[US]).isNot()) { canpromote = true;}

			if (canpromote) { movelist++->move = make_movepromote2(from, to, pc2);}
			movelist++->move = make_move2(from, to, pc2);//ならない指し手も生成しておく
				
		}
	}
	return movelist;
}

ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, BISHOP);
	Bitboard target2;
	Bitboard effect;
	bool canpromote = false;

	while (occ_us.isNot()) {


		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(BISHOP, US));

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}

		int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
		//cout << effect << endl;
		target2 = target&effect;
		
		while (target2.isNot()) {
			Square to = target2.pop();

			if (canpromote == false)
				if((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromote = true; }

			//成れるときはからなず成らせる
			if (canpromote) {
				movelist++->move = make_movepromote2(from, to, pc2);
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}

		}
	}
	return movelist;
}


ExtMove* make_move_ROOK(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, ROOK);
	Bitboard target2;
	Bitboard effect;
	bool canpromote = false;

	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pt == ROOK);

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}
		int from = sq << 7;
		int pc2 = pc << 17;

		int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		target2 = target&effect;
		while (target2.isNot()) {

			Square to = target2.pop();

			if (canpromote == false)
				if((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromote = true; }

			if (canpromote) {
				//なれる場合は必ず成る
				movelist++->move = make_movepromote2(from, to, pc2);
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}
		}
	}
	return movelist;
}

//GOLD相当の駒の移動の生成
ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	
	//こいつらは成れない


	//GOLD相当の奴はひとつのocc_ptにまとめておいた方がいいかも
	Bitboard occ_us = pos.occ_pt(US, GOLD)|pos.occ_pt(US,PRO_PAWN)|pos.occ_pt(US,PRO_LANCE)|pos.occ_pt(US,PRO_NIGHT)|pos.occ_pt(US,PRO_SILVER);
	Bitboard target2;
	//Bitboard effect;


	//この処理をb[0]とb[1]で並列化させたい！！！！
	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		
		int from = sq << 7;
		int pc2 = pc << 17;
		target2 = target&StepEffect[US][GOLD][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			movelist++->move = make_move2(from, to, pc2);
		}

	}
	return movelist;
}



//movelist配列の先頭ポインタを受け取って、最終ポインタを返すようにする。
//templateを使うことで条件分岐を減らして高速化
/*
これの実態を書かないとリンクエラーに成る！””””」
*/
//この関数でmovetypeを分岐させてること自体があまりよくないかもしれない
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

		//ここでcanpromoteを用意して差し手を本生成するときにもcanpromoteを確認するのはばかばかしい。
		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | (canPromoteBB[US] & ~pos.occ(US)) :
			(mt == Quiet) ? (~pos.occ_all()&ALLBB)&~canPromoteBB[US]:
			target_nonPAWN;

		//ここの並ぶ順番も考えた方がいいか？（あとでorderingするのでそこまでする必要はないか）
		movelist = make_move_PAWN<mt>(pos, target_PAWN, movelist);
		movelist = make_move_LANCE(pos, target_nonPAWN, movelist);
		movelist = make_move_KNIGHT(pos, target_nonPAWN, movelist);
		movelist = make_move_SILVER(pos, target_nonPAWN, movelist);
		movelist = make_move_BISHOP(pos, target_nonPAWN, movelist);
		movelist = make_move_ROOK(pos, target_nonPAWN, movelist);
		movelist = make_move_ASGOLD(pos, target_nonPAWN, movelist);

	}
	else {
		const Bitboard target_drop = ~pos.occ_all()&ALLBB;//ALLBBをマスクするのは番外にも１が立ってしまっている場所があるから

	}

	return movelist;
}

ExtMove * test_move_generation(const Position & pos, ExtMove * movelist)
{


	movelist = move_generation<Cap_Propawn>(pos, movelist);
	movelist = move_generation<Quiet>(pos, movelist);

	return movelist;
}




//ExtMove * move_generation<Cap_Propawn>(const Position& pos, ExtMove * movelist);