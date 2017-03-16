

#include "evaluate.h"
#include "position.h"

#include <utility>




namespace Eval {

	Bp2Piece bp2piece;

	//三角テーブルにすべきか....??
	ALIGNED(32) int32_t PP[fe_end2][fe_end2];

#ifdef EVAL_PROG
	ALIGNED(32) int32_t PP_F[fe_end2][fe_end2];
#endif




	int16_t piece_value[PC_ALL] = {
		Value_Zero,

		PawnValue,
		LanceValue,
		KnightValue,
		SilverValue,
		BishopValue,
		RookValue,
		GoldValue,
		KingValue,
		ProPawnValue,
		ProLanceValue,
		ProKnightValue,
		ProSilverValue,
		UnicornValue,
		DragonValue,
		0,0,

		-PawnValue,
		-LanceValue,
		-KnightValue,
		-SilverValue,
		-BishopValue,
		-RookValue,
		-GoldValue,
		-KingValue,
		-ProPawnValue,
		-ProLanceValue,
		-ProKnightValue,
		-ProSilverValue,
		-UnicornValue,
		-DragonValue,
	};

	int16_t capture_value[PC_ALL] = {
		Value_Zero,

		2*PawnValue,
		2*LanceValue,
		2*KnightValue,
		2*SilverValue,
		2*BishopValue,
		2*RookValue,
		2*GoldValue,
		2*KingValue,
		ProPawnValue+PawnValue,
		ProLanceValue+LanceValue,
		ProKnightValue+KnightValue,
		ProSilverValue+SilverValue,
		UnicornValue+BishopValue,
		DragonValue+RookValue,
		0,0,
		2 * PawnValue,
		2 * LanceValue,
		2 * KnightValue,
		2 * SilverValue,
		2 * BishopValue,
		2 * RookValue,
		2 * GoldValue,
		2 * KingValue,
		ProPawnValue + PawnValue,
		ProLanceValue + LanceValue,
		ProKnightValue + KnightValue,
		ProSilverValue + SilverValue,
		UnicornValue + BishopValue,
		DragonValue + RookValue,
	};

	int16_t diff_promote[GOLD] = {
		0,
		ProPawnValue-PawnValue,
		ProLanceValue-LanceValue,
		ProKnightValue-KnightValue,
		ProSilverValue-SilverValue,
		UnicornValue-BishopValue,
		DragonValue-RookValue,
	};
	
	


	

	//黒番から見たbonapieceを返す。
	BonaPiece bonapiece(const Square sq, const Piece pc)
	{
		switch (pc)
		{
		case B_PAWN:
			return BonaPiece(f_pawn + sq);
			break;
		case B_LANCE:
			return BonaPiece(f_lance+sq);
			break;
		case B_KNIGHT:
			return BonaPiece(f_knight+sq);
			break;
		case B_SILVER:
			return BonaPiece(f_silver+sq);
			break;
		case B_BISHOP:
			return BonaPiece(f_bishop+sq);
			break;
		case B_ROOK:
			return BonaPiece(f_rook+sq);
			break;
		case B_GOLD:
			return BonaPiece(f_gold+sq);
			break;
		case B_KING:
			return BonaPiece(f_king+sq);
			break;
		case B_PRO_PAWN:
		case B_PRO_LANCE:
		case B_PRO_NIGHT:
		case B_PRO_SILVER:
			return BonaPiece(f_gold+sq);
			break;
		case B_UNICORN:
			return BonaPiece(f_unicorn+sq);
			break;
		case B_DRAGON:
			return BonaPiece(f_dragon+sq);
			break;
		case W_PAWN:
			return BonaPiece(e_pawn+sq);
			break;
		case W_LANCE:
			return BonaPiece(e_lance+sq);
			break;
		case W_KNIGHT:
			return BonaPiece(e_knight+sq);
			break;
		case W_SILVER:
			return BonaPiece(e_silver+sq);
			break;
		case W_BISHOP:
			return BonaPiece(e_bishop+sq);
			break;
		case W_ROOK:
			return BonaPiece(e_rook+sq);
			break;
		case W_GOLD:
			return BonaPiece(e_gold+sq);
			break;
		case W_KING:
			return BonaPiece(e_king+sq);
			break;
		case W_PRO_PAWN:
		case W_PRO_LANCE:
		case W_PRO_NIGHT:
		case W_PRO_SILVER:
			return BonaPiece(e_gold+sq);
			break;
		case W_UNICORN:
			return BonaPiece(e_unicorn+sq);
			break;
		case W_DRAGON:
			return BonaPiece(e_dragon+sq);
			break;
		default:
			UNREACHABLE;
			return BONA_PIECE_ZERO;
			break;
		}
	}

	BonaPiece bonapiece(const Color c, const Piece pt, const int num)
	{
		Piece pc = add_color(pt, c);

		switch (pc)
		{
		case B_PAWN:
			return BonaPiece(f_hand_pawn + (num-1));
			break;
		case B_LANCE:
			return BonaPiece(f_hand_lance + (num-1));
			break;
		case B_KNIGHT:
			return BonaPiece(f_hand_knight + (num-1));
			break;
		case B_SILVER:
			return BonaPiece(f_hand_silver + (num-1));
			break;
		case B_BISHOP:
			return BonaPiece(f_hand_bishop + (num-1));
			break;
		case B_ROOK:
			return BonaPiece(f_hand_rook + (num-1));
			break;
		case B_GOLD:
			return BonaPiece(f_hand_gold + (num-1));
			break;

		case W_PAWN:
			return BonaPiece(e_hand_pawn + (num-1));
			break;
		case W_LANCE:
			return BonaPiece(e_hand_lance + (num-1));
			break;
		case W_KNIGHT:
			return BonaPiece(e_hand_knight + (num-1));
			break;
		case W_SILVER:
			return BonaPiece(e_hand_silver + (num-1));
			break;
		case W_BISHOP:
			return BonaPiece(e_hand_bishop + (num-1));
			break;
		case W_ROOK:
			return BonaPiece(e_hand_rook + (num-1));
			break;
		case W_GOLD:
			return BonaPiece(e_hand_gold + (num-1));
			break;
		default:
			UNREACHABLE;
			return BONA_PIECE_ZERO;
			break;
		}
	}


//#define DIFFTEST
	Value eval_material(const Position & pos)
	{
		int16_t v = 0;

		//盤上
		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
			v += piece_value[pos.piece_on(sq)];
		}

		//手駒
		for (Color c = BLACK; c <= WHITE; c++) {


			const auto hands = pos.hand(c);

			for (Piece pt = PAWN; pt <= GOLD; pt++) {
				v += (c == BLACK ? 1:-1)*num_pt(hands, pt)*piece_value[pt];
			}

		}
		return Value(v);
	}
#ifdef EVAL_PP
	Value eval(const Position & pos)
	{
		Value pp,value;
		Value material= pos.state()->material;

#ifndef EVAL_NONDIFF
		//計算済み
		if (pos.state()->bpp != Value_error&&pos.state()->wpp != Value_error) {
			ASSERT(pos.state()->wpp != Value_error);
			pp=Value((pos.state()->bpp + pos.state()->wpp) / FV_SCALE);

#ifdef DIFFTEST
		


			//計算済みの値が正しいか確認
			int bPP = pos.state()->bpp, wPP = pos.state()->wpp;

			if (pp != eval_PP(pos)) {
				cout << " diff " << pp << " evalfull " << eval_PP(pos) << endl;

				cout << pos << endl;
					/*cout << "oldlist" << endl;
					for (int i = 0; i < Num_Uniform; i++) {
					cout <<"fb:"<< old_list_fb[i];
					cout << "fw:" << old_list_fw[i] << endl;
					}*/
				cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;;
				cout << "bpp " << bPP << " " << pos.state()->bpp << endl;;
				cout << "wpp " << wPP << " " << pos.state()->wpp << endl;;
				UNREACHABLE;;
			}
#endif
		}
		else if (pos.state()->previous!=nullptr&&pos.state()->previous->bpp != Value_error&&pos.state()->previous->wpp != Value_error) {
			//差分計算可能
			ASSERT(pos.state()->previous->wpp != Value_error);
			pp = eval_diff_PP(pos);

		}
		else {
			//全計算！
			pp = eval_PP(pos);

		
		}
#else
		pp = eval_PP(pos);
		//ASSERT(material == eval_material(pos));
#endif
		/*if (pp != eval_PP(pos)) {
			cout << " diff " << pp << " evalfull " << eval_PP(pos) << endl;
			UNREACHABLE;
		}*/
		//pp = eval_PP(pos);

		value = material + pp;

		//コレは確認済み
		//Value  full = eval_material(pos);
		////駒得の差分計算ができているかチェック
		//if (value != full) {
		//	cout << pos << endl;
		//	ASSERT(0);
		//}


		return (pos.sidetomove() == BLACK) ? value : -value;

		/*Value value = eval_material(pos);
		return (pos.sidetomove() == BLACK) ? value : -value;*/
	}

	Value eval_PP(const Position & pos)
	{
		int32_t bPP=0, wPP=0;
		BonaPiece *list_fb=pos.evallist().bplist_fb, *list_fw=pos.evallist().bplist_fw;

		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				//listの内容の順番はどうでもいいと思うのだけれど...

				//この方法では見てはいけないメモリを見てしまっているのでセグフォが起こる！　
#if defined(_MSC_VER)
				bPP += PP[list_fb[i]][list_fb[j]];
				wPP -= PP[list_fw[i]][list_fw[j]];
#endif
#if defined(__GNUC__) 
				bPP += PP[pos.evallist().bplist_fb[i]][pos.evallist().bplist_fb[j]];
				wPP -= PP[pos.evallist().bplist_fw[i]][pos.evallist().bplist_fw[j]];
#endif

				
			}
		}
		pos.state()->bpp = (bPP);
		pos.state()->wpp = wPP;
		//評価値ホントに16bitで収まるかどうか確認
		ASSERT(abs(bPP + wPP) / FV_SCALE < INT16_MAX);

		return Value((bPP+wPP) / FV_SCALE);
	}

	//差分計算
	Value eval_diff_PP(const Position & pos)
	{
		//cout << "evaldiff" << endl;
		const StateInfo *now = pos.state();
		const StateInfo *prev = pos.state()->previous;
		const auto list = pos.evallist();
		int32_t bPP, wPP;
		bPP = prev->bpp;
		wPP = prev->wpp;

		ASSERT(bPP != Value_error&&wPP != Value_error);

	
		//dirtybonaPがゼロになっている！
		//dirtyuniformもゼロになってる！
		//dirty uniformがおかしいのでdo_moveでおかしくなってないか確認する。
		const Eval::BonaPiece oldbp1_fb = now->dirtybonap_fb[0];
		const Eval::BonaPiece oldbp2_fb = now->dirtybonap_fb[1];
		const Eval::BonaPiece oldbp1_fw = now->dirtybonap_fw[0];
		const Eval::BonaPiece oldbp2_fw = now->dirtybonap_fw[1];
		//swapする可能性があるためここはconstにしない
		Eval::UniformNumber moveduniform1 = now->dirtyuniform[0];
		Eval::UniformNumber moveduniform2 = now->dirtyuniform[1];
		
		


		const Eval::BonaPiece* now_list_fb = list.bplist_fb;
		const Eval::BonaPiece* now_list_fw = list.bplist_fw;

		const Eval::BonaPiece newbp1_fb = list.bplist_fb[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fb = list.bplist_fb[now->dirtyuniform[1]];
		const Eval::BonaPiece newbp1_fw = list.bplist_fw[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fw = list.bplist_fw[now->dirtyuniform[1]];


#if 1
#define ADD_PP(oldbp,oldwp,newbp,newwp){ \
		bPP -= PP[oldbp][now_list_fb[i]];\
		bPP += PP[newbp][now_list_fb[i]];\
		wPP += PP[oldwp][now_list_fw[i]];\
		wPP -= PP[newwp][now_list_fw[i]];\
}

		int i;

		
		if (moveduniform2 == Num_Uniform) {
			//駒が①つ動いた

			//このようにして②つに分けることでPP[old1][new1]を引いてしまうのを防ぎ,PP[new][new]を足してしまうのを防ぐ。(賢い...さすがはやねうら王...)
			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}

		}
		else {
			//駒が②つ動いた

			//pawn1<=dirty<dirty2<fe_end2　にしておく
			if (moveduniform1 > moveduniform2) { std::swap(moveduniform1, moveduniform2); }

			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//コレで[old1][old2]回避
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < moveduniform2; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//コレで[old1][old2] [new1][new2]回避
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			//ここで回避してしまった[old1][old2] [new1][new2]を補正する。
			bPP -= PP[oldbp1_fb][oldbp2_fb];
			bPP += PP[newbp1_fb][newbp2_fb];
			wPP += PP[oldbp1_fw][oldbp2_fw];
			wPP -= PP[newbp1_fw][newbp2_fw];

		}


#endif




		pos.state()->bpp = bPP;
		pos.state()->wpp = wPP;
#ifdef DIFFTEST


		/*
		diffの値は全計算と異なってしまうバグが発生した。
		これは PP[i][j]==PP[j][i]でないため発生してしまったバグであると考えられる。
		dj[i][j]=dJ[j][i]ではあるため、値としては近い値になっているのでそこまで大きなエラーではない。
		これは
		PP[i][j]=PP[j][i]=(PP[i][j]+PP[j][i])/2とすることで対応しようかな....
		*/



		//nullmove時に差分計算がおかしくなる！！！
		eval_PP(pos);
		//if (Value((bPP + wPP) / FV_SCALE) != eval_PP(pos)) {
		if (bPP!=pos.state()->bpp||wPP!=pos.state()->wpp) {

			cout << pos << endl;
			/*cout << "oldlist" << endl;
			for (int i = 0; i < Num_Uniform; i++) {
				cout <<"fb:"<< old_list_fb[i];
				cout << "fw:" << old_list_fw[i] << endl;
			}*/
			cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;
			cout << "bpp " << bPP << " " << pos.state()->bpp << endl;
			cout << "wpp " << wPP << " " << pos.state()->wpp << endl;
			UNREACHABLE;
		}
#endif
		ASSERT(((bPP + wPP) / FV_SCALE) < INT16_MAX);
		return Value((bPP + wPP) / FV_SCALE);

	}
#endif;
	//差分計算

	void BonaPList::makebonaPlist(const Position & pos)
	{
		//初期化
		init();

		//駒番号の為のカウンタ(グローバルではなく関数内に置くことでこの関数に来る度に値が初期化されるはず)
		UniformNumber uniform[9] = {
			Num_Uniform,
			pawn1,//pawn1を0にしないとリスト[0]に空きが出てしまう！！
			lance1,
			knight1,
			silver1,
			bishop1,
			rook1,
			gold1,
			king1,
		};

		//盤上
		Bitboard occ = pos.occ_all() ;

		while (occ.isNot()) {

			Square sq = occ.pop();
			Piece pc = pos.piece_on(sq);
			Piece pt = piece_type(pc);
			//Color c = piece_color(pc);

			switch (pt)
			{
			case PAWN:case PRO_PAWN:
				bplist_fb[uniform[PAWN]] = bonapiece(sq, pc);
				bplist_fw[uniform[PAWN]] = bonapiece(hihumin_eye(sq),inverse(pc));
				sq2Uniform[sq] = uniform[PAWN];
				uniform[PAWN]++;
				break;
			case LANCE:case PRO_LANCE:
				bplist_fb[uniform[LANCE]] = bonapiece(sq, pc);
				bplist_fw[uniform[LANCE]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[LANCE];
				uniform[LANCE]++;
				break;
			case KNIGHT:case PRO_NIGHT:
				bplist_fb[uniform[KNIGHT]] = bonapiece(sq, pc);
				bplist_fw[uniform[KNIGHT]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[KNIGHT];
				uniform[KNIGHT]++;
				break;
			case SILVER:case PRO_SILVER:
				bplist_fb[uniform[SILVER]] = bonapiece(sq, pc);
				bplist_fw[uniform[SILVER]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[SILVER];
				uniform[SILVER]++;
				break;
			case BISHOP:case UNICORN:
				bplist_fb[uniform[BISHOP]] = bonapiece(sq, pc);
				bplist_fw[uniform[BISHOP]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[BISHOP];
				uniform[BISHOP]++;
				break;
			case ROOK:case DRAGON:
				bplist_fb[uniform[ROOK]] = bonapiece(sq, pc);
				bplist_fw[uniform[ROOK]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[ROOK];
				uniform[ROOK]++;
				break;
			case GOLD:
				bplist_fb[uniform[GOLD]] = bonapiece(sq, pc);
				bplist_fw[uniform[GOLD]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[GOLD];
				uniform[GOLD]++;
				break;
			case KING:
				bplist_fb[uniform[KING]] = bonapiece(sq, pc);
				bplist_fw[uniform[KING]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[KING];
				uniform[KING]++;
				break;
			default:
				UNREACHABLE;
				break;
			}

		}//end of while occ

	
		 //手駒
		int num = 0;

		for (Color hc = BLACK; hc < ColorALL; hc++) {

			const Hand& h = pos.hand(hc);
			if (have_hand(h)) {

				for (Piece pt = PAWN; pt < KING; pt++) {

					//コレは意図した記述
					num = num_pt(h, pt);
					if (num !=0) {

						for (int i = 1; i < num + 1; i++) {
							bplist_fb[uniform[pt]] = bonapiece(hc, pt, i);
							bplist_fw[uniform[pt]] = bonapiece(opposite(hc), pt, i);
							hand2Uniform[hc][pt][i] = uniform[pt];
							uniform[pt]++;
						}
					}
				}
			}
		}
	}//end of makebplist


	void BonaPList::print_bplist()
	{

		for (int i = 0; i < Num_Uniform; i++) {
			cout << "fb:" << bplist_fb[i]<<" "<<bp2sq(bplist_fb[i])<<" "<<bpwithoutsq(bplist_fb[i])<<bp2color(bplist_fb[i])<<" "<<bp2piece.bp_to_piece(bpwithoutsq(bplist_fb[i]))<<endl;
			cout << "fw:" << bplist_fw[i] << " " << bp2sq(bplist_fw[i]) << " " << bpwithoutsq(bplist_fw[i]) << bp2color(bplist_fw[i]) << " " << bp2piece.bp_to_piece(bpwithoutsq(bplist_fw[i])) <<endl;
			cout << "inverse fb:" << inversebonapiece(bplist_fb[i]) << "inverse fw:" << inversebonapiece(bplist_fw[i]) << endl;
			cout << "sym" << sym_rightleft(bplist_fb[i]) << endl << endl;;
			ASSERT(inversebonapiece(bplist_fb[i]) == bplist_fw[i]);
			ASSERT(inversebonapiece(bplist_fw[i]) == bplist_fb[i]);
		}

	}//endof print bp
	

	void BonaPList::list_check() {
		for (int i = 0; i < Num_Uniform; i++) {
			ASSERT(inversebonapiece(bplist_fb[i]) == bplist_fw[i]);
			ASSERT(inversebonapiece(bplist_fw[i]) == bplist_fb[i]);
		}
	}

	std::ostream & operator<<(std::ostream & os, const Eval::BonaPiece bp)
	{
		//bp<fe_handendであればそれは手駒
		if (bp < fe_hand_end) {

			//ここもうちょっとなんとかなるようなきがするんだけど,,,
			if (f_hand_pawn <= bp&&bp < e_hand_pawn) {
				cout << "f_handpawn + " << int(bp - f_hand_pawn) << endl;
			}
			else if (e_hand_pawn <= bp&&bp < f_hand_lance) {
				cout << "e_handpawn + " << int(bp - e_hand_pawn) << endl;
			}
			else if (f_hand_lance <= bp&&bp < e_hand_lance) {
				cout << "f_hand lance + " << int(bp - f_hand_lance) << endl;
			}
			else if (e_hand_lance <= bp&&bp < f_hand_knight) {
				cout << "e_handlance + " << int(bp - e_hand_lance) << endl;
			}
			else if (f_hand_knight <= bp&&bp < e_hand_knight) {
				cout << "f_handknight + " << int(bp - f_hand_knight) << endl;
			}
			else if (e_hand_knight <= bp&&bp < f_hand_silver) {
				cout << "e_hand_knight + " << int(bp - e_hand_knight) << endl;
			}
			else if (bp < e_hand_silver) {//条件はコレだけで十分か？？
				cout << "f_hand_silver + " << int(bp - f_hand_silver) << endl;
			}
			else if (bp < f_hand_gold) {
				cout << "e_hand_silver + " << int(bp - e_hand_silver) << endl;
			}
			else if (bp < e_hand_gold) {
				cout << "f_hand_gold + " << int(bp - f_hand_gold) << endl;
			}
			else if (bp < f_hand_bishop) {
				cout << "e_hand_gold + " << int(bp - e_hand_gold) << endl;
			}
			else if (bp < e_hand_bishop) {
				cout << "f_hand_bishop + " << int(bp - f_hand_bishop) << endl;
			}
			else if (bp < f_hand_rook) {
				cout << "e_hand_bishop + " << int(bp - e_hand_bishop) << endl;
			}
			else if (bp < e_hand_rook) {
				cout << "f_hand_rook + " << int(bp - f_hand_rook) << endl;
			}
			else if (bp < fe_hand_end) {
				cout << "e_hand_rook + " << int(bp - e_hand_rook) << endl;
			}

		}
		else {
			//盤上の駒
			if (f_pawn <= bp&&bp < e_pawn) {
				cout << "f_pawn + " << int(bp - f_pawn) << endl;
			}
			else if (e_pawn <= bp&&bp < f_lance) {
				cout << "e_pawn + " << int(bp - e_pawn) << endl;
			}
			else if (f_lance <= bp&&bp < e_lance) {
				cout << "f_lance + " << int(bp - f_lance) << endl;
			}
			else if (e_lance <= bp&&bp < f_knight) {
				cout << "e_lance + " << int(bp - e_lance) << endl;
			}
			else if (f_knight <= bp&&bp < e_knight) {
				cout << "f_knight + " << int(bp - f_knight) << endl;
			}
			else if (e_knight <= bp &&bp< f_silver) {
				cout << "e_knight + " << int(bp - e_knight) << endl;
			}
			else if (f_silver <= bp&&bp < e_silver) {
				cout << "f_silver + " << int(bp - f_silver) << endl;
			}
			else if (e_silver <= bp&&bp < f_gold) {
				cout << "e_silver + " << int(bp - e_silver) << endl;
			}
			else if (f_gold <= bp&&bp < e_gold) {
				cout << "f_gold + " << int(bp - f_gold) << endl;
			}
			else if (e_gold <= bp&&bp < f_bishop) {
				cout << "e_gold + " << int(bp - e_gold) << endl;
			}
			else if (bp<e_bishop) {
				cout << "f_bishop + " << int(bp - f_bishop) << endl;
			}
			else if (bp<f_unicorn) {
				cout << "e_bishop + " << int(bp - e_bishop) << endl;
			}
			else if (bp < e_unicorn) {
				cout << "f_unicorn + " << int(bp - f_unicorn) << endl;
			}
			else if (bp < f_rook) {
				cout << "e_unicorn + " << int(bp - e_unicorn) << endl;
			}
			else if (bp < e_rook) {
				cout << "f_rook + " << int(bp - f_rook) << endl;
			}
			else if (bp < f_dragon) {
				cout << "e_rook + " << int(bp - e_rook) << endl;
			}
			else if (bp < e_dragon) {
				cout << "f_dragon + " << int(bp - f_dragon) << endl;
			}
			else if (bp < f_king) {
				cout << "e_dragon + " << int(bp - e_dragon) << endl;
			}
			else if (bp < e_king) {
				cout << "f_king + " << int(bp - f_king) << endl;
			}
			else if (bp < fe_end2) {
				cout << "e_king + " << int(bp - e_king) << endl;
			}
			else {
				UNREACHABLE;
			}
		}
		return os;
	}

	//コマ割をチェックするための関数（カツ丼将棋がコマ割の値がおかしかったそうで怖くなってきたので確認しておく。）
	void komawari_check()
	{
		for (Piece p = B_PAWN; p < PC_ALL;p++) {

			if (p < KING) {
				cout << p << " piecevalue " << piece_value[p]<< " capture" << capture_value[p]  << " promote" << diff_promote[p] << endl;
			}
			else {
				cout << p << " piecevalue " << piece_value[p] << " capture" << capture_value[p] << endl;
			}
		}
	}


	//bonapieceの左右を反転させる関数
	BonaPiece sym_rightleft(const BonaPiece bp) {

		//手駒のbpは左右反転なんて出来ないのでそのままの値を返す。
		if (bp < fe_hand_end) {
			return bp;
		}

		int sq, symsq;
		//手駒以外

		//縦型bitboardなのでrank=sq%9,file=sq/9
		//BonaPiece bprank, bpfile;
		//盤上の駒
		if (f_pawn <= bp&&bp < e_pawn) {
			sq = bp - f_pawn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_pawn + symsq);
		}
		else if (e_pawn <= bp&&bp < f_lance) {
			sq = bp - e_pawn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_pawn + symsq);
		}
		else if (f_lance <= bp&&bp < e_lance) {
			sq = bp - f_lance;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_lance + symsq);
		}
		else if (e_lance <= bp&&bp < f_knight) {
			sq = bp - e_lance;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_lance + symsq);
		}
		else if (f_knight <= bp&&bp < e_knight) {
			sq = bp - f_knight;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_knight + symsq);
		}
		else if (e_knight <= bp &&bp< f_silver) {
			sq = bp - e_knight;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_knight + symsq);
		}
		else if (f_silver <= bp&&bp < e_silver) {
			sq = bp - f_silver;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_silver + symsq);
		}
		else if (e_silver <= bp&&bp < f_gold) {
			sq = bp - e_silver;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_silver + symsq);
		}
		else if (f_gold <= bp&&bp < e_gold) {
			sq = bp - f_gold;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_gold + symsq);
		}
		else if (e_gold <= bp&&bp < f_bishop) {
			sq = bp - e_gold;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_gold + symsq);
		}
		else if (bp<e_bishop) {
			sq = bp - f_bishop;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_bishop + symsq);
		}
		else if (bp<f_unicorn) {
			sq = bp - e_bishop;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_bishop + symsq);
		}
		else if (bp < e_unicorn) {
			sq = bp - f_unicorn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_unicorn + symsq);
		}
		else if (bp < f_rook) {
			sq = bp - e_unicorn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_unicorn + symsq);
		}
		else if (bp < e_rook) {
			sq = bp - f_rook;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_rook + symsq);
		}
		else if (bp < f_dragon) {
			sq = bp - e_rook;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_rook + symsq);
		}
		else if (bp < e_dragon) {
			sq = bp - f_dragon;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_dragon + symsq);
		}
		else if (bp < f_king) {
			sq = bp - e_dragon;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_dragon + symsq);
		}
		else if (bp < e_king) {
			sq = bp - f_king;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_king + symsq);
		}
		else if (bp < fe_end2) {
			sq = bp - e_king;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_king + symsq);
		}
		else {
			UNREACHABLE;
			return BONA_PIECE_ZERO;
		}
		
	}



	void Eval::param_sym_ij() {

		
		
		bool check[fe_end2][fe_end2] = { false };
		memset(check, false, sizeof(check));


		for (BonaPiece i = BONA_PIECE_ZERO; i < fe_end2; i++) {

			for (BonaPiece j = BONA_PIECE_ZERO; j < fe_end2; j++) {

				if (check[i][j] != true) {

					check[i][j] =check[j][i]= true;

					int32_t a = PP[i][j], b = PP[j][i];
					PP[i][j] = PP[j][i] = (a + b) / 2;

				}


			}

		}

		write_PP();
		read_PP();
	}






	/*
	AVX2を用いた評価関数計算高速化。
	
	nodchipさんの本を参考に読み進めていく。
	
	PPはlist[40]を用いて計算を行うので
	8個の要素ずつの計算で完了させることができりゅ。

	PP要素は32bit

	_mm256_i32gather_epi32
	https://www.bing.com/search?q=_mm256_i32gather_epi32&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127

	_mm256_load_si256
	https://www.bing.com/search?q=_mm256_load_si256&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127

	*/
	//Value eval_allPP_AVX2(const Position & pos)
	//{
	//	int32_t bPP = 0, wPP = 0;
	//	BonaPiece *list_fb = pos.evallist().bplist_fb, *list_fw = pos.evallist().bplist_fw;

	//	__m256i zero = _mm256_setzero_si256();
	//	__m256i bPP256 = zero, wPP256 = zero;

	//	//40要素に対して計算を行う
	//	for (int i = 0; i < 40; ++i) {

	//		const BonaPiece k0_b = list_fb[i],k0_w=list_fw[i];

	//		//pointer pp black||white
	//		const auto* p_pp_b = PP[k0_b];
	//		const auto* p_pp_w = PP[k0_w];

	//		int j = 0;
	//		//8要素一気に計算する
	//		for (; j + 8 < i; j += 8) {

	//			//bonapieceのindexを8要素ロードする。
	//			__m256i indices_fb = _mm256_load_si256(reinterpret_cast<const __m256i*>(&list_fb[j]));
	//			__m256i indices_fw = _mm256_load_si256(reinterpret_cast<const __m256i*>(&list_fw[j]));

	//			//p_pp_bとp_pp_wから8要素をギャザーしてくる。（4というのはi32*4）
	//			__m256i b = _mm256_i32gather_epi32(reinterpret_cast<const int*>(p_pp_b), indices_fb, 4);
	//			__m256i w = _mm256_i32gather_epi32(reinterpret_cast<const int*>(p_pp_w), indices_fw, 4);

	//			//下位128bitを16bit変数から32bit整数に変換する
	//			__m256i blo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(b, 0));//0というのは128*0
	//			__m256i wlo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w, 0));

	//			//bpp256,wpp256に足し合わせる
	//			bPP256 = _mm256_add_epi32(bPP256, blo);
	//			wPP256 = _mm256_add_epi32(wPP256, wlo);

	//			//上位128bitを16bit変数から32bit整数に変換する
	//			//_mm256_extracti128_si256 ：上位or下位128bitをxmmレジスタに格納する
	//			//_mm256_cvtepi16_epi32 :16 ビットの符号付き整数から 32 ビットの整数への符号拡張パックド移動操作を実行 : https://www.bing.com/search?q=_mm256_cvtepi16_epi32&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127
	//			__m256i bhi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(b, 1));//0というのは128*0
	//			__m256i whi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w, 1));

	//			//bpp256,wpp256に足し合わせる
	//			bPP256 = _mm256_add_epi32(bPP256, bhi);
	//			wPP256 = _mm256_add_epi32(wPP256, whi);
	//		}

	//	}
	//	//上位128bitと下位128bitを独立して8byte(64bit)右シフトしたものを足し合わせる。なんで8byteシフトさせる？？全然わからん.......
	//	bPP256 = _mm256_add_epi32(bPP256, _mm256_srli_si256(bPP256, 8));
	//	wPP256 = _mm256_add_epi32(wPP256, _mm256_srli_si256(wPP256, 8));

	//	//bpp256の上位128bitと下位128bitを足し合わせてbpp128の代入する
	//	__m128i bpp128 = _mm_add_epi32(_mm256_extracti128_si256(bPP256,0), _mm256_extracti128_si256(bPP256, 1));
	//	__m128i wpp128 = _mm_add_epi32(_mm256_extracti128_si256(wPP256, 0), _mm256_extracti128_si256(wPP256, 1));



	//	pos.state()->bpp = (bPP);
	//	pos.state()->wpp = wPP;
	//	//評価値ホントに16bitで収まるかどうか確認
	//	ASSERT(abs(bPP + wPP) / FV_SCALE < INT16_MAX);

	//	return Value((bPP + wPP) / FV_SCALE);


	//}











}