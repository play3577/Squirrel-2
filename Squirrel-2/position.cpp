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

/*
�����̍X�V�i���ƂŁj
���X�g�̍����v�Z�i���ƂŁj
��̍����v�Z�i���ƂŁj

*/
void Position::do_move(Move m, StateInfo * newst)
{
	//stateinfo�̍X�V
	memcpy(newst, st, offsetof(StateInfo, lastmove));

	newst->previous = st;
	st = newst;

	//�w����̏��̗p��
	Square to = move_to(m);//�ړ���
	Piece movedpiece = moved_piece(m);//�ړ�������
	Piece capture;
	if (piece_type(pcboard[to]) == KING) {
		cout << *this << endl;
		ASSERT(0);
	}
	//undomove�ׂ̈̏���p��
	st->DirtyPiece[0] = movedpiece;//dirtypiece�ɓ������������
	st->lastmove = m;//����w�����w����

	ASSERT(is_ok(movedpiece));

	if (is_drop(m)) {

		//�ł�̏���
		Piece pt = piece_type(movedpiece);
		Color c = piece_color(movedpiece);
		int num = num_pt(hands[sidetomove()], pt);
		ASSERT(num != 0);

		makehand(hands[sidetomove()], pt, num - 1);//����������ꖇ���炷
		//board�̍X�V
		pcboard[to] = movedpiece;
		//bitboard�̍X�V����
		put_piece(c, pt, to);

	}
	else {
		/*
		�R�}�̈ړ�
		*/
		Square from = move_from(m);
		Color us = sidetomove_;
		Piece pt = piece_type(movedpiece);
		if (pt == KING&&st->Eboard[opposite(us)][to]) {
			cout << *this << endl;
			ASSERT(0);
		}
		ASSERT(movedpiece == pcboard[from]);
		ASSERT(piece_color(movedpiece) == us);

		//�R�}�̈ړ�
		//�R�}�̈ړ�
		pcboard[from] = NO_PIECE;
		capture = pcboard[to];


		//occ�̍X�V����
		remove_piece(us, pt, from);

		
		if (!is_propawn(m)) {
			//�����Ȃ��ꍇ
			pcboard[to] = movedpiece;
			put_piece(us, pt, to);

		}
		else {
			//��������ꍇ
			Piece propt = promotepiece(pt);

			pcboard[to] = promotepiece(movedpiece);
			put_piece(us, propt, to);

		}
		//��̕ߊl
		if (capture != NO_PIECE) {
			st->DirtyPiece[1] = capture;

			Piece pt2 = rowpiece(piece_type(capture));//������������ꍇ�͂Ȃ��ĂȂ���ɖ߂�
			Piece cappt = piece_type(capture);
			if (cappt == KING) {
				cout << *this << endl;
				ASSERT(0);
			}
			Color c = piece_color(capture);
			ASSERT(c != sidetomove());//�����̋������Ă��܂��ĂȂ���
			remove_piece(c, cappt, to);

			int num = num_pt(hands[sidetomove()], pt2);
			makehand(hands[sidetomove()], pt2, num + 1);
		}
		else {
			st->DirtyPiece[1] = NO_PIECE;
		}
		//ksq�̍X�V
		if (pt == KING) { st->ksq_[us] = to; }
	}

	st->ply_from_root++;
	nodes++;
	sidetomove_ = opposite(sidetomove_);//�w�����Ԃ̓���ւ�

}


void Position::undo_move()
{
	/*
	st��previous�ɖ߂��Η����͌��ɖ߂�̂ł����ł�subeffect�Ȃǂ͂��Ȃ��Ă����B
	���̎����łق�Ƃɂ����̂��낤��...��������A�N�Z�X�̒m�����K�v��
	*/

	Move LMove = st->lastmove;
	Square to = move_to(LMove);
	Piece movedpiece = st->DirtyPiece[0];
	Piece capture = st->DirtyPiece[1];

	//��ł��̏ꍇ
	if (is_drop(LMove)) {
		/*
		�ړ���ɂ͂��܂��Ȃ������̂ł��̋����菜��
		*/
		pcboard[to] = NO_PIECE;
		Piece pt = piece_type(movedpiece);
		Color c = piece_color(movedpiece);
		int num = num_pt(hands[c], pt);
		makehand(hands[c], pt, num + 1);

		remove_piece(c, pt, to);
	}
	else {
		//�R�}�̈ړ�
		Square from = move_from(LMove);
		pcboard[from] = movedpiece;
		Piece afterpiece = pcboard[to];
		ASSERT(afterpiece != NO_PIECE);
		pcboard[to] = capture;

		//from��occupied��ǉ�
		Color from_c = piece_color(movedpiece);
		Piece from_pt = piece_type(movedpiece);
		put_piece(from_c, from_pt, from);

		//��̕ߊl������ꍇ
		if (capture != NO_PIECE) {
			Piece pt;

			pt = rowpiece(piece_type(capture));//capture���琬�Ǝ�Ԃ����� ���ꂽ��Ȃ̂Ő���͏����Ȃ���΂Ȃ�Ȃ�

			Color enemy = opposite(sidetomove());
			int num = num_pt(hands[enemy], pt);
			makehand(hands[enemy], pt, num - 1);//������̏��������������B
			Piece cappt = piece_type(capture);
			Color c = piece_color(capture);
			//�ߊl����Ă�����
			put_piece(c, cappt, to);
			//���Ƃ��Ƃ�����̕�
			remove_piece(from_c, piece_type(afterpiece), to);
			ASSERT(from_c != c);



		}
		else {
			//��̕ߊl�̂Ȃ��ꍇ
			Piece pt = piece_type(afterpiece);
			Color c = piece_color(afterpiece);
			//to�ɂ���������������ł���
			remove_piece(c, pt, to);
		}
	}
	sidetomove_ = opposite(sidetomove_);

	st = st->previous;


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
	os << "occ all" << endl << pos.occ_all() << endl;

	for (Color c = BLACK; c <= WHITE; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			os << " color " << c << " pt " << pt << endl << pos.occ_pt(c, pt) << endl;
		}
	}
	
	os << " ��� " << pos.sidetomove() << endl;

	

	return os;
}
