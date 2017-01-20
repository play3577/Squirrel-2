#pragma once
#include "fundation.h"


// The Stats struct stores moves statistics. According to the template parameter
// the class can store History and Countermoves. History records how often
// different moves have been successful or unsuccessful during the current search
// and is used for reduction and move ordering decisions.
// Countermoves store the move that refute a previous one. Entries are stored
// using only the moving piece and destination square, hence two moves with
// different origin but same destination and piece will be considered identical.
/*
Statsはmovesの統計を格納する
templateのパラメーターによってHistoryかcountermovesを保存するか切り替える
historyは現在の探索で指し手がどれだけ成功したか失敗したかを格納し、これはreducationやmoveorderingに用いられる

countermovesは以前の指し手に対してやり返す指し手を格納する。
将棋にはこの手に対してこの手という流れ（桂馬には銀など）があるのでcountermoveは入れると入れないとではぜんぜん違うはず

エントリーには[動いた駒種][移動先の升]だけを用いて格納される
したがって移動元が違うが同じ移動先で同じ駒酒であれば同一だとみなされる。

Aperyではdropかどうかでも分けていたが魔女ではdropかどうかは区別していなかった



点数は格納された時の状況とぜんぜん違う状況でも同じ点数が用いられてしまうが、
それを補正しようとしても、その指し手が一番良い指し手じゃなくなったら一気に悪い点数（残り探索深さの２条に比例する値）が格納されるようになるので
無理やり何かを加えて補正しようとする必要はない（一敗）

残り探索深さが深いほうが大きい値がつけられるのはゲーム木の上辺の方での枝切りがあればその分探索すべきノードが一気に減ってくれるので価値が高いからであると思っている


d 0 d^2+2d-2 -2　ｄ＝０は静止探索の深さなので0になることはありえない

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

vが324の値を超えるのはd=18

なんで324で制限を入れるんだ...????
全くわからん

*/
template<typename T, bool CM = false>
struct Stats {

	static const Value Max = Value(1 << 14);

	const T* operator[](Piece pc) const { return table[pc]; }
	T* operator[](Piece pc) { return table[pc]; }
	void clear() { 
		memset(table, 0, sizeof(table));
	}

	//moveを格納する場合
	void update(Piece pc, Square to, Move m) { 
		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		table[pc][to] = m;
	}

	//値を格納する場合
	void update(Piece pc, Square to, Value v) {

		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		//絶対値が324以上であればreturn ???　ここなぜなのかよくわからん
		if (abs(int(v)) >= 324)
			return;

		table[pc][to] -= table[pc][to] * abs(int(v)) / (CM ? 936 : 324);
		table[pc][to] += (v) * 32;
	}

private:
	T table[PC_ALL][SQ_NUM];
};

typedef Stats<Value, false> HistoryStats;