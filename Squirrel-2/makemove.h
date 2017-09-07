#pragma once

#include "fundation.h"
#include "Bitboard.h"
#include "position.h"


enum Move_type {

	Cap_Propawn,//ここは飛車の成を入れるとか、歩の成を省くとかlance nightのナリを含めるとか 色々考える必要がある
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


//KPP学習用に玉をランダムに動かす用
ExtMove * test_move_king(const Position& pos, ExtMove * movelist);
ExtMove * test_move_exceptking(const Position& pos, ExtMove * movelist);
