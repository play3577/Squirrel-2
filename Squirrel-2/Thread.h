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
	
public:
	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomic�ɂ��邱�ƂŃX���b�h�ϋ������N����Ȃ��悤�ɂ���
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore=Value_Zero;
	//����w�K���ɂ������N����Ă��܂��̂�h��
	CounterMoveHistoryStats CounterMoveHistory;

#ifdef LEARN
	Value l_alpha;
	Value l_beta;
#endif


public:

	std::vector<Move> pv;//�ǂ݋�

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