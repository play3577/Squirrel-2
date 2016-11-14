#pragma once
#include "fundation.h"

#include <algorithm>

using namespace std;

struct Position;


namespace Eval {

	//�R�}���͂�˂��牤�i��˂��牤�̃R�}����bonanza6�j
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
	//�Տ�̋��������ꍇ�A�]���l�͑��肪������������{�����������ɓ��ꂽ���ϓ�����B
	//�]���l�͔��]�����̂ŕ����͂���ł���
	extern int16_t capture_value[PC_ALL];
	//����������̍���
	extern int16_t diff_promote[GOLD];


	//�R�}���S�v�Z
	Value eval_material(const Position& pos);

	//�]���l�v�Z
	Value eval(const Position& pos);

};
