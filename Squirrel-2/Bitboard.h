#pragma once
#include "fundation.h"


struct Position;

//
//bitboardには縦型bitboardを用いる
//

//PEXTbitboardを使うつもりはない（merom686さんのアドバイス）のでfor_each_bbの計算の並列化の性能アップのためにも5段目で分ける。
//どうせこのPCではPEXT命令使えないしね...
/*
rotetedを使うつもり
縦型にするかとか横型にするかとかもうちょっと時間を取ってbitboardの形式について考えたほうがいいかもしれない
もうちょっと勉強してからいじり始めよう
それともより好みせずなにか一つを信じてその最適化を頑張ったほうがいいか...?


http://yaneuraou.yaneu.com/2015/10/11/%E7%B6%9A-haswell%E4%BB%A5%E9%99%8D%E5%B0%82%E7%94%A8%E3%81%A0%E3%81%A8%E4%BD%95%E3%81%8C%E5%AC%89%E3%81%97%E3%81%84%E3%81%AE%E3%81%A7%E3%81%99%E3%81%8B%EF%BC%9F/
縦型　歩の効きを求めるのにシフトが使える。
香の効きも連続するbitを集めてくるだけでいい。


バグが伝播しやすいように、できるだけ（orやandではなく）xorを使うようにするべき
歩を打てる場所のBitboardを保持して更新させるのはロスいのでやめるべき。（やろうとしていた）
↑正直そんなにロスいとは思えない。
歩に対する指し手生成が少なくて済むし、あとから二歩判定をするほうが駒打ちを生成するときに判定するよりもろすだと思う
よってpositionで歩の打てる筋が１になったbitboardを作る！

王の自殺手のチェックは後に回してもいいと思う（後からでも十分にできると思う）

*/
struct Bitboard
{
	//コレがボード
	uint64_t b[2];//sq<=44まではb[0]sq>44はb[1]に
	//コンストラクタ
	Bitboard() {}
	Bitboard(uint64_t num1, uint64_t num2) { b[0] = num1; b[1] = num2; }
	//popした後値の変わるpop
	Square pop();

	Bitboard& operator=(const Bitboard& b1) { b[0] = b1.b[0];b[1]=b1.b[1]; return *this; }
	Bitboard& operator^=(const Bitboard& b1) { b[0] = (b[0] ^ b1.b[0]);  b[1] = (b[1] ^ b1.b[1]); return *this; }
	Bitboard& operator|=(const Bitboard& b1) { b[0] = (b[0] | b1.b[0]);  b[1] = (b[1] | b1.b[1]); return *this; }
	Bitboard& andnot(const Bitboard& b1) { b[0] = (b[0] &~b1.b[0]); b[1] = (b[1] & ~b1.b[1]); return *this; }
	bool isNot() { return ((b[0] | b[1]) != 0); }
};
std::ostream& operator<<(std::ostream& os, const Bitboard& board);

inline Bitboard operator^(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] ^ b2.b[0]); b3.b[1] = (b1.b[1] ^ b2.b[1]);  return b3; }
inline Bitboard operator|(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] | b2.b[0]); b3.b[1] = (b1.b[1] | b2.b[1]); return b3; }
inline Bitboard operator&(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] & b2.b[0]); b3.b[1] = (b1.b[1] & b2.b[1]); return b3; }
inline Bitboard operator~(const Bitboard& b1) { Bitboard b3; b3.b[0] = ~b1.b[0]; b3.b[1] = ~b1.b[1]; return b3; }

//このbitboardの内容は絶対に後から変更してはいけない！！popしてはいけない！
extern Bitboard SquareBB[SQ_NUM];
extern Bitboard FileBB[File_Num];
extern Bitboard RankBB[Rank_Num];
extern Bitboard ZeroBB, ALLBB;
extern Bitboard canPromoteBB[ColorALL];

//===============================
//効き関連
//================================
extern Bitboard StepEffect[ColorALL][PT_ALL][SQ_NUM];
extern Bitboard LongRookEffect_yoko[SQ_NUM][128];//OK
extern Bitboard LongRookEffect_tate[SQ_NUM][128];//OK(香車もコレを使うコレを香車でも使うためにはinfrontBBを用意しなければならない)
extern Bitboard LongBishopEffect_plus45[SQ_NUM][128];//OK
extern Bitboard LongBishopEffect_minus45[SQ_NUM][128];//OK

extern Bitboard InFront_BB[ColorALL][Rank_Num];

extern Bitboard CantGo_PAWNLANCE[ColorALL];
extern Bitboard CantGo_KNIGHT[ColorALL];

extern Direction direct_table[SQ_NUM][SQ_NUM];
extern Bitboard BetweenBB[SQ_NUM][SQ_NUM];//OK

void bitboard_debug();
void bitboard_init();
Bitboard effectBB(const Position &pos, const Piece pt, const Color c, const Square sq);

int change_indian(int i);

void check_directtable();


void check_between();

//===============================
//効き関連関数
//================================
Bitboard step_effect(const Color c, const Piece pt, const Square sq);
Bitboard long_effect(const Position &pos, const Color c, const Piece pt,  const Square sq);


Bitboard lance_effect(const Bitboard & occ, const Color c, const Square sq);
Bitboard rook_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
Bitboard bishop_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);
Bitboard dragon_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
Bitboard unicorn_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);
