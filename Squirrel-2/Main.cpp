#include "fundation.h"
#include "Bitboard.h"
#include "position.h"
using namespace std;


int main() {

	bitboard_init();

	Position pos;

	//bitboard_debug();
	
	/*
	for (Square a = SQ1A; a < SQ_NUM; a++) {

		cout << a << endl;
	}*/
	/*for (Piece pc = NO_PIECE; pc < PC_ALL; pc++) {
		cout << pc << endl;
	}*/
	/*Move normal = make_move(SQ9I, SQ1D, ROOK);
	Move promote = make_movepropawn(SQ9I, SQ1D, ROOK);
	Move drop = make_drop(SQ9I, ROOK);
	check_move(normal);
	check_move(promote);
	check_move(drop);*/

	//Hand h = Hand(0b00011);//•à‚ª‚R–‡
	//h += Hand(0b100<<6);//‚ª‚S–‡
	//h += Hand(0b100 << 9);//Œj”n‚ª4–‡
	//h += Hand(0b100 << 12);//‹â‰Í4–‡
	//h += Hand(0b10 << 15);//Šp‚ª‚Q–‡
	//h += Hand(0b10 << 17);//”òŽÔ‚ª2–‡
	//h += Hand(0b100 << 19);
	//cout << h << endl;
	//for (Piece pt = PAWN; pt <= GOLD; pt++) {
	//	makehand(h, pt, 1);
	//	cout << h << endl;
	//}
	
	

	//pos.set(sfen_max);
	//pos.set(sfen_maturi);

	/*pos.remove_piece(BLACK, PAWN, SQ1G);
	cout << pos << endl;
	pos.put_piece(BLACK, PAWN, SQ1G);
	cout << pos << endl;*/




	return 0;
}