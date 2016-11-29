#pragma once

#include "fundation.h"

/*
transposition tableを利用するためのZoblisthash
*/

typedef uint64_t Key;

//http://yaneuraou.yaneu.com/2015/12/17/%E9%80%A3%E8%BC%89%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8Bmini%E3%81%A7%E9%81%8A%E3%81%BC%E3%81%86%EF%BC%818%E6%97%A5%E7%9B%AE/
namespace Zoblist {
	/*
	上記ブログより引用

	HashEntryが先手と後手とで異なる場所であることを担保するには、
	先手と後手とで、hashのどこかのbitが異なる値になっていれば良いが、
	上のようにエントリーのアドレスを求めるとしたら、そのために使えるbitはbit0しかない。
	ここ以外のbitはアドレスの計算に必ず使うとは言いがたいからである。
	そこで先手ならbit0を0、後手ならbit0を1とするような実装にする。

	そうするとさきほどのpsq,handの乱数テーブルなどもbit0は手番情報として使うので0にしておかなければならない。
	bit0が0である数字をいくら足そうが引こうがbit0は0のままであるので、bit0に影響を及ぼさない。

	こうしておいて、手番としてhash keyのbit0を用いる。
	さきほどの乱数テーブルの初期化するソースコードで言うところのZobrist::sideがこのための定数である。
	*/

	//先手ならbit0を0、後手ならbit0を1とするような実装にする
	const Key side= uint64_t(1);

	//これらのbit0は確実に0にする。
	extern Key psq[PC_ALL][SQ_NUM];
	//なんで持ち駒の枚数を考慮しないのかは
	//http://yaneuraou.yaneu.com/2015/12/16/%E9%80%A3%E8%BC%89%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8Bmini%E3%81%A7%E9%81%8A%E3%81%BC%E3%81%86%EF%BC%817%E6%97%A5%E7%9B%AE/
	//ここを参照
	extern Key hand[ColorALL][KING];

	//psqとhandのzobristの初期化
	void init();//end of init zobrist hash

}
