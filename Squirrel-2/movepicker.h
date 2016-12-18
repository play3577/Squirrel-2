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
	const Stack* ss_;
	ExtMove killers_[2];
	Move ttMove_;
public:
	//通常探索用コンストラクタ
	movepicker(const Position& pos,const Stack* ss,const Move ttmove) :pos_(pos),ss_(ss){

		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
			//ttMove_ = MOVE_NONE;
		}
		else {
			st = START_Normal;
			
		}
		ttMove_ = ((ttmove != MOVE_NONE)&&(pos_.is_psuedolegal(ttmove))) ? ttmove : MOVE_NONE;
		end_ += (ttMove_ != MOVE_NONE);
	}

	//精子探索用コンストラクタ
	movepicker(const Position& pos, Square recapsq, const Move ttmove) :pos_(pos), ss_(nullptr) {
		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
			//ttMove_ = MOVE_NONE;
			ttMove_ = ((ttmove != MOVE_NONE) && (pos_.is_psuedolegal(ttmove))) ? ttmove : MOVE_NONE;
			end_ += (ttMove_ != MOVE_NONE);
		}
		else {
			st = START_Qsearch;
			recapsq_ = recapsq;
			//recaptureのみを生成するのでコレでいいが王手も生成するならコレじゃいけない
			ttMove_ = ((ttmove != MOVE_NONE) && (pos_.is_psuedolegal(ttmove)) && (move_to(ttmove) == recapsq_)) ? ttmove : MOVE_NONE;
			end_ += (ttMove_ != MOVE_NONE);
		}
		
	}
	//multicut用コンストラクタ
	movepicker(const Position& pos, Value v) :pos_(pos), ss_(nullptr) {

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
