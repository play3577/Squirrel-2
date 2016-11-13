#pragma once
#include "fundation.h"
#include "position.h"
#include "search.h"
#include <vector>

using namespace std;

struct Thread {

private:
	Position rootpos;//rootpos�̓X���b�h���ɕۂK�v������̂ŎQ�Ɠn���ł͂����Ȃ��C������B
	int rootdepth;
	ExtMove RootMoves[200], *end;

public:

	std::vector<Move> pv;//�ǂ݋�

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