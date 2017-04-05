#pragma once

#include "position.h"
#include "evaluate.h"
#if 0
namespace Progress {

	/*
	�i�s�x������ɂ�KP����Ԃ������đ��ŏo�����񂪌����Ă��B�i���R�͂킩���j
	*/
	extern ALIGNED(32) int32_t prog_KP[SQ_NUM][Eval::fe_end];

	constexpr  int paramscale = 1 << 8;

	constexpr Value progEG = Value(800);
	constexpr  int prog_scale = 4098;
	void initialize_KP();//�����ŏ���������
	void read_KP();
	void write_KP();

	void learner();

	double calc_prog(const Position& pos);
	double calc_diff_prog(const Position& pos);
}

#endif