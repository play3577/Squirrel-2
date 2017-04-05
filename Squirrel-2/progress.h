#pragma once

#include "position.h"
#include "evaluate.h"
#if 0
namespace Progress {

	/*
	進行度を見るにはKPが一番いいって大会で出村さんが言ってた。（理由はわからん）
	*/
	extern ALIGNED(32) int32_t prog_KP[SQ_NUM][Eval::fe_end];

	constexpr  int paramscale = 1 << 8;

	constexpr Value progEG = Value(800);
	constexpr  int prog_scale = 4098;
	void initialize_KP();//乱数で初期化する
	void read_KP();
	void write_KP();

	void learner();

	double calc_prog(const Position& pos);
	double calc_diff_prog(const Position& pos);
}

#endif