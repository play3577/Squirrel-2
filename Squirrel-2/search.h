#pragma once

#include "fundation.h"
#include "position.h"
#include "misc.h"

enum Nodetype {
	Root,
	PV,
	NonPV,

};


//pv�ׂ̈�SearchStack�𓱓�
struct Stack {
	//Move pvmove=MOVE_NONE;
	Move *pv;
};


//���Ԑ������i�[���邽�߂̍\���́B
//�R���̓O���[�o���ɂ��ċ��L�ł���悤�ɂ���B
struct SearchLimit {
	SearchLimit() { memset(this, 0, sizeof(SearchLimit)); }
	TimePoint starttime;
};

extern SearchLimit limit;

inline Value mated_in_ply(int ply) { return Value(Value_Mated + ply); }

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);