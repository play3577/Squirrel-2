#pragma once
#include "fundation.h"
#include <algorithm>
// The Stats struct stores moves statistics. According to the template parameter
// the class can store History and Countermoves. History records how often
// different moves have been successful or unsuccessful during the current search
// and is used for reduction and move ordering decisions.
// Countermoves store the move that refute a previous one. Entries are stored
// using only the moving piece and destination square, hence two moves with
// different origin but same destination and piece will be considered identical.
/*
Stats��moves�̓��v���i�[����
template�̃p�����[�^�[�ɂ����History��countermoves��ۑ����邩�؂�ւ���
history�͌��݂̒T���Ŏw���肪�ǂꂾ���������������s���������i�[���A�����reducation��moveordering�ɗp������

countermoves�͈ȑO�̎w����ɑ΂��Ă��Ԃ��w������i�[����B
�����ɂ͂��̎�ɑ΂��Ă��̎�Ƃ�������i�j�n�ɂ͋�Ȃǁj������̂�countermove�͓����Ɠ���Ȃ��Ƃł͂��񂺂�Ⴄ�͂�

�G���g���[�ɂ�[���������][�ړ���̏�]������p���Ċi�[�����
���������Ĉړ������Ⴄ�������ړ���œ�������ł���Γ��ꂾ�Ƃ݂Ȃ����B

Apery�ł�drop���ǂ����ł������Ă����������ł�drop���ǂ����͋�ʂ��Ă��Ȃ�����



�_���͊i�[���ꂽ���̏󋵂Ƃ��񂺂�Ⴄ�󋵂ł������_�����p�����Ă��܂����A
�����␳���悤�Ƃ��Ă��A���̎w���肪��ԗǂ��w���肶��Ȃ��Ȃ������C�Ɉ����_���i�c��T���[���̂Q���ɔ�Ⴗ��l�j���i�[�����悤�ɂȂ�̂�
������艽���������ĕ␳���悤�Ƃ���K�v�͂Ȃ��i��s�j

�c��T���[�����[���ق����傫���l��������̂̓Q�[���؂̏�ӂ̕��ł̎}�؂肪����΂��̕��T�����ׂ��m�[�h����C�Ɍ����Ă����̂ŉ��l����������ł���Ǝv���Ă���


d 0 d^2+2d-2 -2�@�����O�͐Î~�T���̐[���Ȃ̂�0�ɂȂ邱�Ƃ͂��肦�Ȃ�

d 1 d^2+2d-2 1
d 2 d^2+2d-2 6
d 3 d^2+2d-2 13
d 4 d^2+2d-2 22
d 5 d^2+2d-2 33
d 6 d^2+2d-2 46
d 7 d^2+2d-2 61
d 8 d^2+2d-2 78
d 9 d^2+2d-2 97
d 10 d^2+2d-2 118
d 11 d^2+2d-2 141
d 12 d^2+2d-2 166
d 13 d^2+2d-2 193
d 14 d^2+2d-2 222
d 15 d^2+2d-2 253
d 16 d^2+2d-2 286
d 17 d^2+2d-2 321
d 18 d^2+2d-2 358

v��324�̒l�𒴂���̂�d=18

�Ȃ��324�Ő����������...????
https://chessprogramming.wikispaces.com/History+Heuristic

However, all of those statements were made at the time when typical search depth was much lower than today.
Nowadays some authors say that given enough search depth, history heuristic produces just a random noise

�\���[���T���[����^�����history huristic�͂����̃����_���m�C�Y�ɂȂ邩��ł���悤��...
�Ȃ�d=18�ȏ�ł����v=324��history���X�V����΂����̂ł͂Ȃ����H�i�����Ă݂�j���キ�Ȃ���

https://chessprogramming.wikispaces.com/Butterfly+Heuristic
beta�𒴂������ǂ����Ɋ֌W�Ȃ��T���ŏo�Ă����񐔂ɂ����ordering������@������炵��

*/
template<typename T, bool CM = false>
struct Stats {

	static const Value Max = Value(1 << 28);//Value��16bit�����ɂ��Ă���̂ł���Ɏ��܂�悤�ɒ���

	const T* operator[](Piece pc) const { return table[pc]; }
	T* operator[](Piece pc) { return table[pc]; }
	void clear() { 
		memset(table, 0, sizeof(table));
	}

	//move���i�[����ꍇ
	void update(Piece pc, Square to, Move m) { 
		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		table[pc][to] = m;
	}

	//�l���i�[����ꍇ
	void update(Piece pc, Square to, Value v) {

		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		//��Βl��324�ȏ�ł����return ???�@�����Ȃ��Ȃ̂��悭�킩���
		if (abs(int(v)) >= 324)
			return;

		//v = clamp(v, Value(-324), Value(324));

		table[pc][to] -= table[pc][to] * abs(int(v)) / (CM ? 936 : 324);
		table[pc][to] += (v) * 32;
	}

private:
	T table[PC_ALL][SQ_NUM];
};
typedef Stats<Move> MoveStats;
typedef Stats<Value, false> HistoryStats;
//http://yaneuraou.yaneu.com/2016/02/28/pseudo-legal%E3%81%AE%E5%88%A4%E5%AE%9A%E3%81%8C%E3%83%9E%E3%82%B8%E3%81%A7%E9%9B%A3%E3%81%97%E3%81%84%E4%BB%B6/
//��̂悤�Ȃ��Ƃɒ��ӂ���B
//countermove�͏����̂悤�Ȏ�؂�����Q�[���ł͂��Ȃ�L���炵���B
typedef Stats<Value, true> CounterMoveStats;
typedef Stats<CounterMoveStats> CounterMoveHistoryStats;

/*
���stats��to��pc�����l�����Ă��Ȃ��������������from��to���l������stats.

�悭�l������drop�̎��̂��Ƃ��l���ĂȂ������ȕ␳���Ȃ����
fromto�Ŋi�[�����̂�Batterflyboard�ƌĂ΂��
*/
struct FromToStats {

	Value get(Color c, Move m) const { return (is_drop(m)) ? table[c][81][move_to(m)] :table[c][move_from(m)][move_to(m)]; }
	void clear() { std::memset(table, 0, sizeof(table)); }

	void update(Color c, Move m, Value v)
	{
		
		if (abs(int(v)) >= 324)
			return;
		//v = clamp(v, Value(-324), Value(324));
		
		const Square t = move_to(m);
		if (is_drop(m)) {
			table[c][81][t] -= table[c][81][t] * abs(int(v)) / 324;
			table[c][81][t] += (v) * 32;
		}
		else {
			const Square f = move_from(m);

			table[c][f][t] -= table[c][f][t] * abs(int(v)) / 324;
			table[c][f][t] += (v) * 32;
		}
	}

private:
	Value table[ColorALL][82][SQ_NUM];//drop�̏ꍇ��81���g������
};

