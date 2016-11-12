#pragma once
#include "fundation.h"


struct Position;

//
//bitboard�ɂ͏c�^bitboard��p����
//

//PEXTbitboard���g������͂Ȃ��imerom686����̃A�h�o�C�X�j�̂�for_each_bb�̌v�Z�̕��񉻂̐��\�A�b�v�̂��߂ɂ�5�i�ڂŕ�����B
//�ǂ�������PC�ł�PEXT���ߎg���Ȃ�����...
/*
roteted���g������
�c�^�ɂ��邩�Ƃ����^�ɂ��邩�Ƃ�����������Ǝ��Ԃ������bitboard�̌`���ɂ��čl�����ق���������������Ȃ�
����������ƕ׋����Ă��炢����n�߂悤
����Ƃ����D�݂����Ȃɂ����M���Ă��̍œK�����撣�����ق���������...?


http://yaneuraou.yaneu.com/2015/10/11/%E7%B6%9A-haswell%E4%BB%A5%E9%99%8D%E5%B0%82%E7%94%A8%E3%81%A0%E3%81%A8%E4%BD%95%E3%81%8C%E5%AC%89%E3%81%97%E3%81%84%E3%81%AE%E3%81%A7%E3%81%99%E3%81%8B%EF%BC%9F/
�c�^�@���̌��������߂�̂ɃV�t�g���g����B
���̌������A������bit���W�߂Ă��邾���ł����B


�o�O���`�d���₷���悤�ɁA�ł��邾���ior��and�ł͂Ȃ��jxor���g���悤�ɂ���ׂ�
����łĂ�ꏊ��Bitboard��ێ����čX�V������̂̓��X���̂ł�߂�ׂ��B�i��낤�Ƃ��Ă����j
����������ȂɃ��X���Ƃ͎v���Ȃ��B
���ɑ΂���w���萶�������Ȃ��čςނ��A���Ƃ��������������ق�����ł��𐶐�����Ƃ��ɔ��肷������낷���Ǝv��
�����position�ŕ��̑łĂ�؂��P�ɂȂ���bitboard�����I

���̎��E��̃`�F�b�N�͌�ɉ񂵂Ă������Ǝv���i�ォ��ł��\���ɂł���Ǝv���j

*/
struct Bitboard
{
	//�R�����{�[�h
	uint64_t b[2];//sq<=44�܂ł�b[0]sq>44��b[1]��
	//�R���X�g���N�^
	Bitboard() {}
	Bitboard(uint64_t num1, uint64_t num2) { b[0] = num1; b[1] = num2; }
	//pop������l�̕ς��pop
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

//����bitboard�̓��e�͐�΂Ɍォ��ύX���Ă͂����Ȃ��I�Ipop���Ă͂����Ȃ��I
extern Bitboard SquareBB[SQ_NUM];
extern Bitboard FileBB[File_Num];
extern Bitboard RankBB[Rank_Num];
extern Bitboard ZeroBB, ALLBB;
extern Bitboard canPromoteBB[ColorALL];

//===============================
//�����֘A
//================================
extern Bitboard StepEffect[ColorALL][PT_ALL][SQ_NUM];
extern Bitboard LongRookEffect_yoko[SQ_NUM][128];//OK
extern Bitboard LongRookEffect_tate[SQ_NUM][128];//OK(���Ԃ��R�����g���R�������Ԃł��g�����߂ɂ�infrontBB��p�ӂ��Ȃ���΂Ȃ�Ȃ�)
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
//�����֘A�֐�
//================================
Bitboard step_effect(const Color c, const Piece pt, const Square sq);
Bitboard long_effect(const Position &pos, const Color c, const Piece pt,  const Square sq);


Bitboard lance_effect(const Bitboard & occ, const Color c, const Square sq);
Bitboard rook_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
Bitboard bishop_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);
Bitboard dragon_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
Bitboard unicorn_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);
