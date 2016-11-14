#include "misc.h"
#include "fundation.h"

File Sfen2File(const char c) {

	return File(c - '1');

}
Rank Sfen2Rank(const char c) {
	return Rank(c - 'a');
}


Move Sfen2Move(const string smove, const Position& pos)
{
	//example 2c2b+ P*1a
	ASSERT(smove.size() <= 5);

	Color turn = pos.sidetomove();
	Move m;
	
	if (isdigit(smove[0])) {

		//Å‰‚ª•¶Žš‚Å‚ ‚ê‚ÎƒRƒ}‚ÌˆÚ“®

		File ff = Sfen2File(smove[0]);
		Rank fr = Sfen2Rank(smove[1]);
		Square from = make_square(fr, ff);
		File tf = Sfen2File(smove[2]);
		Rank tr = Sfen2Rank(smove[3]);
		Square to = make_square(tr, tf);

		Piece pc = pos.piece_on(from);
		ASSERT(pc != NO_PIECE);

		if (smove.size() == 5 && smove[4] == '+') {
			m =make_movepromote(from, to, pc);
		}
		else {
			m = make_move(from, to, pc);
		}
	}
	else {

		//‹î‘Å‚¿
		ASSERT(smove[1] == '*');

		Piece pt = Sfen2Piece_.sfen_to_piece(smove[0]);
		Piece pc = add_color(pt, turn);
		Square to = make_square(Sfen2Rank(smove[3]), Sfen2File(smove[2]));
		ASSERT(num_pt(pos.hand(turn), pt));
		m = make_drop(to, pc);
	}

	cout << m << endl;
	check_move(m);
	return m;

}