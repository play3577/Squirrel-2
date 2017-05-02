#pragma once
#include "position.h"
#include "search.h"

enum Stage {

	//Start_Multicut,
	//Gen_Malticut,
	Start_Probcut,
	Gen_Probcut,
	START_Normal,
	CAP_PRO_PAWN,
	Killers,
	QUIET,
	BAD_CAPTURES,
	START_Eversion,
	EVERSION,
	START_Q_RECAPTURE,
	RECAPTURE,
	START_Q_CAP_PROPAWN,
	Q_CAP_PROPAWN,
	START_Q_WITH_CHECKS,
	Q_CAP_PROPAWN_2,
	Q_CHECKS,
	STOP,

};
inline Stage operator++(Stage& d, int) { Stage prev = d; d = Stage(int(d) + 1); return prev; }



class movepicker {

private:
	ExtMove move_[600], *current_=move_, *end_=move_;
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
	ExtMove killers[3];
	Move ttMove;
	const Stack* ss;
	Depth depth_;
	Move countermove;
public:
	//通常探索用コンストラクタ
	movepicker(const Position& pos, Stack* ss_, Move ttm, Depth d);

	//精子探索用コンストラクタ
	movepicker(const Position& pos, Square recapsq, Move ttm,Depth d, Stack * ss_);
	////multicut用コンストラクタ
	//movepicker(const Position& pos, Value v) :pos_(pos) {

	//	ASSERT(pos.is_incheck() == false);
	//	current_ = end_ = move_;
	//	Threshold = v;//まだ使わない
	//	st = Start_Multicut;
	//}

	movepicker(const Position& pos, Move ttm, Value th) :pos_(pos), Threshold(th) {


		ASSERT(pos.is_incheck() == false);
		current_ = end_ = move_;
		st = Start_Probcut;

		ttMove = (ttm != MOVE_NONE
			&&pos_.pseudo_legal(ttm)
			&& pos_.capture(ttm)
			//&&pos_.capture_or_propawn(ttm)
			&& pos.see_ge(ttm,Threshold))? ttm : MOVE_NONE;

		end_ += (ttMove != MOVE_NONE);
	}


	inline Stage ret_stage() { return st; }

	Move return_nextmove(bool skipQuiets = false);

	int num_move() {
		//cout << "num_move " << (end_ - move_) << endl;
		return int(end_ - move_);
	};
	int see_sign()const {
		return st == CAP_PRO_PAWN ? 1 : st == BAD_CAPTURES ? -1 : 0;
	}
};
