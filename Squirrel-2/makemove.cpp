#include "makemove.h"

using namespace std;

#define fastdrop
#define shiftpawn

#ifdef shiftpawn
template<Color US>
ExtMove* make_move_PAWN_bitshift(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	//縦型であれば歩の効きがあるマスはbitshiftで得られる
	Bitboard target2=(US==BLACK? (occ_us>>1):(occ_us<<1))&target;

	const Rank rank1_teban = (US == BLACK ? RankA : RankI);
	const Direction south_teban = (US == BLACK ? DOWN : UP);//効きのあるマスからその効きの原因を求めるために使う。
	const int pc2 = add_color(PAWN, US) << 17;
	bool canpromoteto = false;

	while (target2.isNot())
	{
		Square to = target2.pop();
		Square from = to + south_teban;
		int from2 = from << 7;

		//ifでmtまで分けるのは無駄だったか（targetですでに考慮されているので）(´･ω･｀)

		canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
		if (canpromoteto) {
			movelist++->move = make_movepromote2(from2, to, pc2);
		}
		else {
			movelist++->move = make_move2(from2, to, pc2);
		}
	}
	return movelist;
}

ExtMove* make_move_PAWN_bitshift(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	if (pos.sidetomove() == BLACK) {
		return make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
	}
	else {
		return make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
	}
}
#else
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
		ASSERT(pc == add_color(PAWN, US));

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
					//rank A Iへのならずの移動は許されない(この条件は必要ない)targetでそんなtarget入ってこない
					/*if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
						movelist++->move = make_move2(from, to, pc2);
					}*/
					movelist++->move = make_move2(from, to, pc2);
				}

			}
			else if(mt==Quiet) {
				//rank A Iへのならずの移動は許されない（この条件は必要ない targetでそんなtarget入ってこない）
				/*if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}*/
				movelist++->move = make_move2(from, to, pc2);
			}
			else if (mt == Eversion) {
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
				if (canpromoteto) {
					movelist++->move = make_movepromote2(from, to, pc2);
				}
				else {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}
#endif


//香車の移動の生成関数
//成りの場合とならずの場合で関数を分けた方がいいかもしれない。
template<Color US>
ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, LANCE);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;
	//bool canpromotefrom = false;//ランスは前にしか進めないのでfromで成の判定する必要はない
	bool canpromoteto = false;
	
	while (occ_us.isNot()) {

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		//Piece pt = piece_type(pc);
		ASSERT(pc == add_color(LANCE, US));
		ASSERT(is_ok(sq));
		
		//int obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bitしか必要ないのでintでいいか（uint8_tで十分か？？？？）
		//target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		target2 = target&LanceEffect[US][sq][obstacle_tate];
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

ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return 	pos.sidetomove() == BLACK ?  make_move_LANCE<BLACK>(pos, target, movelist) : make_move_LANCE<WHITE>(pos, target, movelist);
}

//桂馬の移動の生成関数 多分これは遅い
template<Color US>
ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

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
		ASSERT(pc == add_color(KNIGHT, US));
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

ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_KNIGHT<BLACK>(pos, target, movelist) : make_move_KNIGHT<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, SILVER);
	Bitboard target2;
	bool canpromotefrom= false;

	while (occ_us.isNot())
	{
		//必ずここでcanpromoteをfalseに戻しておく！
		canpromotefrom = false;
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		ASSERT(piece_type(pos.piece_on(sq)) == SILVER);
		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}
		target2 = target&StepEffect[US][SILVER][sq];
		int from = sq << 7;
		int pc2 = pc << 17;
		while (target2.isNot()) {
			bool canpromoteto = false;
			Square to = target2.pop();
			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}
			if (canpromotefrom||canpromoteto) { movelist++->move = make_movepromote2(from, to, pc2);}
			movelist++->move = make_move2(from, to, pc2);//ならない指し手も生成しておく
				
		}
	}
	return movelist;
}


ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_SILVER<BLACK>(pos, target, movelist) : make_move_SILVER<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, BISHOP);
	Bitboard target2;
	Bitboard effect;
	bool canpromotefrom = false;

	while (occ_us.isNot()) {
		//ここでfalseに戻さなければならない！
		canpromotefrom = false;

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(BISHOP, US));

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}

		/*int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;*/
		int obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
	//	cout << effect << endl;
		target2 = target&effect;
		
		while (target2.isNot()) {
			bool canpromoteto = false;
			Square to = target2.pop();

			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}
			//成れるときはからなず成らせる
			if (canpromotefrom==true||canpromoteto==true) {
				movelist++->move = make_movepromote2(from, to, pc2);
				//棋譜に不成が入ってい棋譜があったので学習中は生成してみたがやはりそんな局面学習する必要もないので生成やめ
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}

		}
	}
	return movelist;
}


ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_BISHOP<BLACK>(pos, target, movelist) : make_move_BISHOP<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_UNICORN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, UNICORN);
	Bitboard target2;
	Bitboard effect;
	

	while (occ_us.isNot()) {


		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(UNICORN, US));


		//int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		//int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		int obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)]|StepEffect[US][KING][sq];
		//cout << effect << endl;
		target2 = target&effect;

		while (target2.isNot()) {
			Square to = target2.pop();

			
				movelist++->move = make_move2(from, to, pc2);

		}
	}
	return movelist;
}


ExtMove* make_move_UNICORN(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_UNICORN<BLACK>(pos, target, movelist) : make_move_UNICORN<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_ROOK(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, ROOK);
	Bitboard target2;
	Bitboard effect;
	bool canpromotefrom = false;
	

	while (occ_us.isNot()) {
		//ここでfalseに戻さなければならない！
		canpromotefrom = false;
		

		const Square sq = occ_us.pop();
		const Piece pc = pos.piece_on(sq);
		const Piece pt = piece_type(pc);
		ASSERT(pt == ROOK);

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}
		int from = sq << 7;
		int pc2 = pc << 17;

		/*
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		
		*/

		/*int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;*/
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		//cout << effect << endl;
		target2 = target&effect;
		while (target2.isNot()) {
			bool canpromoteto = false;

			Square to = target2.pop();

			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}

			if (canpromotefrom==true||canpromoteto==true) {
				//なれる場合は必ず成る(棋譜に成らずが含まれていることがあったので学習時は成らずも生成する)
				ASSERT((SquareBB[sq] & canPromoteBB[US]).isNot() || (SquareBB[to] & canPromoteBB[US]).isNot());
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
	return (pos.sidetomove() == BLACK) ? make_move_ROOK<BLACK>(pos, target, movelist) : make_move_ROOK<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_DRAGON(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, DRAGON);
	Bitboard target2;
	Bitboard effect;
	//bool canpromote = false;

	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pt == DRAGON);

		
		int from = sq << 7;
		int pc2 = pc << 17;

		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		/*int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;*/

	/*	int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;*/
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[US][KING][sq];
		target2 = target&effect;
		while (target2.isNot()) {

			Square to = target2.pop();

			
			movelist++->move = make_move2(from, to, pc2);
		}
	}
	return movelist;
}


ExtMove* make_move_DRAGON(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_DRAGON<BLACK>(pos, target, movelist) : make_move_DRAGON<WHITE>(pos, target, movelist);
}

//GOLD相当の駒の移動の生成
template<Color US>
ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());
	
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


ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_ASGOLD<BLACK>(pos, target, movelist) : make_move_ASGOLD<WHITE>(pos, target, movelist);
}


//王の指し手生成（自殺手は生成してしまわないようにする）
template<Color US>
ExtMove* make_move_KING(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());
//	Color ENEMY = opposite(US);
	//こいつらは成れない


	//GOLD相当の奴はひとつのocc_ptにまとめておいた方がいいかも
	Bitboard occ_us = pos.occ_pt(US, KING);
	Bitboard target2;
	//Bitboard effect;


	//この処理をb[0]とb[1]で並列化させたい！！！！
	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);

		int from = sq << 7;
		int pc2 = pc << 17;
		target2 = target&StepEffect[US][KING][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			//王の移動先に相手の効きが効いていることはめったにないことなのでここで合法確認するのではなく後回しにする
			movelist++->move = make_move2(from, to, pc2);
			
		}

	}


	return movelist;
}


ExtMove* make_move_KING(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_KING<BLACK>(pos, target, movelist) : make_move_KING<WHITE>(pos, target, movelist);
}

#ifndef fastdrop


//駒打ち
ExtMove* make_move_DROP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	auto hands = pos.hand(US);
	Square to;
	Piece pc;
	Bitboard target2;

	if (have_hand(hands)) {

		if (num_pt(hands, PAWN)) {

			//target2 = target&~CantGo_PAWNLANCE[US]&~pos.pawnbb(US);
			// _mm_andnot_si128は１命令？？
			target2 = andnot(andnot(target, CantGo_PAWNLANCE[US]), pos.pawnbb(US));
			pc = add_color(PAWN, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				//ここで２歩が入ってくることはないと思うけれども一応確認しておく
				//ここで二歩がはいって来ているのはpawnbbがおかしいせいか （解決）
				/*if (pos.check_nihu(make_drop2(to, pc2)) == true) {
				cout << "nihu " << endl;
				cout << "target2"<<endl << target2 << endl;
				cout << make_drop2(to, pc2) << endl;
				cout << pos << endl;
				cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
				cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
				UNREACHABLE;
				}*/


				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, LANCE)) {
			target2 = andnot(target, CantGo_PAWNLANCE[US]);
			pc = add_color(LANCE, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, KNIGHT)) {
			target2 = andnot(target, CantGo_KNIGHT[US]);
			pc = add_color(KNIGHT, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		//ここ駒種別にforを回すのではなく一気に指し手生成して高速化できる。
		for (Piece pt = SILVER; pt < KING; pt++) {

			if (num_pt(hands, pt)) {

				target2 = target;
				pc = add_color(pt, US);
				int pc2 = pc << 17;
				while (target2.isNot()) {
					to = target2.pop();
					ASSERT(is_ok(to));
					movelist++->move = make_drop2(to, pc2);
				}
			}
		}




	}

	return movelist;
}

#endif // !fastdrop
#ifdef fastdrop
//Apery参考の駒うち
template<Color US>
ExtMove* make_move_DROP_fast(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	auto hands = pos.hand(US);
	Square to;
	Piece pc;
	Bitboard target2;

	if (have_hand(hands)) {

		//ここで2歩は防がれているがうち歩詰めはここでは見ていない。isleagalで見る。
		if (num_pt(hands, PAWN)) {
			pc = add_color(PAWN, US);
			int pc2 = pc << 17;
			target2 = andnot(andnot(target, CantGo_PAWNLANCE[US]), pos.pawnbb(US));
			foreachBB(target2, to, movelist++->move = make_drop2(to, pc2););
		}
		if (have_except_pawn(hands)) {
			Move Drop[6];
			int num = 0;
			if (have_pt(hands, KNIGHT)) {Drop[num++] = make_drop(SQ_ZERO, add_color(KNIGHT, US));}
			int no_knight_index = num;//桂馬がない場合
			if (have_pt(hands, LANCE)) {Drop[num++] = make_drop(SQ_ZERO, add_color(LANCE, US));}
			int no_lance_index = num;//香車がない場合
			if (have_pt(hands, SILVER)) { Drop[num++] = make_drop(SQ_ZERO, add_color(SILVER, US)); }
			if (have_pt(hands, GOLD)) { Drop[num++] = make_drop(SQ_ZERO, add_color(GOLD, US)); }
			if (have_pt(hands, BISHOP)) { Drop[num++] = make_drop(SQ_ZERO, add_color(BISHOP, US)); }
			if (have_pt(hands, ROOK)) { Drop[num++] = make_drop(SQ_ZERO, add_color(ROOK, US)); }

			if (no_lance_index == 0) {

				target2 = target;
				//香車と桂馬がないので味方のいないマスに打つ差し手を生成すればいい
				switch (num)
				{
					//0になることはあり得ない
				case 1:
					foreachBB(target2, to, { unroller1({movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 2:
					foreachBB(target2, to, { unroller2({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 3:
					foreachBB(target2, to, { unroller3({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 4:
					foreachBB(target2, to, { unroller4({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}
			}
			else {
				//それ以外のケース
				/*------------------------------------------------------
				それ以外のケースには
				香車　ある　ない
				桂馬　ある　ない
				その他　ある　ない
				の3つのケースがある。
				香車は　1段目に打てない
				桂馬は１～２段目に打てない。

				まずは一段目への駒うちだけを香車以外に対して生成
				2段目への駒うちだけを桂馬以外に対して生成。
				3段目以降はすべての駒種を生成する
				---------------------------------------------------------*/
				Bitboard rank1 = target& CantGo_PAWNLANCE[US];//1段目(9段目)
				Bitboard rank2 = target& (US==BLACK?RankBB[RankB]:RankBB[RankH]);//2段目（8段目）
				target2 = andnot(target, CantGo_KNIGHT[US]);//3~9
				/*cout << rank1 << endl;
				cout << rank2 << endl;
				cout << target2 << endl;*/
				switch (num-no_lance_index)
				{
				case 0:
					break;
				case 1:
					foreachBB(rank1, to, { unroller1({ movelist++->move = (Move)(Drop[i+no_lance_index] + to); }) });
					break;
				case 2:
					foreachBB(rank1, to, { unroller2({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				case 3:
					foreachBB(rank1, to, { unroller3({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				case 4:
					foreachBB(rank1, to, { unroller4({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}

				switch (num - no_knight_index)
				{
				case 0:
					break;
				case 1:
					foreachBB(rank2, to, { unroller1({ movelist++->move = (Move)(Drop[i+no_knight_index] + to); }) });
					break;
				case 2:
					foreachBB(rank2, to, { unroller2({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 3:
					foreachBB(rank2, to, { unroller3({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 4:
					foreachBB(rank2, to, { unroller4({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 5:
					foreachBB(rank2, to, { unroller5({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}

				switch (num)
				{
				case 1:
					foreachBB(target2, to, { unroller1({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 2:
					foreachBB(target2, to, { unroller2({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 3:
					foreachBB(target2, to, { unroller3({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 4:
					foreachBB(target2, to, { unroller4({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 5:
					foreachBB(target2, to, { unroller5({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 6:
					foreachBB(target2, to, { unroller6({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}
			}
		}


	}


	return movelist;
}

ExtMove* make_move_DROP_fast(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_DROP_fast<BLACK>(pos, target, movelist) : make_move_DROP_fast<WHITE>(pos, target, movelist);
}

#endif
/*

int8_t obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
int8_t obstacle_yoko = (occ256.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;

*/


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

	Color US = pos.sidetomove();//ここでUS用意してるってことはここでtemplate化できるやん
	Color ENEMY = opposite(US);


	if (mt != Drop) {

		const Bitboard target_nonPAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) :
			(mt == Quiet) ? ~pos.occ_all() :
			(mt == Recapture) ? SquareBB[move_to(pos.state()->lastmove)] :
			ALLBB;

		//ここでcanpromoteを用意して差し手を本生成するときにもcanpromoteを確認するのはばかばかしい。
		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | andnot(canPromoteBB[US],pos.occ(US)) :
			(mt == Quiet) ? andnot((~pos.occ_all()),canPromoteBB[US]):
			target_nonPAWN;

		//ここの並ぶ順番も考えた方がいいか？（あとでorderingするのでそこまでする必要はないか）
		if (US == BLACK) {
			movelist = make_move_PAWN_bitshift<BLACK>(pos, target_PAWN, movelist);
			movelist = make_move_LANCE<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_KNIGHT<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_SILVER<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_BISHOP<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_ROOK<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_ASGOLD<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_UNICORN<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_DRAGON<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_KING<BLACK>(pos, target_nonPAWN, movelist);
		}
		else {
			movelist = make_move_PAWN_bitshift<WHITE>(pos, target_PAWN, movelist);
			movelist = make_move_LANCE<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_KNIGHT<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_SILVER<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_BISHOP<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_ROOK<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_ASGOLD<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_UNICORN<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_DRAGON<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_KING<WHITE>(pos, target_nonPAWN, movelist);
		}
	}
	else {
		const Bitboard target_drop = ~pos.occ_all();//ALLBBをマスクするのは番外にも１が立ってしまっている場所があるから
		//
		if (US == BLACK) {
			movelist = make_move_DROP_fast<BLACK>(pos, target_drop, movelist);
		}
		else {
			movelist = make_move_DROP_fast<WHITE>(pos, target_drop, movelist);
		}
	}

	return movelist;
}

//王手をかけている駒の利きの聴いていないところに逃げる。
//上手く動いているかどうかは明日確認する今日はもう眠いのだ(-_-)zzz
ExtMove * move_eversion(const Position& pos, ExtMove * movelist) {

	ASSERT(pos.is_incheck());
	ASSERT(pos.state()->checker.isNot());
	/*王手をかけてきている駒を取る。
		２重王手の場合は逃げるしか無い
		pinごまは動かしてはいけない（コレはislegalで確認する）
		トビ効きの間に割って入らせる。
		王が効きから逃げる。
	*/
	/*
	実装方法
	checkersから王手をかけている駒の効きの場所が立っているbitboardの作成をする
	checkersのビット数を数える
	
	王が逃げる手の作成をする
	２重王手の場合は奥が逃げる手しか許されないためここで終わる。

	一重王手の場合はここから先
	王手をかけている駒を取る、betweenに割って入る指し手を生成する
	*/

	int num_checker = 0;
	const Color US = pos.sidetomove();
	const Square ksq = pos.ksq(US);
	Square esq;
	const Color ENEMY = opposite(US);
	Bitboard checkers = pos.state()->checker;
	Bitboard enemy_effected = ZeroBB;

	int from2 = ksq << 7;
	int king2 = add_color(KING, US)<<17;


	while (checkers.isNot()) {
		esq = checkers.pop();


		//王手がかかっていないのにincheckになってしまっている
		//if (pos.piece_on(esq) == NO_PIECE) { cout << pos << endl; ASSERT(0); };
		//if(piece_color(pos.piece_on(esq)) != ENEMY) { cout << pos << endl; ASSERT(0); }
		ASSERT(pos.piece_on(esq) != NO_PIECE);
		ASSERT(piece_color(pos.piece_on(esq)) == ENEMY);


		++num_checker;
		enemy_effected |= effectBB(pos, piece_type(pos.piece_on(esq)), ENEMY, esq);

	}

	ASSERT(num_checker);

	//王の逃げ場
	Bitboard cankingmove= andnot(andnot(step_effect(BLACK,KING, ksq),pos.occ(US)),enemy_effected);
	//玉が移動した先に効きがあるかどうかはislegalで調べる
	while (cankingmove.isNot()) {
		Square to = cankingmove.pop();
		movelist++->move = make_move2(from2, to, king2);
	}
	
	//２重王手は逃げるしか無い
	if (num_checker > 1) {
		return movelist;
	}

	//後は王手をかけている指し手を取るか王手に割って入るかbetweenBBを開区間で作っておいてよかった..
	Bitboard target_drop = BetweenBB[ksq][esq];
	Bitboard target = target_drop | SquareBB[esq];
	/*movelist = make_move_PAWN<Cap_Propawn>(pos, target, movelist);
	movelist = make_move_PAWN<Quiet>(pos, target, movelist);*/
	if (US == BLACK) {
		movelist = make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
		movelist = make_move_LANCE<BLACK>(pos, target, movelist);
		movelist = make_move_KNIGHT<BLACK>(pos, target, movelist);
		movelist = make_move_SILVER<BLACK>(pos, target, movelist);
		movelist = make_move_BISHOP<BLACK>(pos, target, movelist);
		movelist = make_move_ROOK<BLACK>(pos, target, movelist);
		movelist = make_move_ASGOLD<BLACK>(pos, target, movelist);
		movelist = make_move_UNICORN<BLACK>(pos, target, movelist);
		movelist = make_move_DRAGON<BLACK>(pos, target, movelist);
		//movelist = make_move_KING(pos, target, movelist);

		//dropを打てるのは駒と駒の間だけ！！
		//movelist = make_move_DROP(pos, target_drop, movelist);
		movelist = make_move_DROP_fast<BLACK>(pos, target_drop, movelist);
	}
	else {
		movelist = make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
		movelist = make_move_LANCE<WHITE>(pos, target, movelist);
		movelist = make_move_KNIGHT<WHITE>(pos, target, movelist);
		movelist = make_move_SILVER<WHITE>(pos, target, movelist);
		movelist = make_move_BISHOP<WHITE>(pos, target, movelist);
		movelist = make_move_ROOK<WHITE>(pos, target, movelist);
		movelist = make_move_ASGOLD<WHITE>(pos, target, movelist);
		movelist = make_move_UNICORN<WHITE>(pos, target, movelist);
		movelist = make_move_DRAGON<WHITE>(pos, target, movelist);
		//movelist = make_move_KING(pos, target, movelist);

		//dropを打てるのは駒と駒の間だけ！！
		//movelist = make_move_DROP(pos, target_drop, movelist);
		movelist = make_move_DROP_fast<WHITE>(pos, target_drop, movelist);

	}
	return movelist;

}

ExtMove * move_recapture(const Position & pos, ExtMove * movelist, Square recapsq)
{
	const Bitboard target = SquareBB[recapsq];

	const Color US = pos.sidetomove();
	if (US == BLACK) {
		movelist = make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
		movelist = make_move_LANCE<BLACK>(pos, target, movelist);
		movelist = make_move_KNIGHT<BLACK>(pos, target, movelist);
		movelist = make_move_SILVER<BLACK>(pos, target, movelist);
		movelist = make_move_BISHOP<BLACK>(pos, target, movelist);
		movelist = make_move_ROOK<BLACK>(pos, target, movelist);
		movelist = make_move_ASGOLD<BLACK>(pos, target, movelist);
		movelist = make_move_UNICORN<BLACK>(pos, target, movelist);
		movelist = make_move_DRAGON<BLACK>(pos, target, movelist);
		movelist = make_move_KING<BLACK>(pos, target, movelist);//これで相手の効きのある場所に移動してしまうことがあるがそれはislegalでしらべているはず
	}
	else {
		movelist = make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
		movelist = make_move_LANCE<WHITE>(pos, target, movelist);
		movelist = make_move_KNIGHT<WHITE>(pos, target, movelist);
		movelist = make_move_SILVER<WHITE>(pos, target, movelist);
		movelist = make_move_BISHOP<WHITE>(pos, target, movelist);
		movelist = make_move_ROOK<WHITE>(pos, target, movelist);
		movelist = make_move_ASGOLD<WHITE>(pos, target, movelist);
		movelist = make_move_UNICORN<WHITE>(pos, target, movelist);
		movelist = make_move_DRAGON<WHITE>(pos, target, movelist);
		movelist = make_move_KING<WHITE>(pos, target, movelist);//これで相手の効きのある場所に移動してしまうことがあるがそれはislegalでしらべているはず
	}
	return movelist;
}



ExtMove * test_move_generation(const Position & pos, ExtMove * movelist)
{
	if (pos.is_incheck()) {
		movelist = move_eversion(pos, movelist);
	}
	else {
		movelist = move_generation<Cap_Propawn>(pos, movelist);
		movelist = move_generation<Quiet>(pos, movelist);
		movelist = move_generation<Drop>(pos, movelist);
	}
	return movelist;
}
//
//ExtMove * test_drop_fast(const Position & pos, ExtMove * movelist)
//{
//	return nullptr;
//}


/*
Cap_Propawn,//ここは飛車の成を入れるとか、歩の成を省くとかlance nightのナリを含めるとか 色々考える必要がある
Quiet,
Eversion,
Recapture,
Drop,
*/

//ExtMove * move_generation<Cap_Propawn>(const Position& pos, ExtMove * movelist);
template ExtMove* move_generation<Cap_Propawn>(const Position& pos, ExtMove* movelist);
template ExtMove* move_generation<Quiet>(const Position& pos, ExtMove* movelist);
template ExtMove* move_generation<Drop>(const Position& pos, ExtMove* movelist);



#if 1
template<Piece pt>
ExtMove* make_checkdrop(const Position& pos,const Bitboard target, ExtMove * movelist) {

	const Color us = pos.sidetomove();
	auto hands = pos.hand(us);
	int pc2 = add_color(pt,us) << 17;
	Bitboard target2 = target;
	if (num_pt(hands, pt) != 0) {
		while (target2.isNot()) {
			Square to = target2.pop();
			movelist++->move = make_drop2(to, pc2);
		}
	}
	return movelist;
}




/*
静止探索で使うquietな王手生成

将棋の場合
１味方の飛び効きを遮っている駒をどかす
２王手できる範囲に駒を打つ
３王手できる範囲に駒を移動させる
の3つの王手の方法がある

ここで１と３には重複があるので１で3も生成しないようにしなければならない。
両王手を先に生成しておくべきだと思うので3から先に生成すべき？？



テスト局面
2重王手可能　position sfen lnsgkgsnl/1r5b1/pp2G1ppp/9/9/4RB3/PPPPPPPPP/9/LNSGK1SNL b P3p 1 OK
position sfen lnsgkgsnl/1r5b1/pp2GLppp/9/9/4RB3/PPPPP1PP1/9/LNSGK1SN1 b 3P3p 1 OK

dropは除いたりしたほうがいいかもしれない
*/


template<Color us,bool is_eksq_infield>
ExtMove * move_generation_quietcheck__(const Position & pos, ExtMove * movelist) {

	ASSERT(!pos.is_incheck());
	ASSERT(us == pos.sidetomove());
	
	//const Color us = pos.sidetomove();

	const Color enemy = opposite(us);
	const Square eksq = pos.ksq(opposite(us));
	//const bool is_eksq_infield=(SquareBB[eksq] & canPromoteBB[us]).isNot();//相手玉が敵陣にいるかどうか
	ASSERT(is_eksq_infield == (SquareBB[eksq] & canPromoteBB[us]).isNot());
	Bitboard apart_check_brocker = pos.state()->pinner[us];//多分pinner_usでOKのはず（名前の付け方がまずかった）これを動かすと関節王手になる

	//１味方の飛び効きを遮っている駒をどかす
	while (apart_check_brocker.isNot())
	{
		const Square from = apart_check_brocker.pop();
		const Piece pt = piece_type( pos.piece_on(from));
		const int from2 = from << 7;
		const int pc2 = pos.piece_on(from) << 17;
		if (pt == KING) { continue; }//king動かすのはやばそうなのでやめとく
		ASSERT(piece_color(pos.piece_on(from)) == us);
		//移動先（fromとeksqの直線上（この直線状には飛びゴマもある）だと空き王手にならない　直接王手は別で考えるので省く）

		Bitboard target;
		if (pt == PAWN) {
			target = andnot(StepEffect[us][PAWN][from], LineBB[from][eksq] | effectBB(pos.ret_occ_256(), pt, enemy, eksq)|pos.occ_all());
			target = andnot(target, canPromoteBB[us]);
		}
		else {
			target = andnot(effectBB(pos.ret_occ_256(), pt, us, from), LineBB[from][eksq] | effectBB(pos.ret_occ_256(), pt, enemy, eksq) | pos.occ_all());
		}
		while (target.isNot())
		{
			Square to = target.pop();
			movelist++->move = make_move2(from2, to, pc2);
		}

	}
	//２王手できる範囲に駒を打つ
	//３王手できる範囲に駒を移動させる

	//is_ksqIncheckはtemplate化できるな...
	Bitboard target;

	//歩
	target = andnot(StepEffect[enemy][PAWN][eksq], pos.occ_all() | canPromoteBB[us]);
	movelist=make_move_PAWN_bitshift(pos, target, movelist);

	//香
	target = andnot(lance_effect(pos.ret_occ_256(), enemy, eksq), pos.occ_all());
	movelist = make_checkdrop<LANCE>(pos, target, movelist);
	if (is_eksq_infield) { target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all()); }//ちゃんと作れてないが,まぁ大体で...
	movelist = make_move_LANCE(pos, target, movelist);

	//桂馬
	target = andnot(StepEffect[enemy][KNIGHT][eksq], pos.occ_all());
	movelist= make_checkdrop<KNIGHT>(pos, target, movelist);
	if (is_eksq_infield) {target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all());}//これ非効率的なんだが仕方ない
	movelist = make_move_KNIGHT(pos, target, movelist);

	//銀
	target = andnot(StepEffect[enemy][SILVER][eksq], pos.occ_all());
	movelist = make_checkdrop<SILVER>(pos, target, movelist);
	if (is_eksq_infield) { target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_SILVER(pos, target, movelist);

	//金
	target= andnot(StepEffect[enemy][GOLD][eksq], pos.occ_all());
	movelist = make_move_ASGOLD(pos, target, movelist);
	movelist=make_checkdrop<GOLD>(pos,target, movelist);

	//飛車
	target = andnot(rook_effect(pos.ret_occ_256(), eksq), pos.occ_all());
	movelist = make_checkdrop<ROOK>(pos,target, movelist);
	if (is_eksq_infield) {target = andnot(target|(StepEffect[us][KING][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_ROOK(pos, target, movelist);

	//角
	target = andnot(bishop_effect(pos.ret_occ_256(), eksq), pos.occ_all()); 
	movelist = make_checkdrop<BISHOP>(pos,target, movelist);
	if (is_eksq_infield) {target = andnot(target| (StepEffect[us][KING][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_BISHOP(pos, target, movelist);
	

	//竜
	target = andnot(rook_effect(pos.ret_occ_256(), eksq) | StepEffect[us][KING][eksq], pos.occ_all());
	movelist = make_move_DRAGON(pos, target, movelist);
	//ユニコーン
	target = andnot(bishop_effect(pos.ret_occ_256(), eksq) | StepEffect[us][KING][eksq], pos.occ_all());
	movelist = make_move_UNICORN(pos, target, movelist);

	return movelist;
}


template<Color us>
ExtMove * move_generation_quietcheck_(const Position & pos, ExtMove * movelist) {
	const Square eksq = pos.ksq(opposite(us));
	if ((SquareBB[eksq] & canPromoteBB[us]).isNot()) { return move_generation_quietcheck__<us, true>(pos, movelist); }
	else { return move_generation_quietcheck__<us, false>(pos, movelist); }
}


ExtMove * move_generation_quietcheck(const Position & pos, ExtMove * movelist) {
	if (pos.sidetomove() == BLACK) { return move_generation_quietcheck_<BLACK>(pos, movelist); }
	else { return move_generation_quietcheck_<WHITE>(pos, movelist); }
}



ExtMove * test_quietcheck(const Position & pos, ExtMove * movelist) {

	//const Color us = pos.sidetomove();
	//const Square eksq = pos.ksq(opposite(us));
	//const bool is_eksq_infield=(SquareBB[eksq] & canPromoteBB[us]).isNot();//相手玉が敵陣にいるかどうか

	movelist = move_generation_quietcheck(pos, movelist);

	return movelist;
}


#endif