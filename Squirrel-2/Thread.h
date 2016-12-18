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
	Position rootpos;//rootpos�̓X���b�h���ɕۂK�v������̂ŎQ�Ɠn���ł͂����Ȃ��C������B
	int rootdepth;
	//rootmoves�ɔ񍇖@�肪�����Ă���̂�h���Ȃ���΂Ȃ�Ȃ�
	ExtMove RootMoves[600], *end;
public:
	std::atomic_bool resetCalls;//atomic�ɂ��邱�ƂŃX���b�h�ϋ������N����Ȃ��悤�ɂ���
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
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
	void print_pv(const int depth,const Value v);

};