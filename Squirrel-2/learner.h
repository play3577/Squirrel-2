#pragma once

#include "evaluate.h"
#include <fstream>
#include "Thread.h"
namespace Eval {

	void write_PP();

	void initialize_PP();


	//パラメーターの更新のための関数
	//void renewal_PP(const Position& pos, const double **dJ);

	void learner();



	struct MoveInfo {

		Move move;
		vector<Move> pv;
		Value score;

		MoveInfo(const Move m, const vector<Move> pv_, Value s) {
			move = m; pv = pv_; score = s;
		}

	};
















}