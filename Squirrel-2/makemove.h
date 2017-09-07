#pragma once

#include "fundation.h"
#include "Bitboard.h"
#include "position.h"


enum Move_type {

	Cap_Propawn,//�����͔�Ԃ̐�������Ƃ��A���̐����Ȃ��Ƃ�lance night�̃i�����܂߂�Ƃ� �F�X�l����K�v������
	Quiet,
	Eversion,
	Recapture,
	Drop,
};

template<Move_type mt>ExtMove* move_generation(const Position& pos, ExtMove* movelist);


ExtMove * move_eversion(const Position& pos, ExtMove * movelist);

ExtMove * move_recapture(const Position& pos, ExtMove * movelist,Square recapsq);


ExtMove * test_move_generation(const Position& pos, ExtMove * movelist);
ExtMove * test_quietcheck(const Position & pos, ExtMove * movelist);


//KPP�w�K�p�ɋʂ������_���ɓ������p
ExtMove * test_move_king(const Position& pos, ExtMove * movelist);
ExtMove * test_move_exceptking(const Position& pos, ExtMove * movelist);
