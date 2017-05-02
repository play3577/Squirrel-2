#include "game_database.h"
#include "position.h"
#include "misc.h"

#include <sstream>
#include <iomanip>//setw(6)の為


#if 0
std::ostream & operator<<(std::ostream & os, const Game & game)
{
	cout << "this game info" << endl
		<< "先手:" << game.black_P << " 後手:" << game.white_P << endl
		<< "結果" << game.result << endl
		<< "対局日" << game.day << endl
		<< "手数" << game.ply << endl
		<< "指し手" << endl;
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

	//一行目読み込み		<棋譜番号> <対局開始日> <先手名> <後手名> <勝敗(0:引き分け,1:先手勝ち,2:後手勝ち)> <手数> 
	if (!getline(input_stream_, line)) { cout << "read line1 error!" << endl; return false; }

	//一行目の解析
	std::istringstream header(line);
	int game_ply, game_result;
	string gameid;

	//一行目読み込み<棋譜番号> <対局開始日> <先手名> <後手名> <勝敗(0:引き分け,1:先手勝ち,2:後手勝ち)> <手数> 
	header >> gameid >> (game->day) >> (game->black_P) >> (game->white_P) >> game_result >> game_ply;
	game->ply = game_ply;
	game->result = static_cast<GameResult>(game_result);

	//2行目の読み込み（6文字ずつ並んだ指し手）
	if (!getline(input_stream_, line)) { cout << "read line2 error!" << endl;  return false; }

	//指し手の情報から合法手であればその指し手をuint16の指し手情報に変換
	std::istringstream moves(line);
	Position pos; pos.set_hirate();
	StateInfo si[500];
	game->moves.clear();

	for (int ply = 0; ply < game_ply; ply++) {
		std::string move_str;
		moves >> std::setw(6) >> move_str;
		if (!moves) { break; }//正しく読み込めなかった場合は中断をする。
		Move move=CSA2Move(move_str,pos);

		if (move == MOVE_NONE || !pos.is_legal(move)||!pos.pseudo_legal(move)) {
			break;
		}
		
		//指し手をgameに追加
		game->moves.push_back(move);
		//局面を動かす
		pos.do_move(move, &si[ply]);
	}


	return true;
}
#endif