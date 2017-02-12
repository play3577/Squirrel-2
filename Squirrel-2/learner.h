#pragma once

#include "evaluate.h"
#include <fstream>
#include "Thread.h"
namespace Eval {

	void write_PP();

	void initialize_PP();


	//�p�����[�^�[�̍X�V�̂��߂̊֐�
	//void renewal_PP(const Position& pos, const double **dJ);

	void learner();

	void parallel_learner();

	struct MoveInfo {

		Move move;
		vector<Move> pv;
		Value score;

		MoveInfo(const Move m, const vector<Move> pv_, Value s) {
			move = m; pv = pv_; score = s;
		}

	};


	
}

int sign(const double d);

bool swapmove(ExtMove* moves, const int num, const Move m);

void learnphase1();
void learnphase1body(int number);

void learnphase2();
void learnphase2body();



