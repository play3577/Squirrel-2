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
	os << sqtofile(sq) << sqtorank(sq);
	return os;
}



std::ostream & operator<<(std::ostream & os, const Move m)
{
	//Piece moved;
	Square from = move_from(m);
	Square to = move_to(m);

	if (is_drop(m)) {

		Piece moved = moved_piece(m);
		Piece pt = piece_type(moved);
		os << pt << "*" << to;
	}
	else {
		os << from << to;
		if (is_propawn(m)) {
			os << "+";
		}
	}
	return os;
}

void check_move(const Move m)
{
	std::cout << "from " << move_from(m) << " to " << move_to(m) << " moved_piece " << moved_piece(m) << " " << int(m) << std::endl;
	if (is_drop(m))
		std::cout << " drop ";

	if (is_capture(m))
		std::cout << " capture ";

	if (is_propawn(m))
		std::cout << " propawn ";

	std::cout << std::endl;


}

std::ostream & operator<<(std::ostream & os, Hand h)
{
	char* handtoprint[8] = { "_","•à","","Œj","‹â","Šp","”ò","‹à" };

	cout << "```Žè‹î‚ÌŠm”F```" << endl;

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
