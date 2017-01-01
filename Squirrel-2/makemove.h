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
ExtMove * test_drop_fast(const Position& pos, ExtMove * movelist);
