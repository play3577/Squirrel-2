#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"
#include "tpt.h"
SearchLimit limit;
Signal signal;

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

void check_time() {
	TimePoint now_ = now();
	if (now_ - limit.endtime > 10) {
		signal.stop = true;
	}
}

Value Thread::think() {

	if (end == RootMoves) {
#ifndef LEARN
		cout << "bestmove resign" << endl;
#endif // !LEARN
		return Value_Mated;
	}

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	std::memset(stack, 0,(MAX_PLY+7)*sizeof(Stack));
	Value bestvalue, alpha, beta;
	bestvalue = alpha = Value_Mated;
	beta = Value_Mate;
	rootdepth = 0;
	int maxdepth;

	seldepth = 0;

	//時間制御
	signal.stop = false;
	this->resetCalls = false;
	this->call_count = 0;
#ifndef  LEARN
	//超テキトーな時間制御のコードあとで何とかする
	if (limit.remain_time[rootpos.sidetomove()] / 40 < limit.byoyomi) {
		limit.endtime = limit.byoyomi + limit.starttime;
	}
	else {
		limit.endtime = limit.remain_time[rootpos.sidetomove()] / 40 + limit.starttime;
	}
#endif // ! LEARN

	

	//cout << limit.endtime << endl;
#ifdef LEARN
	maxdepth = 2;
#endif
#ifndef LEARN
	maxdepth = MAX_DEPTH;
#endif // !LEARN


//	Eval::eval(rootpos);
	while (++rootdepth <maxdepth&&!signal.stop) {

		//ここで探索関数を呼び出す。
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY);

		sort_RootMove();

		if (signal.stop) {
			//cout << "signal stop" << endl;
			break;
		}

#ifndef LEARN
		print_pv(rootdepth,bestvalue);
		cout <<"評価値"<< int(bestvalue)*100/int(Eval::PawnValue) << endl;
#endif
	}//end of 反復深化
#ifndef LEARN
	cout << "bestmove " << RootMoves[0] << endl;
#endif // !LEARN

	return bestvalue;
}

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	ASSERT(alpha < beta);

	//初期化
	const bool PVNode = (NT == PV || NT == Root);
	const bool RootNode = (NT == Root);
	bool doFullDepthSearch = false;

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove;
	Move excludedmove;
	Value value;
	StateInfo si;
	Value staticeval;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
	Key poskey;
	bool TThit;
	TPTEntry* tte;
	ss->ply = (ss - 1)->ply + 1;
	Value bestvalue=-Value_Infinite;

#ifndef LEARN
	//timer threadを用意せずにここで時間を確認する。
	//stockfish方式
	if (thisthread->resetCalls.load(std::memory_order_relaxed)) {

		thisthread->resetCalls = false;
		thisthread->call_count = 0;

	}
	if (++thisthread->call_count > 4096) {
		//並列化のときはすべてのスレッドでこの操作を行う。
		thisthread->resetCalls = true;
		check_time();
	}
#endif
	//seldepthの更新をここで行う
	if (PVNode&&(thisthread->seldepth < pos.state()->ply_from_startpos+1)) {
		thisthread->seldepth = ss->ply;
	}

	if (!RootNode) {
		//step2
		if (signal.stop.load(std::memory_order_relaxed)) {
			return Eval::eval(pos);
		}

		// Step 3. Mate distance pruning. Even if we mate at the next move our score
		// would be at best mate_in(ss->ply+1), but if alpha is already bigger because
		// a shorter mate was found upward in the tree then there is no need to search
		// because we will never beat the current alpha. Same logic but with reversed
		// signs applies also in the opposite condition of being mated instead of giving
		// mate. In this case return a fail-high score.
		/*=============================================================================================================================
		たとえ次の指し手で相手を詰ませることが出来て評価値がmate-(ply+1)になったとしても
		もしこのゲーム木の上でより短い手数での詰みが見つかっていてalpha値がすでにその値（mate-(ply+1)）よりも大きくなっていれば
		現在のアルファ値を超えることは決してないのでコレ以上探索をする必要はない

		同じ論理（符号を反転させて）で詰みの代わりにつまされる場合を考えることができる。

		この場合fail-highの値を返す。

		（要約）４手詰めが見つかっている場合に５手詰めを探す必要はない
		===============================================================================================================================*/

		//=====================================================================================================================
		//state->plyfrom_rootでは駄目！！startposからの手数になってしまう！！！（Stackにrootからの手数を格納するしか無いか）
		//=====================================================================================================================
		alpha = std::max(mated_in_ply(ss->ply), alpha);//alpha=max(-mate+ply,alpha)　alphaの値は現在つまされている値よりも小さくは成れない つまりalphaは最小でも-mate+ply
		beta = std::min(mate_in_ply(ss->ply + 1), beta);//beta=min(mate-ply,beta)  betaの値は次の指し手で詰む値よりも大きくはなれない　つまりbetaは最大でもmate-(ply+1)

		/*===================================================
		現在のnodeの深さをplyとする
		他のノードでn手詰みを見つけている場合alpha=mate-n
		n<ply+1の場合はalpha>betaでここでreturn できる
		====================================================*/
		if (alpha >= beta) {
			return alpha;
		}

	}
	// Step 4. Transposition table lookup. We don't want the score of a partial
	// search to overwrite a previous full search TT value, so we use a different
	// position key in case of an excluded move.
	/*
	除外する手がない探索の置換表を上書きするために特定の手を場外しての探索結果はほしくないので、
	exclude moveがある場合には異なるhashkeyを用いる
	*/
	excludedmove = ss->excludedMove;
	poskey = pos.key() ^ Key(excludedmove);
	tte = TT.probe(poskey, TThit);
	if (TThit) {
		cout << "TThit" << endl;
	}





	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

	//評価関数は毎回呼び出したほうが差分計算でお得
	staticeval = Eval::eval(pos);

	//ここに前向き枝切りのコードを書く

	

	movepicker mp(pos);

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }

		if (pos.check_nihu(move) == true) {

			cout << "nihu "<< endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black"<<endl<<pos.pawnbb(BLACK) << endl;
			cout << "pbb white"<<endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}


		if (NT == Root&&thisthread->find_rootmove(move) == nullptr) { continue; }
		++movecount;
		//check_move(move);
		pos.do_move(move, &si);

		doFullDepthSearch = (PV&&movecount == 1);


		if (!doFullDepthSearch) {
			if (depth - ONE_PLY >= ONE_PLY) {
				//null window search
				value = -search<NonPV>(pos, ss + 1, -(alpha+1), -alpha, depth - ONE_PLY);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, depth - ONE_PLY);
			}
			doFullDepthSearch = (value > alpha);
		}


		if (doFullDepthSearch) {
			//ss->pvmove = move;
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;
			if (depth - ONE_PLY >= ONE_PLY) {
				value = -search<PV>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<PV>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
			}
		}
		//cout << "undo move " <<move<< endl;
		pos.undo_move();

		//時間切れなのでbestmoveとPVを汚さないうちに値を返す。
		if (signal.stop.load(std::memory_order_relaxed)) {
			return Value_Zero;
		}

		if (RootNode) {
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

			if (value > bestvalue) {
				bestvalue = value;
			}

			if (PVNode&&!RootNode) {
				update_pv(ss->pv, move, (ss + 1)->pv);
			}

			if (alpha >= beta) {
				break;
			}
		}
	}//指し手のwhile

	if (movecount == 0) {
		return mated_in_ply(ss->ply);
	}

	return alpha;

}

//静止探索(ここかなり時間を食うためできるだけ枝切りしたい)
//http://yaneuraou.yaneu.com/2016/11/03/%E6%8E%A2%E7%B4%A2%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E6%9E%9D%E5%88%88%E3%82%8A%E3%81%AE%E9%9A%8E%E5%B1%A4%E6%80%A7/
template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	const bool is_pv = (NT == PV);
	ASSERT(depth <= DEPTH_ZERO);
	ASSERT(alpha < beta);

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove;
	Value bestvalue;
	Value value;
	StateInfo si;
	Value staticeval;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();

	//ここに前向き枝切りのコードを書く


	//静止探索は駒の取り合いのような評価値が不安定な局面を先延ばして探索を続けることで探索の末端評価値を補正し、より正確にするものであると思っている。
	bestvalue=staticeval = Eval::eval(pos);

	
#ifdef LEARN
	//学習時は精子探索深さ３までで止める。（こんなことはせずバッサリ枝を切れる枝切り方法を導入したい）
	if (depth < -3 * ONE_PLY) { return bestvalue; }
#endif
	//コレで上手く前の指し手の移動先を与えられていると思う
	movepicker mp(pos, move_to(pos.state()->lastmove));

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }
		if (pos.check_nihu(move) == true) {

			cout << "nihu " << endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
			cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}


		movecount++;
		pos.do_move(move, &si);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();

		if (value > bestvalue) {
			//取り合いを読んだ分評価値はより正確になった
			bestvalue = value;

			//alpha値は底上げされた
			if (value > alpha) {
				alpha = value;

				//beta cut!
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	//王手をかけられていて合法手がなければそれは詰み
	if (incheck&&movecount == 0) {
		return mated_in_ply(ss->ply);
	}

	return bestvalue;
}
