#pragma once

#include <iostream>

#include <string>

#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC #include <stdlib.h> #include <crtdbg.h>  
#endif
#if defined(__GNUC__) 
#include <assert.h>
#define ASSERT_(x) assert(x)
#endif

//��˂��牤�̃A�C�f�A
#ifdef _DEBUG
#define ASSERT(x) {if (!(x)){std::cout << "\nError!!\n" << "info string file:" << __FILE__ << " line:" << __LINE__ <<" "<< #x<< std::endl;_ASSERT(x);}}
#endif
#ifndef _DEBUG
#define ASSERT(X) { if (!(X)){std::cout << "\nError!!\n" << "info string file:" << __FILE__ << " line:" << __LINE__ <<" "<< #X<< std::endl; *(int*)1 =0;} }
//#define ASSERT(x) ((void)0)//���ׂĂ����Ȃ���̂Ăđ��x���o�������Ƃ��p
#endif

#define UNREACHABLE ASSERT(0)

//-------------------------------AVX�֘A

#define HAVE_SSE2
#define HAVE_SSE4 
//#define HAVE_AVX2

//#define CHECKPOS
#define SENNICHI


#define MISC//�g��Ȃ��@�\���R�����g�A�E�g���鎞�͂����undefined�ɂ���΂���

//---------------------------------�w�K�֘A

#define LEARN      //�w�K�i����P�̂��ƂڂȂ����j
//#define MAKESTARTPOS //�����ǖʃf�[�^�쐬
//#define MAKETEACHER  //���t�f�[�^�쐬
//#define Prog_LEARN  //�i�s�x�w�K
//#define REIN      //�����w�K


//�w�K����TT��ON�ɂ����leafnode�ɂ����̂ڂ�Ȃ��Ȃ�
//TT�Ɋi�[����Ă���move�ł����̂ڂ�邩������Ȃ����G���g�����j�󂳂�Ă邩������Ȃ��̂�
#ifndef LEARN
#define USETT
#endif
#ifdef USETT
#define PREFETCH
#endif

//��˂��牤�̃A�C�f�B�A
#if defined(_MSC_VER)
// C4800 : 'unsigned int': �u�[���l�� 'true' �܂��� 'false' �ɋ����I�ɐݒ肵�܂�
#pragma warning(disable : 4800)
#endif


//-----------------------------�]���֐��֘A

#define  USETMP
//#define DIFFTEST
//#define EVAL_NONDIFF


#define EVAL_KPP

#ifndef EVAL_KPP
#define EVAL_PP
#endif // !EVAL_KPP


//--------------------------------��Ս쐬���[�h���I���ɂ��邩
//#define MAKEBOOK





#if defined(_MSC_VER)

//declspec�Ƃ�.....????		http://hp.vector.co.jp/authors/VA023539/tips/dll/006.htm
#define ALIGNED(X) __declspec(align(X))
#endif
#if defined(__GNUC__) 
#include <string.h>//for memset
#include <cstring>
#define ALIGNED(X) __attribute__ ((aligned (X)))
#endif



//���t��Ƃ̍������͈͓̔��Ɏ��܂�Ȃ����������͍X�V���Ȃ��B
static const int FV_WINDOW = 256;



//==========================================================
//uint8t�Ƃ��͓���Ēu�����ق��������̂��H�H�H�i�ꉞ����Ƃ��j
//==========================================================

//���
//OK
enum Color :int8_t{
	BLACK, WHITE, ColorALL,
};
constexpr Color opposite(const Color c) { return(Color(int(c) ^ 1)); }
inline std::ostream& operator<<(std::ostream& os, const Color c) { (c == BLACK) ? (os << "BLACK") : (os << "WHITE"); return os; }



//�c�^bitboard��p����
//�擪�𕶎��ɂ��Ȃ��ƍs���Ȃ��݂����Ȃ̂ł���[�Ȃ�����SQ������
enum  Square:int8_t
{
	SQ_ZERO=0,
	SQ1A=0,SQ1B,SQ1C,SQ1D,SQ1E,SQ1F,SQ1G,SQ1H,SQ1I,
	SQ2A,SQ2B,SQ2C,SQ2D, SQ2E, SQ2F, SQ2G, SQ2H, SQ2I,
	SQ3A, SQ3B, SQ3C, SQ3D, SQ3E, SQ3F, SQ3G, SQ3H, SQ3I,
	SQ4A, SQ4B, SQ4C, SQ4D, SQ4E, SQ4F, SQ4G, SQ4H, SQ4I,
	SQ5A, SQ5B, SQ5C, SQ5D, SQ5E, SQ5F, SQ5G, SQ5H, SQ5I,
	SQ6A, SQ6B, SQ6C, SQ6D, SQ6E, SQ6F, SQ6G, SQ6H, SQ6I,
	SQ7A, SQ7B, SQ7C, SQ7D, SQ7E, SQ7F, SQ7G, SQ7H, SQ7I,
	SQ8A, SQ8B, SQ8C, SQ8D, SQ8E, SQ8F, SQ8G, SQ8H, SQ8I,
	SQ9A, SQ9B, SQ9C, SQ9D, SQ9E, SQ9F, SQ9G, SQ9H, SQ9I,
	SQ_NUM,Error_SQ=99,
};


inline constexpr Square hihumin_eye(const Square sq) { return Square(- 1 +SQ_NUM - sq); }
constexpr bool is_ok(const Square sq) { return(SQ1A <= sq&&sq <= SQ9I); }
//USI�o��
std::ostream& operator<<(std::ostream& os, const Square sq);


//�i
enum Rank:int8_t {
	RankA, RankB, RankC, RankD, RankE, RankF, RankG, RankH, RankI,Rank_Num,
};
constexpr bool is_ok(const Rank r) { return(RankA <= r&&r <= RankI); }
Rank sqtorank(const Square sq);
std::ostream & operator<<(std::ostream & os,const Rank r);

//��(uint�ɂ��Ă��܂��ƃf�N�������g��255�ɂȂ��Ă��܂��Ă��������Ȃ��Ă��܂��̂ł悭�Ȃ��I�I�I�I�I�I�I)
enum File:int8_t {
	File1,File2, File3, File4, File5, File6, File7, File8, File9,File_Num,
};
constexpr bool is_ok(const File f) { return(File1 <= f&&f <= File9); }
File sqtofile(const Square sq);
std::ostream& operator<<(std::ostream& os, const File f);


//����ŏ�肭�s���Ă���͂��B
inline Square make_square(const Rank r, const  File f) {
	return Square(r + (f * 9));
}
 
//square�̍��E�𔽓]�����邽�߂̊֐�
inline Square sym_rl_sq(const Square sq) {
	File f = sqtofile(sq);
	Rank r = sqtorank(sq);
	File sym_file = File(9 - 1 - f);

	return make_square(r, sym_file);
}



enum Piece:uint8_t {
	//pt(Piece type) �������Ɏ����Ă��邱�Ƃŋ��͘A������B
	NO_PIECE, PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD, KING,
			  PRO_PAWN, PRO_LANCE, PRO_NIGHT, PRO_SILVER, UNICORN, DRAGON,PT_ALL,

	//pc (colored piece)
	B_PAWN=1, B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING,
	B_PRO_PAWN, B_PRO_LANCE, B_PRO_NIGHT, B_PRO_SILVER, B_UNICORN, B_DRAGON,
	W_PAWN=17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING,
	W_PRO_PAWN, W_PRO_LANCE, W_PRO_NIGHT, W_PRO_SILVER, W_UNICORN, W_DRAGON,PC_ALL,


	PROMOTE = 0b1000,
	WHITE_piece=0b10000,

};

inline Piece inverse(const Piece pc) { return Piece(pc^WHITE_piece); }//xor��whitepiece�t���O�𔽓]������B
inline bool is_ok(const Piece pc) { return ((B_PAWN <= pc&&pc <= B_DRAGON) || (W_PAWN <= pc&&pc <= W_DRAGON)); }
inline Color piece_color(const Piece pc) { return (pc& WHITE_piece) ? WHITE : BLACK; }
inline Piece piece_type(const Piece pc) { return Piece(pc & 0b1111); }//��Ԃ̏�������
inline bool can_promote(const Piece pt){ return ((B_PAWN <= pt&&pt <= B_ROOK) /*|| (W_PAWN <= pt&&pt <= W_ROOK)*/); }
inline Piece promotepiece(const Piece pc) { ASSERT(can_promote(piece_type(pc)));  return Piece(pc | PROMOTE); }
inline Piece rowpiece(const Piece pc) { /*ASSERT(!can_promote(pc));*/ return Piece(pc&~PROMOTE); }//do_move�łȂ��Ă��Ȃ�����ꗥ�ł��̊֐��ɓ˂����݂����̂�ASSERT�͊O��
inline Piece add_color(const Piece pt, const Color c) { return (c == BLACK) ? pt : Piece(pt | WHITE_piece); }
inline bool is_promote_piece(const Piece pc) { return bool(pc&PROMOTE); }
//�\���p�i�R���ł��܂�������....????�jOK��肭�s���Ă��B
extern std::string outputPiece[PC_ALL] ;
extern std::string USIPiece[PC_ALL];
std::ostream& operator<<(std::ostream& os, const Piece pc);//usi�`���\��
std::ostream& outputpiece(std::ostream& os, const Piece pc);


//�T���ő�[��
const int MAX_PLY = 256;


//�]���l��TT��16bit�Ŏ��߂Ȃ���ΐ���Ȃ�����32768����32767�܂�
/*
�\���̕���傫�����邽�߂ɍő�l��傫������ׂ���,
PC�ɂƂ��Ď�舵���₷���l�ɂ���ׂ���
*/
enum Value :int{

	Value_Zero = 0,
	Value_Mate = 32760,
	Value_Mated = -32760,
	Value_mate_in_maxply = int(Value_Mate) - MAX_PLY,
	Value_mated_in_maxply = int(Value_Mated) + MAX_PLY,

	Value_known_win = 10000,//8000���x�ł������͂�������
	Value_Infinite = 32761,
	Value_error = INT_MIN,
	
	//��̉��l
	/*Hiyoko_value = 100,
	Zou_value = 150,
	Kirin_value = 150,
	Niwatori_value = 200,
	Lion_value = 10000,*/
	
};

/*
to 0~6
from 7~13
drop=14
capture=15
propawn=16
piece 17~24

(�]���Ă���Ƃ���ɉ���������Ȃ����H�H�j
rootmove��find����K�v���T�����ł��邽�߁A�T���̓r����move�ɂȂɂ��ǉ��͌���
)
*/
enum Move :uint32_t {

	MOVE_TO = (0b1111111),
	MOVE_FROM = (MOVE_TO << 7),
	FLAG_DROP = 1 << 14,
	FLAG_CAPTURE = 1 << 15,
	FLAG_PROMOTE = 1 << 16,
	FLAG_CAPPROPAWN = (0b11) << 15,
	PIECE_MASK = (0b11111111) << 17,

	MOVE_NONE =0 /*1 + (1 << 7)*/,//from��to����v���Ă���
	MOVE_NULL=2+(2<<7),
};
inline Square move_from(const Move m) { return Square((m >> 7)&MOVE_TO); }
inline Square move_to(const Move m) { return Square(m&MOVE_TO); }
inline bool is_drop(const Move m) { return (m&FLAG_DROP); }
inline bool is_capture(const Move m) { return (m&FLAG_CAPTURE); }
inline bool is_promote(const Move m) { return (m&FLAG_PROMOTE); }
inline bool is_capproppown(const Move m) { return (m&FLAG_CAPPROPAWN); }
inline Piece moved_piece(const Move m) { return Piece((m >> 17) & 0xFF); }
inline Move make_move(const Square from, const Square to, const Piece pc) { return Move((from << 7) + to + ((pc & 0xFF) << 17)); }
inline Move make_movepromote(const Square  from, const Square to, Piece pc) { return Move((from << 7) + to + ((pc & 0xFF) << 17) + FLAG_PROMOTE); }
inline Move make_drop(const Square to, const Piece pc) { return Move(to + ((pc & 0xFF) << 17) + int(FLAG_DROP)); }

//���̕ӂ̊֐��g���̒���
inline Move add_capture(const Move m) { return Move(m | FLAG_CAPTURE); }
inline Move add_promote(const Move m) { return Move(m | FLAG_PROMOTE); }

//from,pc�����O��PC<<17,from<<7�����Ă������Ƃō�������}��Ƃ��ׂ̈�make_move();
inline Move make_move2(const int from, const Square to, const int pc) { return Move(from + to + pc); }
inline Move make_movepromote2(const int from, const Square to, const int pc) { return Move(from + to + pc + FLAG_PROMOTE); }
inline Move make_drop2(const Square to, const int pc) { return Move(to + pc + int(FLAG_DROP)); }

std::ostream& operator<<(std::ostream& os, const Move m);
void check_move(const Move m);
inline bool is_ok(Move m) {

	Square from = move_from(m);
	Square to = move_to(m);
	Piece movedpiece = moved_piece(m);

	if (is_ok(from) && is_ok(to) && is_ok(movedpiece)) {
		return true;
	}
	else {
		return false;
	}

}

//ordering�p�_���Ǝw����̂͂������N���X�B
struct ExtMove {

	Move move;
	Value value=Value_Mated;

	operator Move() { return move; }
	void operator=(Move move_) { move = move_; }
};
//�ƌ������\�[�g�̂��߂̑召��roperator���`���ĂȂ����������....
inline bool operator < (const ExtMove& em1, const ExtMove& em2) { return em1.value < em2.value; }
inline bool operator > (const ExtMove& em1, const ExtMove& em2) { return em1.value > em2.value; }


//����܂�Ԃ��l�߂�����̂������C�����͂��Ȃ���...�s����
//���ѕ���Pt�̏�
//��0~5
//��6~8
//�j9~11
//��12~14
//�p15~16
//��17~18
//��19~21
enum Hand :uint32_t {
	NO_Hands = 0,

	PAWN_MASK = 0b11111,
	LANCE_MASK=(0b111)<<6,
	KNIGHT_MASK=(0b111)<<9,
	SILVER_MASK=(0b111)<<12,
	BISHOP_MASK=(0b11)<<15,
	ROOK_MASK=(0b11)<<17,
	GOLD_MASK=(0b111)<<19

};

constexpr int shift[8] = { 0,0,6,9,12,15,17,19 };
constexpr int mask[8] = { 0,0b11111,0b111,0b111,0b111,0b11,0b11,0b111 };


constexpr bool have_hand(const Hand& h) { return(h != 0); }
constexpr bool have_pawn(const Hand& h) { return(h&PAWN_MASK); }
constexpr bool have_pt(const Hand& h, const Piece pt) { return((h >> shift[pt])&mask[pt]); }//�R���͏�肭�s���Ă��邩��Ŋm�F���Ȃ���΂Ȃ�Ȃ��B�iOK�j
constexpr int  num_pt(const Hand& h, const Piece pt) { return((h >> shift[pt])&mask[pt]); }//�R���͏�Ɠ����Ȃ̂�have_pt�͗v��Ȃ���������Ȃ��B�E�EOK
inline bool have_except_pawn(const Hand& h) { return bool(h&~PAWN_MASK); }//pawn�ȊO�̋�������Ă��邩�H
//��肭�s���Ă��邩�m�F���K�v�iOK�j
inline void makehand(Hand &hand,const Piece pt,const int num){
	hand = Hand(hand&~(mask[pt] << shift[pt]));
	hand = Hand(hand + (num << shift[pt]));
}
std::ostream& operator << (std::ostream& os, Hand h);


//�����x�N�g��
//��ї����Ƃ����������邽�߂Ɏg�������̂Ōj�n�̗������܂߂�12�����ł͂Ȃ��A8����������p�ӂ���

/*
����board���c�^�Ȃ̂ł���ɍ��킹�Ēl��-1�Ƃ�-9�Ƃ��ɂ���ׂ��H
�������������₷����....
*/
enum Direction :int8_t{
	UP=-1,
	RightUP=-10,
	Right=-9,
	RightDOWN=-8,
	DOWN=1,
	LeftDOWN=10,
	Left=9,
	LeftUP=8,

	Direct_NUM=8,
};

constexpr Direction direct[Direct_NUM] = {
	UP,
	RightUP,
	Right,
	RightDOWN,
	DOWN,
	LeftDOWN ,
	Left,
	LeftUP,
};

std::ostream& operator << (std::ostream& os, Direction d);



enum Depth {

	DEPTH_ZERO=0,
	HALF_PLY = 1,
	ONE_PLY = 2,
	MAX_DEPTH = 64,
	DEPTH_QS_CHECKS =0,// 0 * ONE_PLY,
	DEPTH_QS_NO_CHECKS = -2,//-1 * ONE_PLY,
	DEPTH_QS_RECAPTURES =-10, //-5 * ONE_PLY,
	DEPTH_NONE = -12,//6*ONEPLY
};

enum Bound :int8_t {
	BOUND_NONE,
	BOUND_UPPER,
	BOUND_LOWER,
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER,
};

#define ENABLE_OPERATORS_ON(T)                                                  \
  inline T operator+(const T d1, const T d2) { return T(int(d1) + int(d2)); }   \
  inline T operator-(const T d1, const T d2) { return T(int(d1) - int(d2)); }   \
  inline T operator*(const int i, const T d) { return T(i * int(d)); }          \
  inline T operator*(const T d, const int i) { return T(int(d) * i); }          \
  inline T operator-(const T d) { return T(-int(d)); }                          \
  inline T& operator+=(T& d1, const T d2) { return d1 = d1 + d2; }              \
  inline T& operator-=(T& d1, const T d2) { return d1 = d1 - d2; }              \
  inline T& operator*=(T& d, const int i) { return d = T(int(d) * i); }         \
  inline T& operator++(T& d) { return d = T(int(d) + 1); }                      \
  inline T& operator--(T& d) { return d = T(int(d) - 1); }                      \
  inline T operator++(T& d,int) { T prev = d; d = T(int(d) + 1); return prev; } \
  inline T operator--(T& d,int) { T prev = d; d = T(int(d) - 1); return prev; } \
  inline T operator/(const T d, const int i) { return T(int(d) / i); }          \
  inline T& operator/=(T& d, const int i) { return d = T(int(d) / i); }			\
  inline T operator+(const T d1,const int d2){return T(int(d1)+d2);}            \
  inline T operator-(const T d1, const int d2) { return T(int(d1) - d2); }

ENABLE_OPERATORS_ON(Square)
ENABLE_OPERATORS_ON(Rank)
ENABLE_OPERATORS_ON(File)
ENABLE_OPERATORS_ON(Piece)
ENABLE_OPERATORS_ON(Color)
ENABLE_OPERATORS_ON(Hand)
ENABLE_OPERATORS_ON(Depth)
ENABLE_OPERATORS_ON(Value)
