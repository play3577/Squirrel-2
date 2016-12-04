#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"
#include "tpt.h"
SearchLimit limit;
Signal signal;


/*
この関数で詰み関連のスコアを「root nodeからあと何手で詰むか」から「今の局面から後何手で詰むか」に変換をする。
なぜかというと置換表から値を取り出すときにrootnodeからの手数が違う局面から取り出されることがあるので、
root nodeからあと何手で詰むかの情報ではおかしくなってしまうことがあるからである。
TTから読み出すときにvalue_from_ttをかますことでそのあたりを補正する。
*/
Value value_to_tt(Value v,int ply) {
	
	return v >= Value_mate_in_maxply ? v + ply :
		v <= Value_mated_in_maxply ? v - ply : v;
}

//これおかしいかもしれない
Value value_from_tt(Value v, int ply) {

	return v >= Value_mate_in_maxply ? v - ply :
		v <= Value_mated_in_maxply ? v + ply : v;
}

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
	if (now_  > limit.endtime -20) {
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
	Move excludedmove=MOVE_NONE;
	Value value;
	StateInfo si;
	Value staticeval;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
	ss->ply = (ss - 1)->ply + 1;
	Value bestvalue=-Value_Infinite;
	//-----TT関連
	//読み太ではTTから読み込んでいる途中でTTが破壊された場合のことも考えていたので、それを参考にする
	//Key poskey;
	bool TThit;
	TPTEntry* tte;
	Move ttMove;
	Value ttValue;
	Value ttEval;
	Bound ttBound;
	Depth ttdepth;

#ifndef LEARN
	//timer threadを用意せずにここで時間を確認する。
	//stockfish方式
	if (thisthread->resetCalls.load(std::memory_order_relaxed)) {

		thisthread->resetCalls = false;
		thisthread->call_count = 0;

	}
	//なかなか時間通りに指してくれなかったので2048で行く
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
#ifdef USETT
	excludedmove = ss->excludedMove;
	const Key poskey = pos.key() ^ Key(excludedmove);
	ASSERT((poskey & uint64_t(1)) == pos.sidetomove());
	//cout << (poskey>>32) << endl;
	tte = TT.probe(poskey, TThit);
	/*if (TThit) {
		cout << "TThit" << endl;
	}*/
	//アクセス競合が怖いので先に全部読み出しておく。
	if (TThit) {
		ttValue = tte->value();
		ttdepth = tte->depth();
		ttEval = tte->eval();
		ttBound = tte->bound();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
	}
	//ここでもしkey32の値が変わってしまっていた場合はttの値が書き換えられてしまっているので値をなかったことにする（idea from 読み太）(今のところone threadなのであまり意味はない)
	if (TThit && (poskey >> 32) != tte->key()) {
		
		cout << "Access Conflict" << endl;
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		TThit = false;
	}

	// At non-PV nodes we check for an early TT cutoff
	//non pv nodeで置換表の値で枝切り出来ないか試す。
	/*
	PVノードではなく
	置換表の残り探索深さのほうが大きく（depthは残り深さであるので深い探索による結果であるということ？）
	ttvalueがValueNoneでなく

	ttvalue=>betaのときはBOUND_LOWER|BOUNDEXACTであれば真の値はbeta以上であるとみなせるので枝切りできる。
	ttvalue<betaのときはBOUND_UPPER|BOUNDEXACTであれば真の値はbeta以下であることが確定している
	（nonPVnodeであるのでこれはnullwindowsearchでありalpha以下であることが確定したのでここで値を返しても良い）
	*/
	if (!PVNode
		&&TThit
		&&ttdepth>=depth
		&&ttValue != Value_error//これ今のところいらない(ここもっと詳しく読む必要がある)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWERであるのでどちらの&も満たす
		) {
		return ttValue;
	}
#endif
	

	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

	//評価関数は毎回呼び出したほうが差分計算でお得
	staticeval = Eval::eval(pos);

	//ここに前向き枝切りのコードを書く

	

	movepicker mp(pos);

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }

		//二歩が入ってこないことは確認した
		/*if (pos.check_nihu(move) == true) {

			cout << "nihu "<< endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black"<<endl<<pos.pawnbb(BLACK) << endl;
			cout << "pbb white"<<endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}*/


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
		/*if (value > alpha) {
			alpha = value;
			
			if (value > bestvalue) {
				bestvalue = value;
				bestMove = move;
			}

			if (PVNode&&!RootNode) {
				update_pv(ss->pv, move, (ss + 1)->pv);
			}

			if (alpha >= beta) {
				break;
			}
		}*/
		if (value > bestvalue) {

			bestvalue = value;
			if (value > alpha) {
				bestMove = move;
				if (PVNode && !RootNode) {
					update_pv(ss->pv, move, (ss + 1)->pv);
				}
				//PVnodeでなければならないのはnonPVではnullwindowsearchだから
				if (PVNode&&value < beta) {
					alpha = value;
				}
				else {
					//これはPVでもnonPVでも成り立つ（null window）
					ASSERT(value >= beta);
					break;
				}
			}
		}

	}//指し手のwhile

	
	if (movecount == 0) {
		bestvalue= excludedmove?alpha: mated_in_ply(ss->ply);
	}

#ifdef USETT
	//http://yaneuraou.yaneu.com/2015/02/24/stockfish-dd-search-%E6%8E%A2%E7%B4%A2%E9%83%A8/ 　から引用
	// value_to_ttは詰みのスコアの場合、現在の局面からの手数を表す数値に変換するためのヘルパー関数。
	// beta cutしているならBOUND_LOWER(他にもっといい値を記録する指し手があるかも知れないので)
	// PvNodeでかつbestMoveが具体的な指し手として存在するなら、BOUND_EXACT
	// non PV nodeとか、bestMoveがない(適当な見積もりで枝刈りした場合など)は、この状態

	// non PVだと、null windowで探索するので、仮にそれが[α-1,α]の範囲であれば、
	// αを超えるか超えないかしかわからない。αを下回る値が返ってきたなら、このnodeの真の値は、
	// その値以下であるから、BOUND_UPPER、また、αを上回る値が返ってきたら、このnodeの真の値は、
	// その値以上であろうから、BOUND_LOWERである。
	// このように、non PVにおいては、BOUND_UPPER、BOUND_LOWERしか存在しない。(aki.さん)
	tte->save(poskey, value_to_tt(bestvalue, ss->ply),
		bestvalue >= beta ? BOUND_LOWER :
		PVNode&&bestMove ? BOUND_EXACT : BOUND_UPPER,
		depth, bestMove, staticeval, TT.generation());
#endif

	return bestvalue;


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


	//時間通りに指し手を指してくれなかったので精子探索でも時間を見る
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
		/*if (pos.check_nihu(move) == true) {

			cout << "nihu " << endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
			cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}*/


		movecount++;
		pos.do_move(move, &si);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();
#ifndef LEARN
		if (signal.stop.load(std::memory_order_relaxed)) {
			return alpha;
		}
#endif
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
