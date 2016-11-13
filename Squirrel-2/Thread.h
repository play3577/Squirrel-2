#pragma once
#include "fundation.h"
#include "position.h"
#include "search.h"
#include <vector>

using namespace std;

struct Thread {

private:
	Position rootpos;//rootposはスレッド毎に保つ必要があるので参照渡しではいけない気がする。
	int rootdepth;
	ExtMove RootMoves[200], *end;

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
	void print_pv(const int depth, Stack* ss);

};