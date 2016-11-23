#include "misc.h"
#include "fundation.h"

#include <sstream>
#include <random>

CSA2Piece CSA2Piece_;

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

		//�ŏ��������ł���΃R�}�̈ړ�

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

		//��ł�
		ASSERT(smove[1] == '*');

		Piece pt = Sfen2Piece_.sfen_to_piece(smove[0]);
		Piece pc = add_color(pt, turn);
		Square to = make_square(Sfen2Rank(smove[3]), Sfen2File(smove[2]));
		ASSERT(num_pt(pos.hand(turn), pt));
		m = make_drop(to, pc);
	}

	//cout << m << endl;
	check_move(m);
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

//smove[0] fromfile [1]torank [2]tofile [3]torank [4][5]piece
Move CSA2Move(const string smove, const Position& pos)
{
	//�����񂪒�������
	if (smove.size() >= 7) {
		return MOVE_NONE;
	}

	Move m;


	//��ł��̏ꍇ���l������̂Ő�Ɉړ��悩�琶������B
	const File tofile = CSA2File(smove[2]);
	const Rank torank = CSA2Rank(smove[3]);

	const Square to = make_square(torank, tofile);

	//���
	const string st_pt(smove.begin() + 4, smove.end());
	if (CSA2Piece_.is_ok(st_pt) == false) {
		return MOVE_NONE;
	}
	const Piece pc = add_color(CSA2Piece_.csa_to_piece(st_pt), pos.sidetomove());


	if (smove[0] == '0' && smove[1] == '0') {
		//�R�}�ł��̏ꍇ						 
		return  m = make_drop(to, pc);
	}
	else {
		//��ړ��̏ꍇ
		const File fromfile = CSA2File(smove[0]);
		const Rank fromran = CSA2Rank(smove[1]);
		const Square from = make_square(fromran, fromfile);
		const Piece fromPC = pos.piece_on(from);
		if (fromPC == NO_PIECE) { return MOVE_NONE; }

		//���������͂ǂ��Ŕ��ʂ��Ă���I�H�H
		//��pos��from�ɂ�����pc���Ⴄ���ǂ����œ����I�ɔ��肳���Ȃ��Ƃ����Ȃ��݂���
		
		if (fromPC == pc) {
			return  m = make_move(from, to, pc);
		}
		else if(fromPC+PROMOTE==pc){
			//move�̏��Ɋi�[����̂͐���O�̋��ł���
			return  m = make_movepromote(from, to, fromPC);
		}
		else {
			return MOVE_NONE;
		}
	}

	return MOVE_NONE;
}



inline double normal_dist(double mean, double stddiv)
{
	std::default_random_engine generator;
	std::normal_distribution<double> dist(mean, stddiv);

	return dist(generator);
}