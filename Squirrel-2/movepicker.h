#pragma once
#include "position.h"


enum Stage {

	Start_Multicut,
	Gen_Malticut,
	START_Normal,
	CAP_PRO_PAWN,
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
public:
	//�ʏ�T���p�R���X�g���N�^
	movepicker(const Position& pos) :pos_(pos) {

		current_ = end_ = move_;

		if (pos.is_incheck()) {
			st = START_Eversion;
		}
		else {
			st = START_Normal;
		}
	}

	//���q�T���p�R���X�g���N�^
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
	//multicut�p�R���X�g���N�^
	movepicker(const Position& pos, Value v) :pos_(pos) {

		ASSERT(pos.is_incheck() == false);
		current_ = end_ = move_;
		Threshold = v;//�܂��g��Ȃ�
		st = Start_Multicut;
	}

	inline Stage ret_stage() { return st; }

	Move return_nextmove();

	int num_move() {
		//cout << "num_move " << (end_ - move_) << endl;
		return int(end_ - move_);
	};

};
