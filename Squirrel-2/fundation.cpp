#include "fundation.h"

using namespace std;

File SQ2File[SQ_NUM] = {
	File1,File1,File1,File1,File1,File1,File1,File1,File1,
	File2,File2,File2,File2,File2,File2,File2,File2,File2,
	File3,File3,File3,File3,File3,File3,File3,File3,File3,
	File4,File4,File4,File4,File4,File4,File4,File4,File4,
	File5,File5,File5,File5,File5,File5,File5,File5,File5,
	File6,File6,File6,File6,File6,File6,File6,File6,File6,
	File7,File7,File7,File7,File7,File7,File7,File7,File7,
	File8,File8,File8,File8,File8,File8,File8,File8,File8,
	File9,File9,File9,File9,File9,File9,File9,File9,File9,
};

Rank SQ2Rank[SQ_NUM] = {
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,
};

Rank sqtorank(const Square sq)
{
	return SQ2Rank[sq];
}

File sqtofile(const Square sq)
{
	return SQ2File[sq];
}


std::ostream & operator<<(std::ostream & os, const Rank r)
{
	ASSERT(is_ok(r));
	char rank2USI[] = { 'a','b','c','d','e','f','g','h','i' };
	os << rank2USI[r];
	return os;
}

std::ostream & operator<<(std::ostream & os, File f)
{
	ASSERT(is_ok(f));
	os << int(f + 1);
	return os;
}

std::ostream & operator<<(std::ostream & os, const Piece pc)
{
	//os << outputPiece[pc];
	os << USIPiece[pc];
	return os;
}



std::ostream & operator<<(std::ostream & os, const Square sq)
{
	if (sq == Error_SQ) { os << "errorsq"; return os; }
	os << sqtofile(sq) << sqtorank(sq);
	return os;
}

std::string outputPiece[PC_ALL] = { " 0"," P"," L"," N"," S"," B"," R"," G","_K","+P","+L","+N","+S","+B","+R","15","16",
" p"," l"," n"," s"," b"," r"," g","_k","+p","+l","+n","+s","+b","+r" };

std::string USIPiece[PC_ALL] = { "0","P","L","N","S","B","R","G","K","+P","+L","+N","+S","+B","+R","15","16",
"p","l","n","s","b","r","g","k","+p","+l","+n","+s","+b","+r" };

std::ostream & operator<<(std::ostream & os, const Move m)
{
	//Piece moved;
	Square from = move_from(m);
	Square to = move_to(m);
	if (m == MOVE_NULL) { os << "nullmove"; return os; }
	if (m == MOVE_NONE) { os << "movenone"; return os; }
	if (is_drop(m)) {

		Piece moved = moved_piece(m);
		Piece pt = piece_type(moved);
		os << pt << "*" << to;
	}
	else {
		os << from << to;
		if (is_promote(m)) {
			os << "+";
		}
	}
	return os;
}

void check_move(const Move m)
{
	std::cout << "from " << move_from(m) << " to " << move_to(m) << " moved_piece " << moved_piece(m) << " " << int(m)<<" ";
	if (is_drop(m))
		std::cout << " drop ";

	/*if (is_capture(m))
		std::cout << " capture ";
*/
	if (is_promote(m))
		std::cout << " promote ";

	std::cout << std::endl;


}

std::ostream & operator<<(std::ostream & os, Hand h)
{
	std::string handtoprint[8] = { "_","歩","香","桂","銀","角","飛","金" };

	cout << "〜〜〜手駒の確認〜〜〜" << endl;

	for (Piece pt = PAWN; pt <= GOLD; pt++) {

		int num = num_pt(h, pt);
		if (num) 
		{
			os << handtoprint[pt] << " " << num << " ";
		}
	}
	os << endl;
	return os;

}


std::ostream& operator << (std::ostream& os, Direction d) {

	switch (d)
	{
	case UP:
		os << "  U";
		break;
	case RightUP:
		os << " RU";
		break;
	case Right:
		os << "  R";
		break;
	case RightDOWN:
		os << " RD";
		break;
	case DOWN:
		os << "  D";
		break;
	case LeftDOWN:
		os << " LD";
		break;
	case Left:
		os << "  L";
		break;
	case LeftUP:
		os << " LU";
		break;
	default:
		os << "   ";
		break;
	}
	return os;
}
//
//std::ostream& operator << (std::ostream& os, const Effect e) {
//
//	//まずは利きの数
//	cout << int(e&ENUM_MASK) << " ";
//
//	//飛び利きがどちら側から聞いているか
//	for (int i = 0; i < Direct_NUM; i++) {
//		Direction d = direct[i];
//		if (is_havelong_direct(e, d)) {
//			cout << d << " ";
//		}
//	}
//
//	cout << endl;
//
//	return os;
//}
