#include "movepicker.h"
#include "makemove.h"
#include "Thread.h"
#include <algorithm>
#include "evaluate.h"

/*
captureの指し手のソートに使われる。
取れる駒の価値が大きく、動く駒の価値が小さい方がいい(MVV-LVA)
*/
/*
NO_PIECE, PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD, KING,
PRO_PAWN, PRO_LANCE, PRO_NIGHT, PRO_SILVER, UNICORN, DRAGON,
*/
const int LVA_[PT_ALL] = {
	0,1,2,3,4,6,7,5,100,
	5,5,5,5,10,13
};

inline Value LVA(const Piece pt) {
	
	return Value(LVA_[pt]);
}

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
	case Start_Multicut:
		st = STOP;
		break;
	case START_Normal:
		st = STOP;
		break;
	case CAP_PRO_PAWN:case Gen_Malticut:
		end_=move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		insertion_sort(move_, end_);
		break;
	case Killers:
		killers_[0] = ss_->killers[0];
		killers_[1] = ss_->killers[1];
		current_ = killers_;
		end_ = current_ + 2;
		break;
	case QUIET:
		end_ = move_generation<Quiet>(pos_, move_);
		end_ = move_generation<Drop>(pos_, end_);
		quietscore();
		//ここSF8はもっと詳しくsortしている
		//将棋だと駒打ちなどがあるときにここの要素数が多くなってしまうのでまずはgoodquietmoveだけソートするようにしてみる（ここ調整が必要）
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
		//end_ = move_+1;
		break;
	default:
		UNREACHABLE;
		break;
	}

}



Move movepicker::return_nextmove()
{

	Move m;
	while (true) {
		while (end_ <= current_&&st != STOP) { st++; generatemove(); }


		switch (st)
		{
		case Start_Multicut:
			break;
		case Gen_Malticut:
			m = current_++->move;
			return m;
			break;
		case START_Normal:
			++current_;
			return ttMove_;
			break;
		case CAP_PRO_PAWN:
			m = current_++->move;
			if (m != ttMove_) {
				return m;
			}
			break;
		case Killers:
			m = current_++->move;
			if (m != Move(0)
				&& m != MOVE_NONE
				&&m!=ttMove_
				&& pos_.is_psuedolegal(m)
				&& !pos_.capture_or_propawn(m)
				) {
				return m;
			}
			break;
		case QUIET:
			m = current_++->move;
			if (m != killers_[0]
				&& m != killers_[1]
				&&m!=ttMove_) {
				return m;
			}
			break;
		case START_Eversion:
			++current_;
			return ttMove_;
			break;
		case EVERSION:
			m = current_++->move;
			if (m != ttMove_) {
				return m;
			}
			break;
		case START_Qsearch:
			++current_;
			return ttMove_;
			break;
		case RECAPTURE:
			m = current_++->move;
			if (m != ttMove_) {
				return m;
			}
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
}


void movepicker::quietscore()
{
	const HistoryStats& history = pos_.searcher()->history;

	ptrdiff_t num_move = end_ - move_;
//	int j = 0;
	for (int i = 0; i < num_move; i++) {

		Piece pc = moved_piece(move_[i].move);
		Square to = move_to(move_[i].move);
		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		move_[i].value = history[pc][to];
	}

}

void movepicker::capturepropawn_score()
{
	/*
	SF8では相対ランクを用いて計算しているが将棋の場合それはあまり良くないように思われる....
	普通にMVV-LVAでいく
	*/
	for (ExtMove* i = move_; i < end_; i++) {

		const Move m = i->move;
		const Piece movept = piece_type(moved_piece(m));
		const Piece capturedpt = piece_type(pos_.piece_on(move_to(m)));

		ASSERT(NO_PIECE < movept&&movept < PT_ALL);
		ASSERT(NO_PIECE <= capturedpt&&capturedpt < PT_ALL);//propawnの場合はNO_pieceを許容

		i->value = Value(Eval::piece_value[capturedpt])-LVA(movept);
		
		if (is_promote(m)) {
			ASSERT(movept <= GOLD);
			i->value += Value(Eval::diff_promote[movept]);
		}
	}

}
