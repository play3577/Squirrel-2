#pragma once

#include "fundation.h"
#include "position.h"
#include "misc.h"
#include "moveStats.h"
#include <atomic>

enum Nodetype {
	Root,
	PV,
	NonPV,

};

struct Signal {

	std::atomic_bool stop,stopOnPonderHit;

};

void search_clear(Thread& th);

//pvの為にSearchStackを導入
struct Stack {
	//Move pvmove=MOVE_NONE;
	Move *pv;
	int ply = 0;
	Move excludedMove = MOVE_NONE;
	Move currentMove=MOVE_NONE;
	bool skip_early_prunning = false;
	Move killers[2];
	Value static_eval;
	int moveCount;
	CounterMoveStats* counterMoves=nullptr;//countermovehistorytableのentryへのポインタを格納する。
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
	TimePoint opitmumTime;
	bool is_inponder = false;
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
extern Signal signal;
void search_init();

inline Value mated_in_ply(int ply) { return Value(Value_Mated + ply); }
inline Value mate_in_ply(int ply) { return Value(Value_Mate - ply); }
