#pragma once
#include "Bitboard.h"

/*

bitboardは縦型bitboardで[0]が44bit迄使用

bitboard 90のレイアウト

 0 1 2 3 4 5 6 7 8
 91011121314151617
181920212223242526
272829303132333435
363738394041424344
454647484950515253
545556575859606162
636465666768697071
727374757677787980

 */

//sqのindexをsq90のインデックスに変換するために使うテーブル
constexpr int sq2sq90[SQ_NUM] = {

	72,63,54,45,36,27,18,9,0,
	73,64,55,46,37,28,19,10,1,
	74,65,56,47,38,29,20,11,2,
	75,66,57,48,39,30,21,12,3,
	76,67,58,49,40,31,22,13,4,
	77,68,59,50,41,32,23,14,5,
	78,69,60,51,42,33,24,15,6,
	79,70,61,52,43,34,25,16,7,
	80,71,62,53,44,35,26,17,8,

};


inline int sq_to_sq90(Square sq) {

	ASSERT(is_ok(sq));
	return sq2sq90[sq];

}


/*

bitboard_plus45

 1 2 3 4 5 6 7 8 0
11121314151617 910
212223242526181920
313233343527282930
414243444737383940
515253454647484950
616254555657585960
716364656667686970
727374757677787980

おかしい可能性もあるので十分気をつける
*/
constexpr int sq2sqplus45[SQ_NUM] = {

	  0,72,63,54,45,36,27,18, 9,10,//9
	  1,73,64,55,46,37,28,19,20,11,//19
	  /*20*/2,/*21*/74,/*22*/65,/*23*/56,/*24*/47,/*25*/38,/*26*/29,/*27*/30,/*28*/21,/*29*/12,
	  /*30*/3,/*31*/75,/*32*/66,/*33*/57,/*34*/48,/*35*/39,/*36*/40,/*37*/31,/*38*/22,/*39*/13,
	  /*40*/4,/*41*/76,/*42*/67,/*43*/58,/*44*/49,/*45*/50,/*46*/41,/*47*/32,/*48*/23,/*49*/14,
	  /*50*/5,/*51*/77,/*52*/68,/*53*/59,/*54*/60,/*55*/51,/*56*/42,/*57*/33,/*58*/24,/*59*/15,
	  /*60*/6,/*61*/78,/*62*/69,/*63*/70,/*64*/61,/*65*/52,/*66*/43,/*67*/34,/*68*/25,/*69*/16,
	  /*70*/7,/*71*/79,/*72*/80,/*73*/71,/*74*/62,/*75*/53,/*76*/44,/*77*/35,/*78*/26,/*79*/17,
	  /*80*/8,

};

inline int sq_to_sqplus45(Square sq) {

	ASSERT(is_ok(sq));
	return sq2sqplus45[sq];

}

/*

bitboard minus 45

635445362718 9 072
554637281910 17364
4738292011 2746556
39302112 375665748
312213 47667584940
2314 5776859504132
15 678696051423324
 77979615243342516
8071625344352617 8

おかしい可能性もあるので十分気をつける
*/
constexpr int sq2sqminus45[SQ_NUM] = {

	/*0*/9,19,29,39,49,59,69,79,/*8*/8,
	/*9*/18,28,38,48,58,68,/*15*/78,/*16*/7,/*17*/17,
	/*18*/27,37,47,57,67,77,/*24*/6,/*25*/16,/*26*/26,
	/*27*/36,46,56,66,/*31*/76,/*32*/5,15,25,/*35*/35,
	/*36*/45,55,65,/*39*/75,/*40*/4,14,24,34,/*44*/44,
	/*45*/54,64,74,/*48*/3,13,23,33,43,/*53*/53,
	/*54*/63,73,/*56*/2,12,22,32,42,52,/*62*/62,
	/*63*/72,1,11,21,31,41,51,61,/*71*/71,
	/*72*/0,10,20,30,40,50,60,70,80,

};


inline int sq_to_sqminus45(Square sq) {

	ASSERT(is_ok(sq));
	return sq2sqminus45[sq];

}


constexpr int effectmask = (0b1111111);//7bitが立っている。

/*
利きを求めるために何ビットシフトさせないといけないか

*/
//int shift_table_tate[SQ_NUM];

/*
縦の利き　OK
あとでconstexprにするか値をテーブルに格納するかして高速化する
*/
inline int shift_tate(Square sq) {

	ASSERT(is_ok(sq));
	//return shift_table_tate[sq];
	//6段目からはb[1]になるので
	return int(1 + 9 * (sqtofile(sq)%5));//0からはじまる！

}

inline int index_tate(Square sq) {

	ASSERT(is_ok(sq));
	return sq > 44 ? 1 : 0;

}

/*
横の効き
OK
*/

inline int shift_yoko(Square sq) {

	ASSERT(is_ok(sq));
	Rank r = sqtorank(sq);
	//File f = sqtofile(sq);
	if (RankA <= r&&r <= RankD) {
		return 1 + (9 * (3 - r));
	}
	else {
		return 1 + (9 * (8 - r));
	}

	//ここには来ないはずである
	ASSERT(0);
}

inline int index_yoko(Square sq) {

	ASSERT(is_ok(sq));
	File f = sqtofile(sq);

	return (9 * f <= sq&&sq <= 9 * f + 3) ? 1 : 0;

}
/*
斜め＋４５度
*/
extern int indexPlus45[SQ_NUM];
extern int shiftPlus45[SQ_NUM];

void make_indexplus45();
void make_shiftplus45();


inline int index_plus45(Square sq) {

	ASSERT(is_ok(sq));
	//indexは間違ってなかった。
	return indexPlus45[sq];
}

inline int shift_plus45(Square sq) {

	ASSERT(is_ok(sq));

	return shiftPlus45[sq];
}

/*
斜めマイナス４５度
*/
extern int indexMinus45[SQ_NUM];
extern int shiftMinus45[SQ_NUM];

void make_indexMinus45();
void make_shiftMinus45();


inline int index_Minus45(Square sq) {

	ASSERT(is_ok(sq));
	//indexは間違ってなかった。
	return indexMinus45[sq];
}

inline int shift_Minus45(Square sq) {

	ASSERT(is_ok(sq));

	return shiftMinus45[sq];
}