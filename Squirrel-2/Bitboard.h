#pragma once
#include "fundation.h"
#include "occupied_m256.h"

#if defined(HAVE_SSE2) || defined(HAVE_SSE4)
#if defined(HAVE_SSE4)
// SSE4
#  include <smmintrin.h>
#else
// SSE4�Ȃ���SSE2
#  include <emmintrin.h>
#endif
#endif

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
/*
SSE��p����bit���Z�̍�����
http://d.hatena.ne.jp/LS3600/20110909
*/
struct Bitboard
{
	//�R�����{�[�h
	union {
		uint64_t b[2];//sq<=44�܂ł�b[0]sq>44��b[1]��
#if defined(HAVE_SSE2) || defined(HAVE_SSE4)
		__m128i m;
#endif
	};

	uint64_t b_(const int index)const { ASSERT(index == 0 || index == 1); return b[index]; }

	//�R���X�g���N�^
	Bitboard() {}
	Bitboard(uint64_t num1, uint64_t num2) { b[0] = num1; b[1] = num2; }
	//pop������l�̕ς��pop
	Square pop();
	Square pop_fromb0();
	Square pop_fromb1();
	Bitboard& operator=(const Bitboard& b1) { b[0] = b1.b[0];b[1]=b1.b[1]; return *this; }

	//https://msdn.microsoft.com/en-us/library/1beaceh8(v=vs.100).aspx
#if defined(HAVE_SSE2) || defined(HAVE_SSE4)
	Bitboard& operator^=(const Bitboard& b1) { m= _mm_xor_si128(m, b1.m); return *this; }
	Bitboard& operator|=(const Bitboard& b1) { m = _mm_or_si128(m, b1.m); return *this; }
	Bitboard& andnot(const Bitboard& b1) { m = _mm_andnot_si128(b1.m, m); return *this; }//���Ԓ���
	Bitboard operator << (const int i) { m = _mm_slli_epi64(m, i); return *this; }//64bit���ɑ΂��č��V�t�g�@
	Bitboard operator >> (const int i)  { m = _mm_srli_epi64(m, i); return *this; }//�E�V�t�g
#else
	Bitboard& operator^=(const Bitboard& b1) { b[0] = (b[0] ^ b1.b[0]);  b[1] = (b[1] ^ b1.b[1]); return *this; }
	Bitboard& operator|=(const Bitboard& b1) { b[0] = (b[0] | b1.b[0]);  b[1] = (b[1] | b1.b[1]); return *this; }
	Bitboard& andnot(const Bitboard& b1) { b[0] = (b[0] &~b1.b[0]); b[1] = (b[1] & ~b1.b[1]); return *this; }
#endif
	bool isNot() { return ((b[0] | b[1]) != 0); }
};
std::ostream& operator<<(std::ostream& os, const Bitboard& board);

extern Bitboard ZeroBB, ALLBB;

#if defined(HAVE_SSE2) || defined(HAVE_SSE4)
inline Bitboard operator^(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.m= _mm_xor_si128(b1.m, b2.m);  return b3; }
inline Bitboard operator|(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.m = _mm_or_si128(b1.m, b2.m); return b3; }
inline Bitboard operator&(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.m = _mm_and_si128(b1.m, b2.m); return b3; }
inline Bitboard operator~(const Bitboard& b1) { Bitboard b3; b3.m = _mm_xor_si128(b1.m, ALLBB.m); return b3; }//ALLBB��xor�}�X�N����͔̂ԊO�ɂ��P�������Ă��܂��Ă���ꏊ�o���Ă��܂�����
//b1��������鑤
inline Bitboard andnot(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.m = _mm_andnot_si128(b2.m, b1.m); return b3; }
#else
inline Bitboard operator^(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] ^ b2.b[0]); b3.b[1] = (b1.b[1] ^ b2.b[1]);  return b3; }
inline Bitboard operator|(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] | b2.b[0]); b3.b[1] = (b1.b[1] | b2.b[1]); return b3; }
inline Bitboard operator&(const Bitboard& b1, const Bitboard& b2) { Bitboard b3; b3.b[0] = (b1.b[0] & b2.b[0]); b3.b[1] = (b1.b[1] & b2.b[1]); return b3; }
inline Bitboard operator~(const Bitboard& b1) { Bitboard b3; b3.b[0] = ~b1.b[0]; b3.b[1] = ~b1.b[1]; return b3; }
#endif
//����bitboard�̓��e�͐�΂Ɍォ��ύX���Ă͂����Ȃ��I�Ipop���Ă͂����Ȃ��I
extern Bitboard SquareBB[SQ_NUM];
extern Bitboard FileBB[File_Num];
extern Bitboard RankBB[Rank_Num];

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

extern Bitboard LanceEffect[ColorALL][SQ_NUM][128];

extern Bitboard CantGo_PAWNLANCE[ColorALL];
extern Bitboard CantGo_KNIGHT[ColorALL];

extern Direction direct_table[SQ_NUM][SQ_NUM];
extern Bitboard BetweenBB[SQ_NUM][SQ_NUM];//OK

void bitboard_debug();
void bitboard_init();
Bitboard effectBB(const Position &pos, const Piece pt, const Color c, const Square sq);
Bitboard effectBB(const Occ_256& occ, const Piece pt, const Color c, const Square sq);
int change_indian(int i);

void check_directtable();


void check_between();

//==========================================
//����֘A
//==========================================
extern Bitboard PsuedoGivesCheckBB[ColorALL][PT_ALL][SQ_NUM];
//extern Bitboard GivesCheckStepBB[ColorALL][PT_ALL][SQ_NUM];
//extern Bitboard GivesCheckRookBB[ColorALL][SQ_NUM][128];
//extern Bitboard GivesCheckBishopBB[ColorALL][SQ_NUM][128];
//extern Bitboard GivesCheckLanceBB[ColorALL][SQ_NUM][128];
//===============================
//�����֘A�֐�
//================================
Bitboard step_effect(const Color c, const Piece pt, const Square sq);

//
//Bitboard long_effect(const Position &pos, const Color c, const Piece pt,  const Square sq);
//Bitboard lance_effect(const Bitboard & occ, const Color c, const Square sq);
//Bitboard rook_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
//Bitboard bishop_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);
//Bitboard dragon_effect(const Bitboard & occ_tate, const Bitboard & occ_yoko, const Square sq);
//Bitboard unicorn_effect(const Bitboard & occ_p45, const Bitboard & occ_m45, const Square sq);



Bitboard long_effect(const Occ_256& occ, const Color c, const Piece pt, const Square sq);
Bitboard lance_effect(const Occ_256 & occ, const Color c, const Square sq);
Bitboard rook_effect(const Occ_256 & occ, const Square sq);
Bitboard bishop_effect(const Occ_256 & occ, const Square sq);
Bitboard dragon_effect(const Occ_256 & occ, const Square sq);
Bitboard unicorn_effect(const Occ_256 & occ, const Square sq);

//----------------------------------------------------------------------------------
//pin�S�}�̈ʒu�e�[�u�����쐬���邽�߂Ɏg��occupied���l�����Ȃ��Ƃь����e�[�u���B
//----------------------------------------------------------------------------------
extern Bitboard RookPsuedoAttack[SQ_NUM];//OK
extern Bitboard BishopPsuedoAttack[SQ_NUM];//OK
extern Bitboard LancePsuedoAttack[ColorALL][SQ_NUM];


inline bool more_than_one(const Bitboard& b) {

	if (b.b_(0) != 0) {
		//b(0)��0�o�Ȃ����b(0)��2�ȏ�bit�����邩b(1)��1�ȏ�bit�������Ă���΂����B
		if (b.b_(1) != 0 || (b.b_(0)&(b.b_(0) - 1))) { return true; }
	}
	else if (b.b_(1) != 0) {
		//b.b_(0)���[���ł��邱�Ƃ͏�Ŋm�F����Ă���B
		if (b.b_(1)&(b.b_(1) - 1)) { return true; }
	}
	return false;
}






//foreach
//bitboard��b[0]��b[1]���ꂼ��ɑ΂��ăR�[�h�𐶐����邽�߂�define.�œK�������͂��i������Ƃ���悭�킩���j
#define foreachBB(bb,sq,state)\
	do{\
		while(bb.b_(0)){\
			sq = bb.pop_fromb0();\
			state;\
		}\
		while(bb.b_(1)){\
			sq = bb.pop_fromb1();\
			state;\
		}\
	}while(false);


//unroller�͉������������̂ق��������炵���H�iApery�������ƍœK������Ȃ����Ƃ��������炵���j�i������Ƃ���悭�킩���j
//������ł�����ȊO�̋��͍ő�6�܂łł���̂�unroller��6�ł���
#define unroller1(state){const int i=0;state;}
#define unroller2(state){unroller1(state); const int i=1; state;}
#define unroller3(state){unroller2(state); const int i=2; state;}
#define unroller4(state){unroller3(state); const int i=3; state;}
#define unroller5(state){unroller4(state); const int i=4; state;}
#define unroller6(state){unroller5(state); const int i=5; state;}