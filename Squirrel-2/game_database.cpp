#include "game_database.h"
#include "position.h"
#include "misc.h"

#include <sstream>
#include <iomanip>//setw(6)�̈�


#if 0
std::ostream & operator<<(std::ostream & os, const Game & game)
{
	cout << "this game info" << endl
		<< "���:" << game.black_P << " ���:" << game.white_P << endl
		<< "����" << game.result << endl
		<< "�΋Ǔ�" << game.day << endl
		<< "�萔" << game.ply << endl
		<< "�w����" << endl;
	for (auto move : game.moves) {
		cout << move << endl;
	}

	cout << "over" << endl;

	return os;
}

bool GameDataStream::read_onegame(Game * game)
{
	ASSERT(game != nullptr);


	std::string line;

	//��s�ړǂݍ���		<�����ԍ�> <�΋ǊJ�n��> <��薼> <��薼> <���s(0:��������,1:��菟��,2:��菟��)> <�萔> 
	if (!getline(input_stream_, line)) { cout << "read line1 error!" << endl; return false; }

	//��s�ڂ̉��
	std::istringstream header(line);
	int game_ply, game_result;
	string gameid;

	//��s�ړǂݍ���<�����ԍ�> <�΋ǊJ�n��> <��薼> <��薼> <���s(0:��������,1:��菟��,2:��菟��)> <�萔> 
	header >> gameid >> (game->day) >> (game->black_P) >> (game->white_P) >> game_result >> game_ply;
	game->ply = game_ply;
	game->result = static_cast<GameResult>(game_result);

	//2�s�ڂ̓ǂݍ��݁i6���������񂾎w����j
	if (!getline(input_stream_, line)) { cout << "read line2 error!" << endl;  return false; }

	//�w����̏�񂩂獇�@��ł���΂��̎w�����uint16�̎w������ɕϊ�
	std::istringstream moves(line);
	Position pos; pos.set_hirate();
	StateInfo si[500];
	game->moves.clear();

	for (int ply = 0; ply < game_ply; ply++) {
		std::string move_str;
		moves >> std::setw(6) >> move_str;
		if (!moves) { break; }//�������ǂݍ��߂Ȃ������ꍇ�͒��f������B
		Move move=CSA2Move(move_str,pos);

		if (move == MOVE_NONE || !pos.is_legal(move)||!pos.pseudo_legal(move)) {
			break;
		}
		
		//�w�����game�ɒǉ�
		game->moves.push_back(move);
		//�ǖʂ𓮂���
		pos.do_move(move, &si[ply]);
	}


	return true;
}
#endif