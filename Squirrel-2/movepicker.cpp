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
	/*0,1,2,3,4,6,7,5,100,
	5,5,5,5,10,13*/
	0,1,2,3,4,8,9,7,100,
	4,5,5,6,10,11
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

void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {

	for (ExtMove *sortedEnd = begin + 1, *p = begin + 1; p < end; ++p)
		if (p->value >= limit)
		{
			ExtMove tmp = *p, *q;
			*p = *sortedEnd;
			for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
				*q = *(q - 1);
			*q = tmp;
			++sortedEnd;
		}
}
#if 0
void movepicker::generatemove()
{
	current_ = end_ = move_;
	ExtMove* goodQuiet;
	switch (st)
	{
	//case Start_Multicut:
	//	st = STOP;
	//	break;
	//case Gen_Malticut:
	//	end_ = move_generation<Cap_Propawn>(pos_, move_);
	//	capturepropawn_score();
	//	//insertion_sort(move_, end_);
	//	break;
	case START_Normal:
		st = STOP;
		break;
	case CAP_PRO_PAWN:case Gen_Probcut:
		end_=move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		//insertion_sort(move_, end_);
		break;
	case Start_Probcut:
		st = STOP;
		break;
	case Killers:
		killers[0].move = ss->killers[0];
		killers[1].move = ss->killers[1];
		killers[2] = countermove;
		current_ = killers;
		end_ = current_ + 2 + (countermove != killers[0] && countermove != killers[1]);
		break;
	case QUIET:
		end_ = move_generation<Quiet>(pos_, move_);
		end_ = move_generation<Drop>(pos_, end_);
		quietscore();
		//ここSF8はもっと詳しくsortしている
		//将棋だと駒打ちなどがあるときにここの要素数が多くなってしまうのでまずはgoodquietmoveだけソートするようにしてみる（ここ調整が必要）
		partial_insertion_sort(move_, end_, -4000 * depth_ / ONE_PLY);
		/*if (depth_ < 3 * ONE_PLY) {
			goodQuiet = std::partition(move_, end_, [](const ExtMove& m) { return m.value > Value_Zero; });
			insertion_sort(move_, goodQuiet);
		}
		else {
			insertion_sort(move_, end_);
			
		}*/
		break;
	case BAD_CAPTURES:
		current_ = move_ + 600 - 1;
		end_ = end_badcaptures;
		break;
	case START_Eversion:
		current_ = end_;
		st = STOP;
		break;
	case EVERSION:
		ASSERT(pos_.is_incheck());
		end_ = move_eversion(pos_, move_);
		if (end_ - move_ > 1) {
			eversion_score();
		}
		break;
	case START_Q_RECAPTURE:
		current_ = end_;
		st = STOP;
		break;
	case RECAPTURE:
		//指し手の生成関数をここに入れる
		end_ = move_recapture(pos_, move_, recapsq_);
		/*if (end_ - move_ > 1) {
			capturepropawn_score();
		}*/
		break;
	case START_Q_CAP_PROPAWN: case START_Q_WITH_CHECKS:
		current_ = end_;
		st = STOP;
		break;
	case Q_CAP_PROPAWN: case Q_CAP_PROPAWN_2:
		end_ = move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		break;
	case Q_CHECKS:
		end_ = test_quietcheck(pos_, move_);
		if (end_ - move_>1) {
			quietscore();//ssを渡していなかった
			insertion_sort(move_, end_);
		}
	case STOP:
		break;
	default:
		UNREACHABLE;
		break;
	}

}
#endif


//かなり大胆なことするなぁ
Move movepicker::return_nextmove(bool skipQuiets)
{

	Move m;

	switch (st)
	{
		//------------------------------------------------
	case Start_Probcut:
		st++;
		return ttMove;
	case Probcur_INIT:
		current_ = move_;
		end_ = move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		st++;

	case Gen_Probcut:
		while (current_<end_)
		{
			m = pick_best(current_++, end_);
			if (m != ttMove&&pos_.see_ge(m, Threshold)) { return m; }
		}
		break;
		//------------------------------------------------
	case START_Normal:
		st++;
		return ttMove;
		
	case Capture_INIT:
		end_badcaptures = current_ = move_;
		end_ = move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		st++;
	case CAP_PRO_PAWN:
		while (current_<end_)
		{
			m = pick_best(current_++, end_);
			if (m != ttMove) {
				if (pos_.see_ge(m, Value_Zero)) {
					return m;
				}
				else {//怖いので一応
					(end_badcaptures++)->move = m;
				}
			}
		}
		st++;
		m = killers[0];
		if (m != MOVE_NONE
			&&  m != ttMove
			&&  pos_.pseudo_legal(m)
			&& !pos_.capture_or_propawn(m)) {
			return m;
		}
	case Killers:
		st++;
		m = killers[1];
		//killers[0]と違う差し手であることは格納時に保証済み
		if (m != MOVE_NONE
			&&  m != ttMove
			&&  pos_.pseudo_legal(m)
			&& !pos_.capture_or_propawn(m)) {
			return m;
		}
	case COUNTERMOVE:
		st++;
		m = countermove;
		if (m!=MOVE_NONE
			&&m != ttMove
			&&m != killers[0]
			&& m != killers[1]
			&& pos_.pseudo_legal(m)
			&& !pos_.capture_or_propawn(m)
			) {
			return m;
		}
		
	case QUIET_INIT:
		current_ = end_badcaptures;
		end_ = move_generation<Quiet>(pos_, end_badcaptures);
		end_ = move_generation<Drop>(pos_, end_);
		quietscore();
		partial_insertion_sort(current_, end_, -4000 * depth_ / ONE_PLY);
		st++;
	case QUIET:
		while (current_ < end_ && (!skipQuiets || current_->value >= Value_Zero)) {
			m = current_++->move;
			if (m != ttMove
				&&m != killers[0]
				&& m != killers[1]
				&&m!=countermove
				) {
				return m;
			}
		}
		st++;
		current_ = move_;
	case BAD_CAPTURES:

		if (current_ < end_badcaptures) {
			return current_++->move;
		}

		break;
		//------------------------------------------------
	case START_Eversion:
		st++;
		return ttMove;
		
	case EVERSION_INIT:
		current_ = move_;
		ASSERT(pos_.is_incheck());
		end_ = move_eversion(pos_, move_);
		if (end_ - move_ > 1) {
			eversion_score();
		}
		st++;
	case EVERSION:
		while (current_<end_)
		{
			m = pick_best(current_++, end_);
			if (m != ttMove) {
				return m;
			}
		}
		break;
		//------------------------------------------------
	case START_Q_RECAPTURE:
		current_ = move_;
		end_ = move_recapture(pos_, move_, recapsq_);
		if (end_ - move_ > 1) {
			capturepropawn_score();
		}
		st++;
	case RECAPTURE:
		while (current_ < end_) {
			m = pick_best(current_++, end_);
			return m;
		}

		break;
	case START_Q_CAP_PROPAWN:
		st++;
		return ttMove;
	case Q_CAP_PROPAWNINIT:
		current_ = move_;
		end_ = move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		st++;
		
	case Q_CAP_PROPAWN:
		while (current_ < end_) {
			m = pick_best(current_++, end_);
			if (m != ttMove) {
				return m;
			}
		}
		break;
		//------------------------------------------------
	case START_Q_WITH_CHECKS:
		st++;
		return ttMove;

	case Q_CAP_PROPAWN_2_INIT:
		current_ = move_;
		end_ = move_generation<Cap_Propawn>(pos_, move_);
		capturepropawn_score();
		st++;
	
	case Q_CAP_PROPAWN_2:
		while (current_ < end_) {
			m = pick_best(current_++, end_);
			if (m != ttMove) {
				return m;
			}
		}
		current_ = move_;
		end_ = test_quietcheck(pos_, move_);
		if (end_ - move_>1) {
			quietscore();//ssを渡していなかった
			insertion_sort(move_, end_);
		}
		st++;
	case Q_CHECKS:
		while (current_<end_)
		{
			m = current_++->move;
			if (m != ttMove) {
				return m;
			}
		}
		break;
	case STOP:
		break;
	default:
		UNREACHABLE;
		break;
	}
	return MOVE_NONE;
}


void movepicker::quietscore()
{
	const HistoryStats& history = pos_.searcher()->history;
	const FromToStats& fromTo= pos_.searcher()->fromTo;
	const CounterMoveStats* cm = (ss - 1)->counterMoves;
	const CounterMoveStats* fm = (ss - 2)->counterMoves;
	const CounterMoveStats* f2 = (ss - 4)->counterMoves;
	Color c = pos_.sidetomove();
	//ptrdiff_t num_move = end_ - move_;
//	int j = 0;
	//ここでbadcaptureにも点数をつけてしまっているな...
	//currentに変えたのでbadcaptureに点数が付かないはずである
	for (ExtMove* i = current_; i < end_; i++) {

		const Move m = i->move;
		Piece pc = moved_piece(m);
		Square to = move_to(m);
		ASSERT(is_ok(pc));
		ASSERT(is_ok(to));
		//move_[i].value = history[pc][to];
		i->value = history[pc][to]
			+ (cm ? (*cm)[pc][to] : Value_Zero)
			+ (fm ? (*fm)[pc][to] : Value_Zero)
			+ (f2 ? (*f2)[pc][to] : Value_Zero)
			+ fromTo.get(c, m);
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
		//ASSERT(NO_PIECE <= capturedpt&&capturedpt < PT_ALL);//propawnの場合はNO_pieceを許容

		i->value = Value(Eval::piece_value[capturedpt])-LVA(movept);
		
		if (is_promote(m)) {
			ASSERT(movept <= GOLD);
			i->value += Value(Eval::diff_promote[movept]);
		}
	}

}

void movepicker::eversion_score()
{
	const HistoryStats& history=pos_.searcher()->history;
	const FromToStats& fromTo = pos_.searcher()->fromTo;

	Value see;

	for (ExtMove* i = move_; i < end_; i++) {

		const Move m = i->move;
		const Piece movept = piece_type(moved_piece(m));
		const Piece capturedpt = piece_type(pos_.piece_on(move_to(m)));
		//piece_color(pcboard[move_to(m)]) == sidetomove()
		//ASSERT(piece_color(pos_.piece_on(move_to(m))) != pos_.sidetomove());
		//if (pos_.piece_on(move_to(m)) != NO_PIECE) {

		//	/*if (piece_color(pos_.piece_on(move_to(m))) == pos_.sidetomove()) {
		//		cout << pos_ << endl;
		//		check_move(m);
		//		ASSERT(0);
		//	}*/
		//	
		//}

		/*if ((see = pos_.see_ge(m,Value_Zero))<Value_Zero) {
			i->value = see - HistoryStats::Max;
		}*/
		if (pos_.capture_or_propawn(m)) {
			i->value = Value(Eval::piece_value[capturedpt]) - LVA(movept) + HistoryStats::Max;
			if (is_promote(m)) {
				ASSERT(movept <= GOLD);
				i->value += Value(Eval::diff_promote[movept]);
			}
		}
		else {
			i->value = history[movept][move_to(m)]+fromTo.get(pos_.sidetomove(),i->move);
		}
	}

}

Move movepicker::pick_best(ExtMove * begin, ExtMove * end)
{
	std::swap(*begin, *std::max_element(begin, end));
	return begin->move;
}

//通常探索用コンストラクタ

 movepicker::movepicker(const Position & pos, Stack * ss_, Move ttm, Depth d) :pos_(pos), ss(ss_), depth_(d) {

	current_ = end_ = move_;

	if (pos.is_incheck()) {
		st = START_Eversion;
	}
	else {
		st = START_Normal;
		Square prevSq = move_to((ss - 1)->currentMove);

		countermove = pos.searcher()->counterMoves[moved_piece((ss - 1)->currentMove)][prevSq];
		killers[0] = ss->killers[0];
		killers[1] = ss->killers[1];
		
	}
	ttMove = (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
	if (ttMove == MOVE_NONE) { st++; }
}

 //静止探索用コンストラクタ
 movepicker::movepicker(const Position& pos, Square recapsq, Move ttm, Depth d, Stack * ss_):pos_(pos),ss(ss_) {
	 ASSERT(d <= DEPTH_ZERO);
	 current_ = end_ = move_;


	 if (pos.is_incheck()) {
		 st = START_Eversion;
		 ttMove = (ttm && pos.pseudo_legal(ttm)&&pos.is_legal(ttm)) ? ttm : MOVE_NONE;
		 //end_ += (ttMove != MOVE_NONE);
		 if (ttMove == MOVE_NONE) { st++; }
	 }
	 else {
		 if (d>DEPTH_QS_NO_CHECKS) {
			 st = START_Q_WITH_CHECKS;
			 ttMove = (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
			// end_ += (ttMove != MOVE_NONE);
			 if (ttMove == MOVE_NONE) { st++; }
		 }
		 else if (d > DEPTH_QS_RECAPTURES) {
			 st = START_Q_CAP_PROPAWN;
			 ttMove= (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
			// end_ += (ttMove != MOVE_NONE);
			 if (ttMove == MOVE_NONE) { st++; }

		 }
		 else {
			 st = START_Q_RECAPTURE;
			 recapsq_ = recapsq;
			/* ttMove = (ttm && pos.pseudo_legal(ttm)) ? ttm : MOVE_NONE;
			 end_ += (ttMove != MOVE_NONE);*/
			 //ttMove = MOVE_NONE;
		 }
	 }
 }

 movepicker::movepicker(const Position & pos, Move ttm, Value th) :pos_(pos), Threshold(th) {


	 ASSERT(pos.is_incheck() == false);
	 current_ = end_ = move_;
	 st = Start_Probcut;

	 ttMove = (ttm != MOVE_NONE
		 &&pos_.pseudo_legal(ttm)
		 && pos_.capture(ttm)
		 //&&pos_.capture_or_propawn(ttm)
		 && pos.see_ge(ttm, Threshold)) ? ttm : MOVE_NONE;

	 if (ttMove == MOVE_NONE) { st++; }
	 //end_ += (ttMove != MOVE_NONE);
 }