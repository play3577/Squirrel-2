#include "reinforce_learner.h"
#include "position.h"
#include <random>
#include <sstream>
#include <fstream>

/*
�����ł̓����_���ɏ����ǖʂ�p�ӂ�sfen������ɕϊ����A�t�@�C���ɏ����o���B
packedsfen�̂ق���������������Ȃ����܂���sfen�ō쐬����

�����_���Ƃ����Ă�
������x�̃��[���͕K�v���낤

�܂����肪�������Ă��Ȃ�����
���̋ǖʂ̕]���l��500�𒴂��Ȃ����Ɓi�������͐i�s�x���g���ď��Ղł��邱�Ƃ��m�F���Ă����������j



...�܂����ꂮ�炢���H�H

����ς菉���ǖʂȂ̂Ŏ��w���猩��4�i�ڂ܂łȂǂƂ����������K�v��...
�����ǖʂɐ����p�ӂ��ׂ���..??
���`�`�񏉊��ǖʂɂȂ������Ə����ǖʂ̕]���l���傫���Ȃ肷���Ă��܂��悤�ȋC������̂�
�����ǖʂɂȂ�����邱�Ƃ͂��Ȃ��ł���


������2�����h���Ȃ��Ƃ����Ȃ���....

������͂Ȃ��ق���������..
*/
string Position::random_startpos()
{
	clear();
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	std::uniform_int_distribution<int> rand_color(BLACK, WHITE);
	std::uniform_int_distribution<int> is_hand_piece(0,1);//9�Ŏ育��
	std::uniform_int_distribution<int> rand_file(0, 8);
	std::uniform_int_distribution<int> rand_rank(0, 3);
	std::uniform_int_distribution<int> rand_king(SQ_ZERO, SQ_NUM-1);
	string sfen;

	int count = 0;
	//pawn
	for (int i = 0; i < 9; i++) {
		
		const Color c = BLACK;
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], PAWN, num_pt(hands[c], PAWN) + 1); count++; break; }
			else {

				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);

				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq] & ExistPawnBB[c]).isNot()) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(PAWN, c);
					put_piece(c, PAWN, sq);
					set_occ256(sq);
					ExistPawnBB[c] |= FileBB[sqtofile(sq)];
					count++;
					break;
				}
				
			}
		}
	}
	for (int i = 0; i < 9; i++) {

		const Color c = WHITE;
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], PAWN, num_pt(hands[c], PAWN) + 1); count++; break; }
			else {

				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);

				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq] & ExistPawnBB[c]).isNot()) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(PAWN, c);
					put_piece(c, PAWN, sq);
					set_occ256(sq);
					ExistPawnBB[c] |= FileBB[sqtofile(sq)];
					count++;
					break;
				}

			}
		}
	}
	//lance
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], LANCE, num_pt(hands[c], LANCE) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(LANCE, c);
					put_piece(c, LANCE, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//knight
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], KNIGHT, num_pt(hands[c], KNIGHT) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq]&CantGo_KNIGHT[c]).isNot()) { continue; }
				else {
					pcboard[sq] = add_color(KNIGHT, c);
					put_piece(c, KNIGHT, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//silver
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], SILVER, num_pt(hands[c], SILVER) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(SILVER, c);
					put_piece(c, SILVER, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//gold
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], GOLD, num_pt(hands[c], GOLD) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(GOLD, c);
					put_piece(c, GOLD, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//bishop
	for (int i = 0; i < 2; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], BISHOP, num_pt(hands[c], BISHOP) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(BISHOP, c);
					put_piece(c, BISHOP, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//rook
	for (int i = 0; i < 2; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], ROOK, num_pt(hands[c], ROOK) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(ROOK, c);
					put_piece(c, ROOK, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//����
	while (true)
	{
		const File f = (File)rand_file(engine);
		const Rank r = (Rank)(8 - rand_rank(engine));
		const Square ksq = make_square(r, f);
		if(piece_on(ksq) != NO_PIECE) { continue; }
		if (attackers_to(WHITE, ksq, occ256).isNot()) { continue; }//����̌����������Ă��Ă͂����Ȃ�
		pcboard[ksq] = add_color(KING, BLACK);
		put_piece(BLACK, KING, ksq);
		set_occ256(ksq);
		count++;
		break;
	}
	while (true)
	{
		const File f = (File)rand_file(engine);
		const Rank r = (Rank)rand_rank(engine);
		const Square ksq = make_square(r, f);
		if (piece_on(ksq) != NO_PIECE) { continue; }
		if (attackers_to(BLACK, ksq, occ256).isNot()) { continue; }//����̌����������Ă��Ă͂����Ȃ�
		pcboard[ksq] = add_color(KING, WHITE);
		put_piece(WHITE, KING, ksq);
		set_occ256(ksq);
		count++;
		break;
	}
	ASSERT(count == 40);
	ASSERT(is_incheck() == false);
	/*cout << *this << endl;
	cout << count << endl;*/
	sfen = make_sfen();
	set(sfen);
	
	//���܂�]���l������Ă���Ə����ǖʂƂ��ĕs�K�؂ƍl������B
	if (abs(Eval::eval(*this)) > 100) {
		return to_string(0);
	}
	return sfen;
}

void make_startpos_detabase()
{
	Position pos;
	vector<string> db;

	int64_t num;
	cout << "num:";
	cin >> num;
	int64_t i = 0;
	string str;
	
	//db�쐬�B
	while (true) {
		str = pos.random_startpos();

		for each (string s in db){if (str == s) { goto NEXT; }}//�����ǖʂ�����΍ēo�^�͂��Ȃ�
		if (str.size() < 3) { continue; }
		else {
			db.push_back(str);
			i++;
		}
		if (i % 1000 == 0) { cout << i << endl; }
		if (i % 20000 == 0) { 
			
			ofstream of1("C:/sp_db/startpos.db");
			for (int j = 0; j < db.size(); j++) {

				of1 << db[j] << endl;
			}
			of1.close();
		}
		if (i > num) { break; }

	NEXT:;
	}

	//�t�@�C���ɏ����o��
	
	ofstream of("C:/sp_db/startpos.db");

	for (int j = 0; j < db.size(); j++) {
		
		of << db[j] << endl;
	}
	of.close();
	cout << "finish make database!" << endl;
}
