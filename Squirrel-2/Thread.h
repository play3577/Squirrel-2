#pragma once
#include "fundation.h"
#include "position.h"
#include "search.h"
#include <vector>
#include <atomic>
#include "moveStats.h"
using namespace std;

struct Thread {

private:
	Position rootpos;//rootposはスレッド毎に保つ必要があるので参照渡しではいけない気がする。
	int rootdepth;
	//rootmovesに非合法手が入ってくるのを防がなければならない
	
public:
	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomicにすることでスレッド観競合が起こらないようにする
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore=Value_Zero;
	//並列学習中にここが侵されてしまうのを防ぐ
	CounterMoveHistoryStats CounterMoveHistory;

#ifdef LEARN
	Value l_alpha;
	Value l_beta;
#endif


public:

	std::vector<Move> pv;//読み筋

	void set(Position pos);
	int rdepth() { return rootdepth; }
	Value think();

	ExtMove* find_rootmove(const Move object) {
		for (ExtMove* i = RootMoves; i < end; i++) {
			if (i->move == object) { return i; }
		}
		return nullptr;
	}
	void sort_RootMove();
	void print_pv(const int depth, Value v);
	void cleartable() {
		history.clear();
		counterMoves.clear();
		fromTo.clear();
		CounterMoveHistory.clear();
	}
};