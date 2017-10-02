#include "misc.h"
#include "fundation.h"

#include <sstream>
#include <random>


File Sfen2File(const char c) {

	return File(c - '1');

}
Rank Sfen2Rank(const char c) {
	return Rank(c - 'a');
}


File CSA2File(const char c) {

	return File(c - '1');

}
Rank CSA2Rank(const char c) {
	return Rank(c - '1');
}


string itos(int number)
{
	stringstream ss;
	ss << number;
	return ss.str();
}


Move Sfen2Move(const string smove, const Position& pos)
{
	//example 2c2b+ P*1a
	ASSERT(smove.size() <= 5);

	Color turn = pos.sidetomove();
	Move m;

	if (isdigit(smove[0])) {

		//最初が文字であればコマの移動

		File ff = Sfen2File(smove[0]);
		Rank fr = Sfen2Rank(smove[1]);
		Square from = make_square(fr, ff);
		File tf = Sfen2File(smove[2]);
		Rank tr = Sfen2Rank(smove[3]);
		Square to = make_square(tr, tf);

		Piece pc = pos.piece_on(from);
		ASSERT(pc != NO_PIECE);

		if (smove.size() == 5 && smove[4] == '+') {
			m = make_movepromote(from, to, pc);
		}
		else {
			m = make_move(from, to, pc);
		}
	}
	else {

		//駒打ち
		ASSERT(smove[1] == '*');

		Piece pt = Sfen2Piece_.sfen_to_piece(smove[0]);
		Piece pc = add_color(pt, turn);
		Square to = make_square(Sfen2Rank(smove[3]), Sfen2File(smove[2]));
		ASSERT(num_pt(pos.hand(turn), pt));
		m = make_drop(to, pc);
	}

	//cout << m << endl;
	//check_move(m);
	return m;

}
/*
2824HI
8586FU
8786FU
8286HI
5958OU
8676HI
8877KA
5152OU
2434HI
7626HI
0028FU
6172KI
3736FU
0083FU
2937KE
2277UM
*/

/*
NO_PIECE, PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD, KING,
PRO_PAWN, PRO_LANCE, PRO_NIGHT, PRO_SILVER, UNICORN, DRAGON,PT_ALL,
*/
const pair<string, Piece> CSAtoPiece[PT_ALL] = { {"",NO_PIECE},{"FU",PAWN},{"KY",LANCE},{"KE",KNIGHT},{"GI",SILVER},{"KA",BISHOP},{"HI",ROOK},{"KI",GOLD},{"OU",KING}
,{"TO",PRO_PAWN},{"NY",PRO_LANCE},{"NK",PRO_NIGHT},{"NG",PRO_SILVER},{"UM",UNICORN},{"RY",DRAGON} };

Piece csa_to_pt(const string p) {

	for (int i = 1; i < PT_ALL; i++) {
		if (p == CSAtoPiece[i].first) {
			return CSAtoPiece[i].second;
		}
	}
	//UNREACHABLE;
	return NO_PIECE;
}

Move CSA2Move(const string smove, const Position& pos)
{
	//文字列が長すぎる
	if (smove.size() >= 7) {
		return MOVE_NONE;
	}
	else if (smove.size() < 4){
		return MOVE_NONE;
	}
	Move m;


	//駒打ちの場合も考えられるので先に移動先から生成する。
	const File tofile = CSA2File(smove[2]);
	const Rank torank = CSA2Rank(smove[3]);

	const Square to = make_square(torank, tofile);

	Piece pt = csa_to_pt(smove.substr(4));
	if (pt == NO_PIECE) { return MOVE_NONE; }
	const Piece pc = add_color(pt, pos.sidetomove());

	if (smove.substr(0,2)=="00") {
		//コマ打ちの場合						 
		return  m = make_drop(to, pc);
	}
	else {
		//駒移動の場合
		const File fromfile = CSA2File(smove[0]);
		const Rank fromran = CSA2Rank(smove[1]);
		const Square from = make_square(fromran, fromfile);
		const Piece fromPC = pos.piece_on(from);
		if (fromPC == NO_PIECE) { return MOVE_NONE; }

		//成りはposのfromにいる駒とpcが違うかどうかで内部的に判定させないといけないみたい

		if (fromPC == pc) {
			return  m = make_move(from, to, pc);
		}
		else if (fromPC + PROMOTE == pc) {
			//moveの情報に格納するのは成る前の駒種である
			return  m = make_movepromote(from, to, fromPC);
		}
		else {
			return MOVE_NONE;
		}
	}

	return MOVE_NONE;
}

#ifdef MISC
/*
gradient noize実装用に用意した
http://yaneuraou.yaneu.com/2016/08/28/%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8B%E3%81%AE%E5%AD%A6%E7%BF%92%E3%83%AB%E3%83%BC%E3%83%81%E3%83%B3%E3%82%92%E4%BD%BF%E3%81%A3%E3%81%A6%E3%81%84%E3%82%8B%E9%96%8B%E7%99%BA%E8%80%85%E3%81%AE/
http://postd.cc/optimizing-gradient-descent/#gradientnoise
*/
inline double normal_dist(double mean, double stddiv)
{
	std::default_random_engine generator;
	std::normal_distribution<double> dist(mean, stddiv);

	return dist(generator);
}
#endif