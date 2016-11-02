#include "position.h"
#include "misc.h"

#include <sstream>

Sfen2Piece Sfen2Piece_;

//"sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
void Position::set(std::string sfen)
{
	//�����ŏ����������Ă����K�v������I

	clear();

	std::istringstream ss(sfen);

	string dummy;
	ss >> dummy;//sfen��ǂݔ�΂�
	std::string s_sfen, s_teban, s_hands, token;
	ss >> s_sfen;
	ss >> s_teban;
	ss >> s_hands;
	/*if (token[0] != '-') {
		s_hands = token;
	}*/

	//�Ō�̂P�͖������Ă�����
	int index = 0;
	bool promote = false;
	char s;
	Piece pc;
	Rank r = RankA;
	File f = File9;
	while (r < Rank_Num&&index < s_sfen.size()) {

		s = s_sfen[index];

		//�����ł���΂��̃}�X���������߂�B
		if (isdigit(s)) {
			f = f - (s - '0');
		}
		// /�ł����rank�𑝂₵�ā@�t�@�C����߂��B
		else if (s == '/') {
			r++;
			f = File9;
		}
		//�Ȃ�̃t���O�𗧂Ă�
		else if(s=='+'){
			promote = true;
		}
		else if (pc = Sfen2Piece_.sfen_to_piece(s)) {
			//�����œ����ė���pc�͐����܂�ł��Ȃ��͂��Ȃ̂Ő��ł���΂��̏���t�^����B
			if (promote) {
				pc = promotepiece(pc);
			}
			Piece pt = piece_type(pc);
			Color c = piece_color(pc);
			Square sq = make_square(r, f);

			pcboard[sq] = pc;//OK
			occupied[c] |= SquareBB[sq];
			occupiedPt[c][pt] |= SquareBB[sq];
			promote = false;
			f--;
		}
		index++;
	}//board

	 //���
	if (s_teban.size()) {
		s_teban != "w" ? sidetomove_ = BLACK : sidetomove_ = WHITE;
	}
	//������
	index = 0;
	int num = 1;
	int num2;
	bool before_isdigit=false;
	while (index < s_hands.size()) {

		s = s_hands[index];
		if (s == '-') { break; }
		//hand�͂Q���ɂ����肤��I�I�I
		if (isdigit(s)) {
			if (!before_isdigit) {
				num = (s - '0');
				//ASSERT(num != 1);
			}
			if (before_isdigit) {
				num2= (s - '0');
				num = num * 10 + num2;
			}
			before_isdigit = true;
		}
		else if (pc = Sfen2Piece_.sfen_to_piece(s)) {

			before_isdigit = false;
			Piece pt = piece_type(pc);
			Color c = piece_color(pc);

			makehand(hands[c], pt, num);

			num = 1;
		}
		index++;
	}
#ifdef CHECKPOS
	cout << *this << endl;
#endif
}

void Position::clear()
{
	std::memset(this, 0, sizeof(Position));
	this->st = &initSt;
	std::memset(this->st, 0, sizeof(StateInfo));
}

//�������o�O���`�����₷���悤��xor�ɂ��������������H
void Position::remove_piece(const Color c, const Piece pt, const Square sq)
{
	/*occupiedPt[c][pt].andnot(SquareBB[sq]);
	occupied[c].andnot(SquareBB[sq]);
	occupied[2].andnot(SquareBB[sq]);*/
	occupiedPt[c][pt]^=(SquareBB[sq]);
	occupied[c]^=(SquareBB[sq]);
	//occupied[2]^=(SquareBB[sq]);
}

void Position::put_piece(const Color c, const Piece pt, const Square sq)
{
	/*occupiedPt[c][pt] |= SquareBB[sq];
	occupied[c] |= SquareBB[sq];
	occupied[2] |= SquareBB[sq];*/
	occupiedPt[c][pt] ^= (SquareBB[sq]);
	occupied[c] ^= (SquareBB[sq]);
	//occupied[2] ^= (SquareBB[sq]);
}

void Position::do_move(Move m, StateInfo * newst)
{
}

void Position::undo_move()
{
}


std::ostream & operator<<(std::ostream & os, const Position & pos)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {
			os << outputPiece[pos.piece_on(make_square(r, f))];
		}
		os << endl;
	}
	for (Color c = BLACK; c < ColorALL; c++) {
		(c == BLACK) ? os << "    ��� " << std::endl : os << "    ��� " << std::endl;
		os << pos.hand(c) << endl;
	}
	for (Color c = BLACK; c <= WHITE; c++) {
		os << " color " << c << endl << pos.occ(c) << endl;
	}
	for (Color c = BLACK; c <= WHITE; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			os << " color " << c << " pt " << pt << endl << pos.occ_pt(c, pt) << endl;
		}
	}
	os << "occ all" << endl << pos.occ_all() << endl;
	os << " ��� " << pos.sidetomove() << endl;

	

	return os;
}
