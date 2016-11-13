#pragma once
#include "position.h"


enum Stage {

	START_Normal,
	CAP_PRO_PAWN,
	QUIET,
	START_Eversion,
	EVERSION,
	STOP,

};
inline Stage operator++(Stage& d, int) { Stage prev = d; d = Stage(int(d) + 1); return prev; }



class movepicker {

private:
	ExtMove move_[600], *current_, *end_;
	Stage st;
	const Position& pos_;

	void generatemove();

public:
	movepicker(const Position& pos) :pos_(pos) {

		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
		}
		else {
			st = START_Normal;
		}
	}

	Move return_nextmove();

};
