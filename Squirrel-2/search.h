#pragma once

#include "fundation.h"
#include "position.h"
#include "misc.h"

#include <atomic>

enum Nodetype {
	Root,
	PV,
	NonPV,

};

struct Signal {

	std::atomic_bool stop;

};


//pv�ׂ̈�SearchStack�𓱓�
struct Stack {
	//Move pvmove=MOVE_NONE;
	Move *pv;
	int ply = 0;
	Move excludedMove = MOVE_NONE;
	bool skip_early_prunning = false;
	Move killers[2];
};


//���Ԑ������i�[���邽�߂̍\���́B
//�R���̓O���[�o���ɂ��ċ��L�ł���悤�ɂ���B
struct SearchLimit {
	SearchLimit() { memset(this, 0, sizeof(SearchLimit)); }
	TimePoint starttime;
	TimePoint remain_time[ColorALL];//�c�莞��
	TimePoint byoyomi;//�b�ǂݎ���
	TimePoint inc_time;//�t�B�b�V���[���[���p
	TimePoint endtime;
	bool is_ponder = false;
};

//�f�o�b�O�p�֐��@����Ȃ̎������Ȃ��Ă�������������Ȃ���������肠�����ق����������낤
inline std::ostream& operator<<(std::ostream& os, const SearchLimit& sl) {

	os << " starttime " << sl.starttime << endl;
	os << "remain black " << sl.remain_time[BLACK] << " white " << sl.remain_time[WHITE] << endl;
	if (sl.byoyomi != 0)
	{
		os << " byoyomi " << sl.byoyomi << endl;
	}
	if (sl.inc_time != 0) {
		os << sl.inc_time << endl;
	}
	return os;
}

extern SearchLimit limit;

inline Value mated_in_ply(int ply) { return Value(Value_Mated + ply); }
inline Value mate_in_ply(int ply) { return Value(Value_Mate - ply); }
template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);

template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);