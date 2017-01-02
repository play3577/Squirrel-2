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
	BAD_CAPTURES,
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
	ExtMove *end_badcaptures = move_ + 600 - 1;
	Stage st;
	const Position& pos_;
	Square recapsq_;
	void generatemove();
	void quietscore();
	void capturepropawn_score();
	void eversion_score();
	Move pick_best(ExtMove* begin, ExtMove* end);
	Value Threshold;
	ExtMove killers[2];
	Move ttMove;
	const Stack* ss;
public:
	//通常探索用コンストラクタ
	movepicker(const Position& pos,Stack* ss_,Move ttm) :pos_(pos),ss(ss_) {

		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
		}
		else {
			st = START_Normal;
		}
		ttMove = (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
		end_ += (ttMove != MOVE_NONE);
	}

	//精子探索用コンストラクタ
	movepicker(const Position& pos, Square recapsq,Move ttm) :pos_(pos) {
		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
			ttMove = (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
			end_ += (ttMove != MOVE_NONE);
		}
		else {
			st = START_Qsearch;
			recapsq_ = recapsq;
			ttMove = MOVE_NONE;
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
	int see_sign()const {
		return st == CAP_PRO_PAWN ? 1 : st == BAD_CAPTURES ? -1 : 0;
	}
};
