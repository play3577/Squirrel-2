#pragma once

#include "fundation.h"
#include "position.h"
#include "misc.h"

enum Nodetype {
	Root,
	PV,
	NonPV,

};


//pvの為にSearchStackを導入
struct Stack {
	//Move pvmove=MOVE_NONE;
	Move *pv;
};


//時間制限を格納するための構造体。
//コレはグローバルにして共有できるようにする。
struct SearchLimit {
	SearchLimit() { memset(this, 0, sizeof(SearchLimit)); }
	TimePoint starttime;
};

extern SearchLimit limit;

inline Value mated_in_ply(int ply) { return Value(Value_Mated + ply); }

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);