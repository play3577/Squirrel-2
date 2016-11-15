#include "movepicker.h"
#include "makemove.h"

void movepicker::generatemove()
{
	current_ = end_ = move_;

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
		//Žw‚µŽè‚Ì¶¬ŠÖ”‚ð‚±‚±‚É“ü‚ê‚é
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
		break;
	}


}