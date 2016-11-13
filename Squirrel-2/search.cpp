#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"

SearchLimit limit;

void update_pv(Move* pv, Move move, Move* childPv) {

	//pvの先頭ポインタをmoveにして
	//childpvがnullptrでなく、MOVENONEでない間
	//pvにchildmoveを格納していく
	//pvは今の局面からのpv
	for (*pv++ = move; childPv && *childPv != MOVE_NONE; )
		*pv++ = *childPv++;
	//pvの最後はMOVENONE
	*pv = MOVE_NONE;
}

Value Thread::think() {

	if (end == RootMoves) {
		return Value_Mated;
	}

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	Value bestvalue, alpha, beta;
	bestvalue = alpha = Value_Mated;
	beta = Value_Mate;
	rootdepth = 0;

	while (++rootdepth <13) {

		//ここで探索関数を呼び出す。
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY);

		sort_RootMove();

		print_pv(rootdepth, ss);
	}

	cout << "bestmove " << RootMoves[0] << endl;

	return bestvalue;

}

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	ASSERT(alpha < beta);

	//初期化
	const bool PVNode = (NT == PV || NT == Root);
	bool doFullDepthSearch = false;

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove;
	Value value;
	StateInfo si;
	Value staticeval;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

	//ここに前向き枝切りのコードを書く

	movepicker mp(pos);

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }
		if (NT == Root&&thisthread->find_rootmove(move) == nullptr) { continue; }
		++movecount;
		//check_move(move);
		pos.do_move(move, &si);

		doFullDepthSearch = (PV&&movecount == 1);


		if (!doFullDepthSearch) {
			if (depth - ONE_PLY > ONE_PLY) {
				value = search<NonPV>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
			}
			else {
				value = Eval::eval(pos);
			}
			doFullDepthSearch = (value > alpha);
		}


		if (doFullDepthSearch) {
			//ss->pvmove = move;
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;
			if (depth - ONE_PLY > ONE_PLY) {
				value = search<PV>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
			}
			else {
				value = Eval::eval(pos);
			}
		}
		//cout << "undo move " <<move<< endl;
		pos.undo_move();


		if (NT == Root) {
			ExtMove* rm = thisthread->find_rootmove(move);
			if (movecount == 1 || value > alpha) {
				rm->value = value;//指し手のスコアリング
				thisthread->pv.clear();
				thisthread->pv.push_back(move);
				for (Move* m = (ss + 1)->pv; *m != MOVE_NONE; ++m) {
					thisthread->pv.push_back(*m);
				}
			}
			else {
				rm->value = Value_Mated;
			}
		}

		//alpha超えの処理
		if (value > alpha) {
			alpha = value;

			if (PVNode&&NT != Root) {
				update_pv(ss->pv, move, (ss + 1)->pv);
			}

			if (alpha >= beta) {
				break;
			}

		}
	}//指し手のwhile

	if (movecount == 0) {
		return mated_in_ply(pos.state()->ply_from_root);
	}

	return alpha;

}
