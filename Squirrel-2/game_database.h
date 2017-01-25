#pragma once

//技巧の学習形式に直してくれるプログラムfg2gdbがあるのでここは技巧方式にする（申し訳ない）(それにしても素晴らしい実装だ)

#include "fundation.h"

#include <istream>
#include <map>
#include <vector>
using namespace std;



#if defined(_MSC_VER)
#define gamedatabasefile  "C:/book2/records_sum.txt"
#define nichkihu "C:/book2/2chkifu.csa1"
#define fg2800_2ch "C:/book2/fg_2800_2chkihu.csa"
#endif
#if defined(__unix__) 
#define gamedatabasefile "/home/daruma/fvPP/records_sum.txt"
#endif

enum GameResult
{
	black_win = 1,
	white_win = 2,
	draw = 0,
};

//一試合分のデータを保持するための構造体

struct Game
{
	//player
	string white_P, black_P;
	//結果
	GameResult result;
	//対局日 
	string day;
	//指し手
	vector<Move> moves;
	int ply;
};


//デバッグ用。
std::ostream& operator<<(std::ostream& os, const Game& game);


//ゲームデータストリーム
class GameDataStream {

private:
	istream& input_stream_;

public:
	GameDataStream(istream& is) : input_stream_(is) {}
	bool read_onegame(Game* game);//一局分のデータを読みだす。（まだ残りがあればtrue）


};