#include "Bitboard.h"
#include "bitop.h"
#include "occupied.h"
#include "position.h"

#include <bitset>
#include <algorithm>
using namespace std;



Bitboard SquareBB[SQ_NUM];//OK
Bitboard FileBB[File_Num];//OK
Bitboard RankBB[Rank_Num];//OK
Bitboard ZeroBB, ALLBB;//OK
Bitboard canPromoteBB[ColorALL];//OK

Bitboard StepEffect[ColorALL][PT_ALL][SQ_NUM];//OK

//127・・7bitすべて１の場合 
//飛車の効きと角の利きは前後対称
//成った場合はlongeffectに王の機器を足せばいい。
/*
roteted bitboardを使っているbonanzaは利きは縦横別で持っていたのでそれに習う
*/
Bitboard LongRookEffect_yoko[SQ_NUM][128];//OK
Bitboard LongRookEffect_tate[SQ_NUM][128];//OK(香車もコレを使うコレを香車でも使うためにはinfrontBBを用意しなければならない)
Bitboard LongBishopEffect_plus45[SQ_NUM][128];//OK
Bitboard LongBishopEffect_minus45[SQ_NUM][128];//OK

//Rankではなくてsquareで持とうかと思ったけどメモリの節約と、ｓｑからランクに変換するの簡単だからrankで持つことにする。
Bitboard InFront_BB[ColorALL][Rank_Num];//OK

//駒が移動できない領域が１になったbitboard
Bitboard CantGo_PAWNLANCE[ColorALL];
Bitboard CantGo_KNIGHT[ColorALL];


//directtable
//[from][to]
//fromから見たtoの位置関係。
Direction direct_table[SQ_NUM][SQ_NUM];




Square Bitboard::pop()
{
	return (b[0] != 0) ? Square(pop_lsb(b[0])) : Square(pop_lsb(b[1])+45);
}

std::ostream & operator<<(std::ostream & os, const Bitboard & board)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {

			Square sq = make_square(r, f);
			if (sq <= 44) {
				if ((board.b[0])&(1ui64 << sq)) {
					os << "*";
				}
				else {
					os << ".";
				}
			}
			else {
				if ((board.b[1])&(1ui64 << (sq-45))) {
					os << "*";
				}
				else {
					os << ".";
				}

			}
		}
		os << std::endl;
	}
	return os;
}

/*
bitを上位下位入れ替えるための関数
ここでは入ってくるbitは7bitであると仮定する。
*/
int change_indian(int i) {

	//iは7bit以下でなければならない
	_ASSERT((i >> 8) == 0);

	int ret = 0;
	for (int b = 0; b < 7; b++) {
		if (i&(1 << b)) {
			ret |= 1 << (6 - b);
		}
	}

	return ret;
}


/*
シフト三角行列
*/

int additional_plus45(Square sq) {

	File f = sqtofile(sq);
	Rank r = sqtorank(sq);

	return std::max(int(f - r), 0);

}

int additional_minus45(Square sq) {

	File f = sqtofile(sq);
	Rank r = sqtorank(sq);

	return std::max(int(f+r-8), 0);

}

void bitboard_init()
{
	ZeroBB = Bitboard(0, 0);


	//SquareBB作るのにも割りと注意が必要なんスねぇ(´・ω・｀)とりあえず1ui64を使えばいいらしい。たぶん他のコンパイラでは使えないような気がする。
	/*まずはsquareBBの作成。
	それを利用してRank,file,Effect,Bitweenを作っていく
	*/
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		//31bit離れたところにもう一つbitが立ってしまっている！！！！！！！（解決した）
		if (sq <= SQ5I) {
			SquareBB[sq] = Bitboard(uint64_t(1ui64 << sq), 0);
		}
		else {
			SquareBB[sq] = Bitboard(0, uint64_t(1ui64 << (sq - SQ6A)));
		}
		ALLBB ^= SquareBB[sq];
	}
	//rankBBの作成
	for (Square sq = SQ1A; sq < SQ9I; sq = sq + 9) {
		RankBB[RankA] ^= SquareBB[sq];
		RankBB[RankB] ^= SquareBB[sq + 1];
		RankBB[RankC] ^= SquareBB[sq + 2];
		RankBB[RankD] ^= SquareBB[sq + 3];
		RankBB[RankE] ^= SquareBB[sq + 4];
		RankBB[RankF] ^= SquareBB[sq + 5];
		RankBB[RankG] ^= SquareBB[sq + 6];
		RankBB[RankH] ^= SquareBB[sq + 7];
		RankBB[RankI] ^= SquareBB[sq + 8];
	}
	//FileBBの作成
	for (Square sq = SQ1A; sq <= SQ1I; sq = sq + 1) {
		FileBB[File1] ^= SquareBB[sq];
		FileBB[File2] ^= SquareBB[sq + 9];
		FileBB[File3] ^= SquareBB[sq + 18];
		FileBB[File4] ^= SquareBB[sq + 27];
		FileBB[File5] ^= SquareBB[sq + 36];
		FileBB[File6] ^= SquareBB[sq + 45];
		FileBB[File7] ^= SquareBB[sq + 54];
		FileBB[File8] ^= SquareBB[sq + 63];
		FileBB[File9] ^= SquareBB[sq + 72];
	}

	//infrontofBB
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (Rank rr = RankA; rr < r; rr++) {
			InFront_BB[BLACK][r] ^= RankBB[rr];
		}
	}
	for (Rank r = RankI; r >= RankA; r--) {
		for (Rank rr = RankI; rr > r; rr--) {
			InFront_BB[WHITE][r] ^= RankBB[rr];
		}
	}

	//桂馬と香車と負のならずで移動できないランクのbitboard
	CantGo_PAWNLANCE[BLACK] = RankBB[RankA];
	CantGo_PAWNLANCE[WHITE] = RankBB[RankI];

	CantGo_KNIGHT[BLACK] = RankBB[RankA] ^ RankBB[RankB];
	CantGo_KNIGHT[WHITE] = RankBB[RankI] ^ RankBB[RankH];

	//canpromote
	canPromoteBB[BLACK] = RankBB[RankA] ^ RankBB[RankB] ^ RankBB[RankC];
	canPromoteBB[WHITE] = RankBB[RankG] ^ RankBB[RankH] ^ RankBB[RankI];

	//====================
	//近接駒の利きのbitboard
	//====================
	/*
	B_PAWN=1, B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING,
	B_PRO_PAWN, B_PRO_LANCE, B_PRO_NIGHT, B_PRO_SILVER, B_UNICORN, B_DRAGON,
	W_PAWN=17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING,
	W_PRO_PAWN, W_PRO_LANCE, W_PRO_NIGHT, W_PRO_SILVER, W_UNICORN, W_DRAGON,PC_ALL,
	*/
	//12方向だが１つの駒は８方向までにしか動けないため配列のサイズは８でいいと思う
	//黒番から見た方向ベクトルを使う。
	//飛車角はstepeffectつくってもしょうがないんだけどなぁ(´・ω・｀)
	//idea from shogi686
	const int stepdirec[PT_ALL][8] = {

		{0,0,0,0,0,0,0,0},
		{-1,0,0,0,0,0,0,0},//歩
		{-1,0,0,0,0,0,0,0},//香車
		{-11,7,0,0,0,0,0,0},//桂馬
		{8,-1,-10,-8,10,0,0,0},//銀
		{-10,-8,8,10,0,0,0,0},//角
		{-1,1,9,-9,0,0,0,0},//飛車
		{9,8,-1,-10,-9,1,0,0},//金(これがなんかおかしい)（解決済み）
		{-1,-10,-9,-8,1,10,9,8},//王
		{ 9,8,-1,-10,-9,1,0,0 },//と
		{ 9,8,-1,-10,-9,1,0,0 },//成香
		{ 9,8,-1,-10,-9,1,0,0 },//成桂
		{ 9,8,-1,-10,-9,1,0,0 },//成銀
		{ -1,-10,-9,-8,1,10,9,8 },//馬
		{ -1,-10,-9,-8,1,10,9,8 },//飛車

	};

	//StepEffect
	Square to;
	for (Color c = BLACK; c <= WHITE; c++) {

		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

				Bitboard b = ZeroBB;

				for (int direc = 0; direc < 8; direc++) {
					c == BLACK ? to = sq + stepdirec[pt][direc] : to = sq - stepdirec[pt][direc];
					//idea from mermo 686
					//飛び効き以外で縦に３升以上動く駒はない
					if ((to != sq) && is_ok(to) && std::abs(int(sqtorank(sq) - int(sqtorank(to)))) <= 2) {
						b ^= SquareBB[to];
					}
				}
				StepEffect[c][pt][sq] = b;
			}
		}
	}
	//LongEffect
	/*
	long effectはちゃんとroteted bitboard理解してから実装した方がいいよね..
	テーブルの用意だけなら今できるのでいましておこうか...
	*/

	/*
	縦と横を分けて格納するのか？？
	*/

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {
			int obstacle2 = obstacle<<1;//一つシフトしてやらないといけない！！！
			int direc_rook_yoko[2] = { 9,-9 };
			
			File tofile;
			for (int i = 0; i < 2; i++) {
				to = sq;
				do {
					to += Square(direc_rook_yoko[i]);
					tofile = sqtofile(to);
					if (is_ok(to)&&(to != sq)) {
						LongRookEffect_yoko[sq][obstacle] ^= SquareBB[to];
					}
					//横や縦の関係であればこの方法は使えるけど斜めでは通用しないぞ...
				} while ((!(obstacle2&(1 << tofile))) && is_ok(to));
			}

		}
	}
	//縦。下端が上手くいっていないなぜだろう？解決済み
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {
			int direc_rook_yoko[2] = { 1,-1 };
			int obstacle2 = obstacle<<1;//一つシフトしてやらないといけない！！！
			Rank torank;
			File tofile;
			for (int i = 0; i < 2; i++) {
				to = sq;
				do {
					to += Square(direc_rook_yoko[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq) && (tofile == sqfile)) {
						LongRookEffect_tate[sq][obstacle] ^= SquareBB[to];
					}
					//横や縦の関係であればこの方法は使えるけど斜めでは通用しないぞ...
				} while ((!(obstacle2&(1 << torank))) && is_ok(to)&&(tofile==sqfile));//縦の場合はfileが同じかどうかも調べなければならない。
			}
		}
	}



	/*
	よく考えると斜めもrankに射影してしまえば横と同じように処理できるか(´・ω・｀)
	横のほうが処理が簡単なので横で計算する。
	*/
	//斜めプラス４５度
	//この方法では上から角の効きが伸びてきてしまうということが起こりうる！！(解決済み)

	/*
	この方法でテーブル作るのはまずい！！！考え直し！！！！！
	*/
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			int direc_bishop_p45[2] = { -10, + 10 };
			int obstacle2 = obstacle << (1+additional_plus45(sq));//1bitシフトさせる。

			Rank torank;
			File tofile;//横方向への射影。
			Square oldto;
			Rank oldrank;
			for (int i = 0; i < 2; i++) {
				to = sq;
				//tofile = sqtofile(to);
				do {
					/*
					oldtoを保持してoldtoとnewtoのrankが２つ以上離れないようにする。
					*/
					oldto = to;
					oldrank = sqtorank(oldto);
					to += Square(direc_bishop_p45[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq)&&(abs(torank-oldrank)<2)) {
						LongBishopEffect_plus45[sq][obstacle] ^= SquareBB[to];
					}
				} while (!(obstacle2&(1 << (tofile))) && is_ok(to) && (abs(torank - oldrank)<2));
			}
		}
	}


	//斜め-45度
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		File sqfile = sqtofile(sq);
		Rank sqrank = sqtorank(sq);
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			//int obstacle_ = change_indian(obstacle);
			int direc_bishop_m45[2] = { -8, +8 };
			int obstacle2 = obstacle << (1+additional_minus45(sq));//

			Rank torank;
			File tofile;//横方向への射影。
			Square oldto;
			Rank oldrank;
			for (int i = 0; i < 2; i++) {
				to = sq;
				//tofile = sqtofile(to);
				do {
					/*
					oldtoを保持してoldtoとnewtoのrankが２つ以上離れないようにする。
					*/
					oldto = to;
					oldrank = sqtorank(oldto);
					to += Square(direc_bishop_m45[i]);
					torank = sqtorank(to);
					tofile = sqtofile(to);
					if (is_ok(to) && (to != sq) && (abs(torank - oldrank)<2)) {
						//obstacleのbitを1を7に7を1にというように入れ替えなければならない。
						LongBishopEffect_minus45[sq][(obstacle)] ^= SquareBB[to];
					}
				} while ((!( obstacle2&(1 << (tofile)))) && is_ok(to) && (abs(torank - oldrank)<2));
			}
		}
	}

	//indexの作成
	make_indexplus45();
	make_shiftplus45();

	make_shiftMinus45();


	for (Square from = SQ_ZERO; from < SQ_NUM; from++) {

		Direction d;
		for (int i = 0; i < Direct_NUM; i++) {

			Square to,oldto;
			d = direct[i];
			to = from;
			do {
				oldto = to;
				to += Square(d);

				if (is_ok(to) &&to!=from&& abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2) {
					direct_table[from][to] = d;
				}

			} while (is_ok(to) && abs(sqtorank(to) - sqtorank(oldto)) < 2 && abs(sqtofile(to) - sqtofile(oldto)) < 2);
		}
	}

	//check_directtable();
	//bitboard_debug();
}


void bitboard_debug()
{
	/*cout << ZeroBB << endl;*/
	//cout << ALLBB << endl;
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout << "Square " << sq << endl;
		cout << SquareBB[sq] << endl;

	}*/
	/*for (Rank r = RankA; r < Rank_Num; r++) {
		cout << RankBB[r] << endl;
	}
	for (File f = File1; f < File_Num; f++) {
		cout << FileBB[f] << endl;
	}*/
	/*for (Color c = BLACK; c <= WHITE; c++) {
		cout << canPromoteBB[c] << endl;
	}*/
/*
	for (Color c = BLACK; c < ColorALL; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
				std::cout << "color " << c << " psq " << sq << " pt " << pt << std::endl;
				std::cout << StepEffect[c][pt][sq] << std::endl << std::endl;
			}
		}
	}
*/
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "ROOK sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongRookEffect_yoko[sq][obstacle] << endl;

		}
	}*/
	//
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "ROOK sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongRookEffect_tate[sq][obstacle] << endl;

		}
	}*/
	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "BIshop sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongBishopEffect_plus45[sq][obstacle] << endl;

		}
	}*/
	/*for (Square sq = SQ9I; sq >= SQ1A; sq--) {
		for (int obstacle = 0; obstacle < 128; obstacle++) {

			cout << "BISHOP sq " << sq << " obstacle " << static_cast<std::bitset<7>>(obstacle) << endl;
			cout << LongBishopEffect_minus45[sq][obstacle] << endl;

		}
	}*/

	/*for (Color c = BLACK; c <= WHITE; c++) {

		for (Rank r = RankA; r < Rank_Num; r++) {
			cout << " color " << c << " rank " << r << endl;
			cout << InFront_BB[c][r] << endl;
		}
	}*/

	//テーブルがちゃんと作れていなかった
	//cout << LongBishopEffect_minus45[39][(0b0100010)] << endl;

}

//飛び利きでない場合はposを渡すのは無駄になってしまうので飛び利き用と飛び利きでない用で関数を分けた方がいいか？
Bitboard effectBB(const Position &pos,const Piece pt, const Color c, const Square sq) {

	//Bitboard effect;

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;


	switch (pt)
	{
	case PAWN:
		return StepEffect[c][pt][sq];
		break;
	case LANCE:
		obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bitしか必要ないのでintでいいか（で十分か？？？？）
		return LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
		break;
	case KNIGHT:
		return StepEffect[c][pt][sq];
		break;
	case SILVER:
		return StepEffect[c][pt][sq];
		break;
	case BISHOP:
		// の方が早く処理できるか？？
		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
		break;
	case ROOK:
		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case GOLD:case PRO_PAWN:case PRO_LANCE:case PRO_NIGHT:case PRO_SILVER:
		return StepEffect[c][GOLD][sq];
		break;
	case KING:
		return StepEffect[c][KING][sq];
		break;
	case UNICORN:
		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
		break;
	case DRAGON:
		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
		break;
	default:
		ASSERT(0);
		//return;
		break;
	}

}

//近接利き用で分けてみた。
Bitboard return_step_effect(const Piece pt, const Color c, const Square sq) {

	switch (pt)
	{
	case PAWN:
		return StepEffect[c][pt][sq];
		break;
	
	case KNIGHT:
		return StepEffect[c][pt][sq];
		break;
	case SILVER:
		return StepEffect[c][pt][sq];
		break;
	
	case GOLD:case PRO_PAWN:case PRO_LANCE:case PRO_NIGHT:case PRO_SILVER:
		return StepEffect[c][GOLD][sq];
		break;
	case KING:
		return StepEffect[c][KING][sq];
		break;
	
	default:
		ASSERT(0);
		//return;
		break;
	}
}



//飛び利きでない場合はposを渡すのは無駄になってしまうので飛び利き用と飛び利きでない用で関数を分けた方がいいか？
Bitboard return_long_effect(const Position &pos, const Piece pt, const Color c, const Square sq) {

	//Bitboard effect;

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;

	switch (pt)
	{
	case BISHOP:
		// の方が早く処理できるか？？
		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
		break;
	case ROOK:
		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case UNICORN:
		 obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		return  LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
		break;
	case DRAGON:
		 obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		return LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
		break;
	default:
		ASSERT(0);
		//return;
		break;
	}

}




void check_directtable()
{
	cout << "check direct table " << endl;

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout << "from " << sq << endl;
		for (Rank r = RankA; r < Rank_Num; r++) {
			for (File f = File9; f >= File1; f--) {
				Square to = make_square(r, f);
				cout << direct_table[sq][to];
			}
			cout << endl;
		}
	}
	cout << endl;


}
