#include "movepicker.h"
#include "makemove.h"
#include "Thread.h"
#include <algorithm>


//ソートのための関数
void insertion_sort(ExtMove* begin, ExtMove* end)
{
	ExtMove tmp, *p, *q;

	for (p = begin + 1; p < end; ++p)
	{
		tmp = *p;
		for (q = p; q != begin && *(q - 1) < tmp; --q)
			*q = *(q - 1);
		*q = tmp;
	}
}


void movepicker::generatemove()
{
	current_ = end_ = move_;
	ExtMove* goodQuiet;
	switch (st)
	{
	case START_Normal:
		break;
	case CAP_PRO_PAWN:
		end_=move_generation<Cap_Propawn>(pos_, move_);
		break;
	case QUIET:
		end_ = move_generation<Quiet>(pos_, move_);
		end_ = move_generation<Drop>(pos_, end_);
		quietscore();
		//ここSF8はもっと詳しくsortしている
		goodQuiet = std::partition(move_, end_, [](const ExtMove& m){ return m.value > Value_Zero; });
		insertion_sort(move_, goodQuiet);
		break;
	case START_Eversion:
		current_ = end_;
		st = STOP;
		break;
	case EVERSION:
		ASSERT(pos_.is_incheck());
		end_ = move_eversion(pos_, move_);
		break;
	case START_Qsearch:
		current_ = end_;
		st = STOP;
		break;
	case RECAPTURE:
		//指し手の生成関数をここに入れる
		end_ = move_recapture(pos_, move_, recapsq_);
		break;
	case STOP:
		break;
	default:
		UNREACHABLE;
		break;
	}

}



Move movepicker::return_nextmove()
{

	Move m;
	while (end_ == current_&&st != STOP) { st++; generatemove(); }


	switch (st)
	{
	case START_Normal:
		break;
	case CAP_PRO_PAWN:
		m = current_++->move;
		return m;
		break;
	case QUIET:
		return current_++->move;
		break;
	case START_Eversion:
		break;
	case EVERSION:
		m = current_++->move;
		return m;
		break;
	case START_Qsearch:
		break;
	case RECAPTURE:
		m = current_++->move;
		return m;
		break;
	case STOP:
		return MOVE_NONE;
		break;
	default:
		UNREACHABLE;
		return MOVE_NONE;
		break;
	}


}


void movepicker::quietscore()
{
	const HistoryStats& history = pos_.searcher()->history;

	ptrdiff_t num_move = end_ - move_;
	int j = 0;
	for (int i = 0; i < num_move; i++) {

		Piece pc = moved_piece(move_[i].move);
		Square to = move_to(move_[i].move);
		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		move_[i].value = history[pc][to];
	}

}
