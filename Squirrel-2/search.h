#pragma once

#include "fundation.h"
#include "position.h"
#include "misc.h"

#include <atomic>

enum Nodetype {
	Root,
	PV,
	NonPV,

};

struct Signal {

	std::atomic_bool stop;

};


//pvの為にSearchStackを導入
struct Stack {
	//Move pvmove=MOVE_NONE;
	Move *pv;
	int ply = 0;
	Move excludedMove = MOVE_NONE;
	bool skip_early_prunning = false;
};


//時間制限を格納するための構造体。
//コレはグローバルにして共有できるようにする。
struct SearchLimit {
	SearchLimit() { memset(this, 0, sizeof(SearchLimit)); }
	TimePoint starttime;
	TimePoint remain_time[ColorALL];//残り時間
	TimePoint byoyomi;//秒読み時間
	TimePoint inc_time;//フィッシャールール用
	TimePoint endtime;
	bool is_ponder = false;
};

//デバッグ用関数　こんなの実装しなくてもいいかもしれないが無いよりあったほうがいいだろう
inline std::ostream& operator<<(std::ostream& os, const SearchLimit& sl) {

	os << " starttime " << sl.starttime << endl;
	os << "remain black " << sl.remain_time[BLACK] << " white " << sl.remain_time[WHITE] << endl;
	if (sl.byoyomi != 0)
	{
		os << " byoyomi " << sl.byoyomi << endl;
	}
	if (sl.inc_time != 0) {
		os << sl.inc_time << endl;
	}
	return os;
}

extern SearchLimit limit;

inline Value mated_in_ply(int ply) { return Value(Value_Mated + ply); }
inline Value mate_in_ply(int ply) { return Value(Value_Mate - ply); }
template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);

template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);