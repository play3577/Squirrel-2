#pragma once
#include "position.h"
#include "search.h"

enum Stage {

	Start_Multicut,
	Gen_Malticut,
	START_Normal,
	CAP_PRO_PAWN,
	Killers,
	QUIET,
	START_Eversion,
	EVERSION,
	START_Qsearch,
	RECAPTURE,
	STOP,

};
inline Stage operator++(Stage& d, int) { Stage prev = d; d = Stage(int(d) + 1); return prev; }



class movepicker {

private:
	ExtMove move_[600], *current_, *end_;
	Stage st;
	const Position& pos_;
	Square recapsq_;
	void generatemove();
	void quietscore();
	void capturepropawn_score();
	Value Threshold;
	ExtMove killers[2];
	Move ttMove;
	const Stack* ss;
public:
	//通常探索用コンストラクタ
	movepicker(const Position& pos,Stack* ss_) :pos_(pos),ss(ss_) {

		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
		}
		else {
			st = START_Normal;
		}
	}

	//精子探索用コンストラクタ
	movepicker(const Position& pos, Square recapsq) :pos_(pos) {
		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
		}
		else {
			st = START_Qsearch;
			recapsq_ = recapsq;
		}
	}
	//multicut用コンストラクタ
	movepicker(const Position& pos, Value v) :pos_(pos) {

		ASSERT(pos.is_incheck() == false);
		current_ = end_ = move_;
		Threshold = v;//まだ使わない
		st = Start_Multicut;
	}

	inline Stage ret_stage() { return st; }

	Move return_nextmove();

	int num_move() {
		//cout << "num_move " << (end_ - move_) << endl;
		return int(end_ - move_);
	};

};
