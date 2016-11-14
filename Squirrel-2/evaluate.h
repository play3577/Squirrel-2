#pragma once
#include "fundation.h"

#include <algorithm>

using namespace std;

struct Position;


namespace Eval {

	//コマ割はやねうら王（やねうら王のコマ割はbonanza6）
	enum :int16_t {
		PawnValue = 86,
		LanceValue = 227,
		KnightValue = 256,
		SilverValue = 365,
		GoldValue = 439,
		BishopValue = 563,
		RookValue = 629,
		ProPawnValue = 540,
		ProLanceValue = 508,
		ProKnightValue = 517,
		ProSilverValue = 502,
		UnicornValue = 826,
		DragonValue = 942,
		KingValue = 15000,
	};

	extern int16_t piece_value[PC_ALL];
	//盤上の駒を取った場合、評価値は相手が駒を失った分＋自分が駒を手に入れた分変動する。
	//評価値は反転されるので符号はこれでいい
	extern int16_t capture_value[PC_ALL];
	//駒が成った分の差分
	extern int16_t diff_promote[GOLD];


	//コマ割全計算
	Value eval_material(const Position& pos);

	//評価値計算
	Value eval(const Position& pos);

};
