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
	ExtMove RootMoves[600], *end;
public:
	std::atomic_bool resetCalls;//atomicにすることでスレッド観競合が起こらないようにする
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
public:

	std::vector<Move> pv;//読み筋

	void set(Position pos);
	int rdepth() { return rootdepth; }
	Value think();

	ExtMove* find_rootmove(Move object) {
		for (ExtMove* i = RootMoves; i < end; i++) {
			if (i->move == object) { return i; }
		}
		return nullptr;
	}
	void sort_RootMove();
	void print_pv(const int depth,const Value v);

};