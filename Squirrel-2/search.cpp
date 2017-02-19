#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"
#include "tpt.h"
#include "book.h"
#include <random>


#if defined(_MSC_VER)
#endif
#if defined(__GNUC__) 
#include <algorithm>
#endif

#ifdef USETT
#define Probcut
#endif

//#define PREF2
SearchLimit limit;
Signal signal;






inline Value bonus(const Depth d1) { int d = d1 / ONE_PLY; return Value(d*d + 2 * d - 2); }
void update_cm_stats(Stack* ss, Piece pc, Square s, Value bonus);
void update_stats(const Position& pos, Stack* ss, Move move, Move* quiets, int quietsCnt, Value bonus);


template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);



//d手で挽回できる評価値の予測(dは最大でも6)
//６手先までこの関数で予測できるというのはどうなんだろうか...もう少し浅いほうがいい気がする...
//後は進行度によってこの値をいじることができる気がする...
//序盤１４０終盤160みたいな....
/*
進行度によってmargin値を変えるためには激指のように学習時のmarginを10+258*progressみたいに進行度が大きくなるに連れて大きくすることが必要か？？
進行度が大きくなるに連れてsigmoidの傾斜が存在する範囲を広くするのも後半の棋譜以外の指し手のPP値が教師手のそれと大きく離れるようにするためにありかもしれない...
*/
Value futility_margin(Depth d) { ASSERT(d < 7*ONE_PLY); return Value(150 * d / ONE_PLY); }

//d手で挽回できる点数であるのでだんだんと大きくならないとおかしい.(局所最適解に陥っている？（by屋根さん）)
const int razor_margin[4] = { 483, 570, 603, 554 };


//探索用の定数
/*
rootを3にしたほうがメモリの節約になっていいか？？やっぱだめだな
*/
int FutilityMoveCounts[2][16]; // [improving][depth]
//これは小さいほうが条件が緩い
int Reductions[3][2][64][64];  // [pv][improving][depth][moveNumber]

template <bool PvNode> Depth reduction(bool i, Depth d, int mn) {
	return Reductions[PvNode][i][std::min(int(d / ONE_PLY), 63)][std::min(mn, 63)] * ONE_PLY;
}

//ここ後でマルチスレッドように変える
void search_clear(Thread& th) {

#ifndef LEARN
	TT.clear();
#endif
	th.CounterMoveHistory.clear();

	
	th.fromTo.clear();
	th.history.clear();
	th.counterMoves.clear();

}

//探索乗数の初期化
//値をもっと真剣に見ていく
//値は小さいほうが枝切りが緩い
void search_init() {

	for (int imp = 0; imp <= 1; ++imp) {
		for (int d = 1; d < 64; ++d) {
			for (int mc = 1; mc < 64; ++mc)
			{
				double r = log(d) * log(mc) / 2;
				//double r = log(d) * log(mc) / 2.5;//少し緩くしてみる。
				if (r < 0.80) {
					continue;
				}
				Reductions[NonPV][imp][d][mc] = int(std::round(r));
				Reductions[PV][imp][d][mc] = std::max(Reductions[NonPV][imp][d][mc] - 1, 0);

				// Increase reduction for non-PV nodes when eval is not improving
				if (!imp && Reductions[NonPV][imp][d][mc] >= 2) {
					Reductions[NonPV][imp][d][mc]++;
				}
			}
		}
	}
	for (int d = 0; d < 16; ++d)
	{
		FutilityMoveCounts[0][d] = int(2.4 + 0.773 * pow(d + 0.00, 1.8));
		FutilityMoveCounts[1][d] = int(2.9 + 1.045 * pow(d + 0.49, 1.8));
		//すこし緩くしてみる
		//これが大きいほうが条件が緩い
		//FutilityMoveCounts[0][d] = int(3.4 + 0.773 * pow(d + 0.00, 1.8));
		//FutilityMoveCounts[1][d] = int(3.9 + 1.045 * pow(d + 0.49, 1.8));
	}

}


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

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	std::memset(stack, 0, (MAX_PLY + 7) * sizeof(Stack));
	Value bestvalue, alpha, beta,delta;

	if (end == RootMoves) {
#ifndef LEARN
		cout << "bestmove resign" << endl;
#endif // !LEARN
		return Value_Mated;
	}

	Move pondermove;

	//ここにbookを探すコードを入れる
	if (Options["usebook"]==true) {

		const string sfen = rootpos.make_sfen();
		auto bookentry = book.find(sfen);

		if (bookentry != book.end()) {
			std::vector<BookEntry> entrys = bookentry->second;

			if (Options["randombook"]) {
				std::random_device rd;
				std::mt19937 mt(rd());
				RootMoves[0].move = entrys[mt() % entrys.size()].move;
				pondermove = entrys[mt() % entrys.size()].counter;
			}
			else {
				RootMoves[0].move = entrys[0].move;
				pondermove = entrys[0].counter;
			}

			if (limit.is_ponder == false) {
				signal.stop = true;
			}
			cout << "book move" << endl;
			goto ID_END;

		}//bookを見つけた場合

		
	}//end book

#ifdef USETT
	TT.new_search();
#endif
	
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
	if ((bool)Options["use_defined_time"] == true) {
		limit.endtime = Options["defined_time"] + limit.starttime;
	}
	//超テキトーな時間制御のコードあとで何とかする
	else if (limit.remain_time[rootpos.sidetomove()] / 40 < limit.byoyomi) {
		limit.endtime = limit.byoyomi + limit.starttime;
	}
	else {
		limit.endtime = limit.remain_time[rootpos.sidetomove()] / 40 + limit.starttime;
	}
#endif // ! LEARN

	

	//cout << limit.endtime << endl;
#ifdef LEARN
	maxdepth = 4;//この値-1が実際に探索される深さ
	alpha = this->l_alpha;
	beta = this->l_beta;
#endif
#ifndef LEARN
	maxdepth = MAX_DEPTH;
#endif // !LEARN


//	Eval::eval(rootpos);
	while (++rootdepth <maxdepth&&!signal.stop) {

		/*if (rootdepth >= 5) {
			delta = Value(40);
			alpha = std::max(previousScore - delta, -Value_Infinite);
			beta = std::min(previousScore + delta, Value_Infinite);
		}*/
research:
		//ここで探索関数を呼び出す。
		ASSERT(alpha < beta);
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY,false);

		sort_RootMove();

		if (signal.stop) {
			//cout << "signal stop" << endl;
			break;
		}

		/*if (bestvalue < alpha) {
			beta = (alpha + beta) / 2;
			alpha = std::max(bestvalue - delta, -Value_Infinite);
			delta += delta / 4 + 5;
			goto research;
		}
		else if (bestvalue >= beta)
		{
			alpha = (alpha + beta) / 2;
			beta = std::min(bestvalue + delta, Value_Infinite);
			delta += delta / 4 + 5;
			goto research;
		}*/



#ifndef LEARN
		print_pv(rootdepth,bestvalue);
		//cout <<"評価値"<< int(bestvalue)*100/int(Eval::PawnValue) << endl;
#endif
	}//end of 反復深化
	sort_RootMove();
ID_END:

#ifndef LEARN
	//print_pv(rootdepth, bestvalue);
	cout << "bestmove " << RootMoves[0] << endl;
#endif // !LEARN
	previousScore = RootMoves[0].value;
	return bestvalue;
}

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth,bool cutNode) {

	ASSERT(alpha < beta);

	

	//初期化
	const bool PVNode = (NT == PV || NT == Root);
	const bool RootNode = (NT == Root);
	bool doFullDepthSearch = false;

	Move pv[MAX_PLY + 1];
	Move move = MOVE_NONE;
	Move bestMove=MOVE_NONE;
	Move excludedmove=MOVE_NONE;
	Move Quiets_Moves[64];
	int quiets_count=0;
	Value value,null_value;
	StateInfo si;
	Value staticeval=Value_Zero;
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
	bool CaptureorPropawn;
	bool givescheck, improve, singler_extension, move_count_pruning;
	Depth extension, newdepth;

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
	if (PVNode&&(thisthread->seldepth < ss->ply)) {
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

	//ssは過去の情報を消しておく必要がある
	ss->currentMove = (ss + 1)->excludedMove=bestMove = MOVE_NONE;
	ss->counterMoves = nullptr;
	(ss + 1)->skip_early_prunning = false;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;

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
		//ttMove = tte->move();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		//ttMove = MOVE_NONE;
	}
	//Rootのときは違う処理を入れなければならない
	ttMove = RootNode ? thisthread->RootMoves[0].move : TThit ? tte->move() : MOVE_NONE;



	//この処理を入れるとクッソ遅くなってしまう(´･_･`)　まあ途中でTTの値が書き換わってしまうということについては考えないほうがいいのかもしれない
	//ここでもしkey32の値が変わってしまっていた場合はttの値が書き換えられてしまっているので値をなかったことにする（idea from 読み太）(今のところone threadなのであまり意味はない)
	/*if (TThit && (poskey >> 32) != tte->key()) {

		cout << "Access Conflict" << endl;
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		TThit = false;
	}*/

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
		&&ttdepth >= depth
		&&ttValue != Value_error//これ今のところいらない(ここもっと詳しく読む必要がある)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWERであるのでどちらの&も満たす
		//&& (pos.piece_on(move_to(ttMove)) == NO_PIECE || piece_color(pos.piece_on(move_to(ttMove))) != pos.sidetomove())//ここでこの局面で非合法手だったら省く
		) {
		ss->currentMove = ttMove; // Can be MOVE_NONE
		//ttMoveがquietでttvalue>=betaであればhistoryを更新することができる。
		if (ttValue >= beta&&ttMove != MOVE_NONE) {
			//hashの偶然一致でここでバグって落ちることが起こったが、レアケースであるため無視をすることにする

			/*if (pos.piece_on(move_to(ttMove)) != NO_PIECE && piece_color(pos.piece_on(move_to(ttMove))) == pos.sidetomove()) {

				ASSERT(0);
			}*/
			if (pos.capture_or_propawn(ttMove)==false) {
				update_stats(pos, ss, ttMove, nullptr, 0, bonus(depth));
			}
			// Extra penalty for a quiet TT move in previous ply when it gets refuted
			//やり返されてしまった以前のquietなTTmoveに対してペナルティーをかける
			//※(ss-1)->moveCount == 1 なのでTTmoveとは限らないのではないか？？
			if ((ss - 1)->moveCount == 1 && pos.state()->DirtyPiece[1]==NO_PIECE&& (ss - 1)->currentMove!=MOVE_NULL)
			{
				int d = depth / ONE_PLY;
				Value penalty = Value(d * d + 4 * d + 1);
				Square prevSq = move_to((ss - 1)->currentMove);
				update_cm_stats(ss - 1, moved_piece((ss-1)->currentMove), prevSq, -penalty);
			}


		}

		return ttValue;
	}



#endif

#define MATEONE

#ifdef MATEONE
	if (!RootNode && !incheck) {
		if (pos.mate1ply()) {
			ss->static_eval = bestvalue = mate_in_ply((ss->ply)+1);
#ifdef USETT
			tte->save(poskey, value_to_tt(bestvalue, ss->ply), BOUND_EXACT, depth, MOVE_NONE, ss->static_eval, TT.generation());
#endif
			return bestvalue;
		}
	}
#endif //mateone

	

	//評価関数は毎回呼び出したほうが差分計算でお得
	//毎回評価関数を呼び出すのでSFのようにTTのevalとどちらが信用性があるか比較する必要はないと思う。
	//そうなるとtt->eval()は保存する必要が無いか...???う〜〜んもしかしたらtt.eval()を用いたほうがメリットが有るのかもしれない...（ここは後でよく考えよう）
	ss->static_eval=staticeval = Eval::eval(pos);

	//stockfish風にしてみる
#if 0
	//王手がかかっている場合は前向き枝切りはしない
	//（もし前向き枝切りが出来てしまったらこのノードで詰んでしまう）
	if (incheck) {
		//	cout << "incheck" << endl;
		ss->static_eval = staticeval = Value_error;
		goto moves_loop;
	}
	else if (TThit) {

		if (ttValue != Value_error) {
			if (ttBound&(ttValue > staticeval ? BOUND_LOWER : BOUND_UPPER)) { staticeval = ttValue; }
		}

	}
	else {
		if ((ss - 1)->currentMove == MOVE_NULL) { staticeval = ss->static_eval = -(ss - 1)->static_eval + 2 * (Value)20;}
#ifdef USETT
		tte->save(poskey, Value_error, BOUND_NONE, DEPTH_NONE, MOVE_NONE, ss->static_eval, TT.generation());
#endif
	}
#else
	if (incheck) {
		//	cout << "incheck" << endl;
		goto moves_loop;
	}
	else if ((ss - 1)->currentMove == MOVE_NULL) {
		staticeval = ss->static_eval = -(ss - 1)->static_eval + 2 * (Value)20;
	}
#endif


	if (ss->skip_early_prunning == true) {
		goto moves_loop;
	}
#ifdef USETT
	// Step 6. Razoring (skipped when in check)
	/*
	ttMoveが存在せず
	残り深さが４以下で
	静止探索の値にrazormargin[]を足してもalphaを上回れない場合は枝切りできる

	静止探索の値を返すので正確な戻り値を期待できる？？（このノードにおける）

	ここもう少し色々考えられるけどrazoringを除いてもELO-10にしか成らなかったらしいのでそこまで力を入れてチューニングするメリットはない
	*/
	if (!PVNode
		&&  depth < 4 * ONE_PLY
		&&  ttMove == MOVE_NONE
		&&  staticeval + razor_margin[depth / ONE_PLY] <= alpha)
	{

		//残りが一手以下で静的評価値にrazormargin[3]を足してもalphaを上回れない場合は枝切りできる
		//ここなんでmargin[3]??? 他の値を用いたほうがいいのでは？
		//razormargin[3]が二箇所で使われているので変な値になってしまっている？？

		//stackはss+1でなくssでいいのか？？？（do_moveをしていないので構わない（ss->staticEvalなどの情報を使いまわせる！！！！））
		if (depth <= ONE_PLY
			&& staticeval + razor_margin[3 * ONE_PLY] <= alpha) {
			return qsearch<NonPV>(pos, ss, alpha, beta, DEPTH_ZERO);
		}

		Value ralpha = alpha - razor_margin[depth / ONE_PLY];
		Value v = qsearch<NonPV>(pos, ss, ralpha, ralpha + 1, DEPTH_ZERO);
		if (v <= ralpha) {
			return v;
		}
	}

#endif

	// Step 7. Futility pruning: child node (skipped when in check)
	/*------------------------------------------------------------------------------------------------------------
	大抵の枝はここで刈られる（by出村さん）
	つまり他の枝切りを頑張ってチューニングするよりもここの枝切りを頑張ってチューニングしてやるほうが効果が大きい。


	d手で挽回できる評価値の予測がfutilitymargin.(dは最大でも6)
	もし残り探索深さで相手が挽回できる点数を現在の静的評価値から引いてもbetaを超えている場合はここで枝を切ってしまってもいい。
	futility margin が重要になってくる。評価値は深さに線形に大きくなっていく

	//６手先までこの関数で予測できるというのはどうなんだろうか...もう少し浅いほうがいい気がする...
	//後は進行度によってこの値をいじることができる気がする...
	//序盤１４０終盤160みたいな....
	-------------------------------------------------------------------------------------------------------------*/
	if (!RootNode
		&&depth < 7 * ONE_PLY
		&&staticeval - futility_margin(depth) >= beta
		&& staticeval < Value_known_win
		//nonpawnmaterialという条件は将棋には関係ないはず
		)
	{
		return staticeval - futility_margin(depth);
	}

	// Step 8. Null move search with verification search
	/*
	（私の中での理解）
	静的評価値がbetaを超えている局面（相手がヘマをしてきたと思われる局面（長期的に見れば計算された手かもしれない））でこちらが割りと良い手を返したとして浅い探索をしてみる。
	探索から帰ってきた結果がそれでもbetaを超えていた場合（つまりホントに相手がヘマをしたとわかった）はそれは枝切りをできる。
	パスでbetaを超えたということは、パスよりも良い指してを指してみてもbetaを超えるということである。（局面はZugzwangではないと仮定する）
	割りと良い手というのにはパスをする手を用いる。殆どの指してはパスに劣る。
	*/

	if (!PVNode
		&&staticeval >= beta
		&& (staticeval >= beta - 35 * (int(depth / ONE_PLY) - 6) || depth >= 13 * ONE_PLY)
		) {
		ss->currentMove = MOVE_NULL;
		ss->counterMoves = nullptr;
		ASSERT(staticeval >= beta);




		//Rの値は場合分けをして後で詳しく見る
		//staticevalに大きな値が付きすぎていてRが大きくなりすぎている！（どどど　どうしよう...）(しかし一致率表示させようとしてる時だけに起こる現象なのでメモリエラーが原因なのかもしれない)
		Depth R= Depth(((823 + 67 * int(depth / ONE_PLY)) / 256 + std::min(int(staticeval - beta) / Eval::PawnValue, 3)) * ONE_PLY);

		pos.do_nullmove(&si);
		(ss + 1)->skip_early_prunning = true;//前向き枝切りはしない。（２回連続パスはよろしくない）

		//nullmoveは取り合いが起こらないので静止探索を呼ばない。（静止探索で王手も探索するように成れば実装を変更する）
		null_value = (depth - R) < ONE_PLY ? staticeval : -search<NonPV>(pos, ss + 1, -(beta), -beta + 1, depth - R,!cutNode);
		(ss + 1)->skip_early_prunning = false;
		pos.undo_nullmove();

		//null_valueがbetaを超えた場合
		if (null_value >= beta) {

			//NULLMOVEなので詰みを見つけてもそのまま帰すとおかしくなる
			if (null_value >= Value_mate_in_maxply) {
				null_value = beta;
			}

			//(staticeval >= beta - 35 * int(depth / ONE_PLY - 6)の条件を持たしているときなのでホントにbetaを超えるかのチェックはしなくてもいい
			if (depth < 12 * ONE_PLY&&std::abs(beta) < Value_known_win) {
				return null_value;
			}

			//残りdepthが大きいときであるのでちゃんと確認しておく。
			//またここに入ってこないように前向き枝切りには入らないようにする。
			ss->skip_early_prunning = true;
			//do_moveをしていないのでss,betaでよい。
			Value v= (depth - R) < ONE_PLY ? staticeval : search<NonPV>(pos, ss ,beta-1, beta, depth - R,false);
			ss->skip_early_prunning = false;

			if (v >= beta) {
				return null_value;
			}
		}

	}

#ifdef   multicut 
	//step9 multicut 
	//nullmoveよりも弱くなってしまった！
	//もっと条件を考える
	/*
	SDT4でツツカナとSeleneが行っていたという手法
	よくわからんけどprobcutと似た考えに基づく枝切り法だと思うのでここで行ってみる。
	StockFishではmulticutがProbcut(Stockfish方式)より良い結果を残したことはないみたい
	https://groups.google.com/forum/?fromgroups=#!searchin/fishcooking/Probcut%7Csort:relevance/fishcooking/mpssXCKNuFo/T34cijXSfF8J
	https://chessprogramming.wikispaces.com/Multi-Cut
	*/
	if (!PVNode
		&&depth >= 5 * ONE_PLY
		&&std::abs(beta) < Value_mate_in_maxply)
	{
		//Value rbeta = std::min(beta + 200, Value_Infinite);
		Depth rdepth = depth - 4 * ONE_PLY;
		ASSERT(rdepth >= ONE_PLY);
		ASSERT(pos.state()->lastmove != MOVE_NULL);

		//ここでどんな指してを生成すべきなのか....
		//capturepropawn??killerも含めてみるのもありかもしれない
		movepicker mp_prob(pos,beta);

		int c = 0;
		const int overbeta= 3;
		
		
		Value bestvalue2 = -Value_Infinite;

		while ((move = mp_prob.return_nextmove()) != MOVE_NONE) {
			//生成された指しての数が少なかった場合はmulticutは出来ない。
			//domoveする前にgotoする
			if (mp_prob.num_move() < overbeta) { goto end_multicut; }
			if (!pos.is_legal(move)) { continue; }
			
			pos.do_move(move, &si);
			value = -search<NonPV>(pos, (ss + 1), -beta, -beta + 1, rdepth);
			pos.undo_move();

			if (value > bestvalue2) {
				bestvalue2 = value;
				if (value > beta) {
					c = c + 1;
					//fail soft
					if (c >= overbeta) return bestvalue2;
				}
			}//end of value>bestvalue2
		}
	}

end_multicut:


#endif

#ifdef Probcut

	if (!PVNode
		&&depth >= 5 * ONE_PLY
		&&std::abs(beta) < Value_mate_in_maxply)
	{
		Value rbeta = std::min(beta + 200, Value_Infinite);
		Depth rdepth = depth - 4 * ONE_PLY;

		ASSERT(rdepth >=ONE_PLY);
		
		//ここprobcutように変える
		movepicker mp(pos,ttMove,rbeta-staticeval);

		while ((move = mp.return_nextmove()) != MOVE_NONE) {

			if (pos.is_legal(move)) {
				ss->currentMove = move;
				ss->counterMoves = &thisthread->CounterMoveHistory[moved_piece(move)][move_to(move)];
				pos.do_move(move, &si);
				value = -search<NonPV>(pos, (ss + 1), -rbeta, -rbeta + 1, rdepth,!cutNode);
				pos.undo_move();

				if (value >= rbeta) { return value; }
			}

		}
	}




#endif

	//内部反復深化
	/*
	https://chessprogramming.wikispaces.com/Internal+Iterative+Deepening

	内部反復深化は（過去のPVまたはTT）からプログラムがbestmoveを見つけられなかったゲームツリーのノードで行われる.
	IIDは現在の局面で深さを減らして探索をすることでよい差し手を見つけるために使われる。
	IIDは保険のようなものだ。ほとんどの場合それは必要ではない、しかしそのコストは非常に小さい。　そして大きな時間の無駄を避けることができる。
	*/
	/*
	深さは6以上
	ttMoveが存在しない
	PVNodeが存在しない||静的評価値はbeta+256　のとき

	前向き枝切りをオフにして、先に少し探索をしてttmoveを用意する


	これするとえげつないぐらい探索深さが増えますねぇ！！！
	もっと根底から理解すべきか
	*/
#ifdef USETT
	if (depth >= 6 * ONE_PLY
		&&ttMove == MOVE_NONE
		&& (PVNode || staticeval + 256 >= beta)
		) {

		Depth d = (3 * (int)depth / (4 * ONE_PLY) - 2)*ONE_PLY;
		ss->skip_early_prunning = true;
		//ここで帰ってきた値を何かに利用できないか？？？
		search<NT>(pos, ss, alpha, beta, d,cutNode);
		ss->skip_early_prunning = false;
		tte = TT.probe(poskey, TThit);
		//TTが汚されてしまう場合を考えてこうする
		if (TThit) {
			ttValue = tte->value();
			ttdepth = tte->depth();
			ttEval = tte->eval();
			ttBound = tte->bound();
			ttMove = tte->move();
		}
		else {
			ttValue = Value_error;
			ttdepth = DEPTH_NONE;
			ttEval = Value_error;
			ttBound = BOUND_NONE;
			ttMove = MOVE_NONE;
		}
	}




#endif



	//王手がかかっている場合は前向き枝切りはしない
	//（もし前向き枝切りが出来てしまったらこのノードで詰んでしまう）
moves_loop:
	
#ifndef  LEARN

#define EXTENSION

#endif // ! LEARN
	const CounterMoveStats* cmh = (ss - 1)->counterMoves;
	const CounterMoveStats* fmh = (ss - 2)->counterMoves;
	const CounterMoveStats* fmh2 = (ss - 4)->counterMoves;

			//静的評価値が前の自分の手番よりもよくなっているかまたは以前の評価値が存在しなかった
	improve = ss->static_eval >= (ss - 2)->static_eval || (ss - 2)->static_eval == Value_error;
#ifdef EXTENSION
		/*=================================================================================
				singularExtension
				その局面で良い差し手が一つしかないときにその指し手を延長させる
			
				条件
			
				rootnodeではない。（まあそりゃそうだ）
				深さは8以上（これはどうだろう....あまり考えがわからない）
				ttMoveが存在する（ttMoveはよい差し手になりうるはずなのでその指し手を延長したい）
				評価値がこれ以上よくなりうる可能性がある
				延長された差し手をまた延長するのはよろしくない
				ttvalueはttdepthがdepth-3*ONE_PLYで割と保証されている
		===================================================================================*/
	singler_extension = !RootNode
		 &&depth >= 8 * ONE_PLY
		&&ttMove != MOVE_NONE
		&&excludedmove != MOVE_NONE
		&& (ttBound&BOUND_LOWER)//LOWER or EXACT
		 && ttdepth >= depth - 3 * ONE_PLY;
#endif
		
		




#ifdef USETT
	movepicker mp(pos,ss,ttMove,depth);
#else 
	movepicker mp(pos, ss, MOVE_NONE,depth);
#endif
	//学習中countermovesにおかしな指し手が入っている
	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (move == ss->excludedMove) {continue;}
	
		if (!RootNode) {
			if (pos.is_legal(move) == false) { continue; }
		}


		if (NT == Root&&thisthread->find_rootmove(move) == nullptr) { continue; }

	

		/*
		capture propawnの指し手になりうるのはcappropawnのステージとEVERSIONのステージだけ
		*/
		Stage st = mp.ret_stage();
		if (st == CAP_PRO_PAWN||st==BAD_CAPTURES) {
			//ASSERT(pos.capture_or_propawn(move) == true); 今のところassertなくても大丈夫そう
			CaptureorPropawn = true;
		}
		else if (st == QUIET) {
			//ASSERT(pos.capture_or_propawn(move) == false);
			CaptureorPropawn = false;
		}
		else {
			CaptureorPropawn = pos.capture_or_propawn(move);
		}

		ss->moveCount=++movecount;
		extension = DEPTH_ZERO;
		givescheck = pos.is_gives_check(move);

		 move_count_pruning = depth < 16 * ONE_PLY&&movecount >= FutilityMoveCounts[improve][depth / ONE_PLY];

#ifdef EXTENSION
					//=====================
					//差し手の延長
					//=====================
			
			
				// Step 12. Extend checks
					/*=========================================================================================================
					王手をかける指し手を延長する
					
					条件
					王手をかける指し手
					後ろのほうの差し手でない
					seeが正である
					
					技巧では王手をかけられた局面に許される合法手の数は少ないのでseeが負の局面でも王手をHALF_PLY延長するらしいそれに習う。
					==========================================================================================================*/
			
			if (givescheck
				 && !move_count_pruning) {
				if (pos.see_sign(move) >= Value_Zero) { extension = ONE_PLY; }
				else { extension = HALF_PLY; }
			}
			//singler extension
			/*
			この局面でttmove以外の差し手以外に良い差し手がなければttmoveを延長する
			*/
			if (singler_extension
				 &&move == ttMove
				&& !extension) {
			
				Value rBeta = ttValue - 2 * depth / ONE_PLY;
			Depth d = (depth / (2 * int(ONE_PLY)))*int(ONE_PLY);
			ss->excludedMove = move;
			ss->skip_early_prunning = true;
			value = search<NonPV>(pos, ss, rBeta - 1, rBeta, d,cutNode);
			ss->skip_early_prunning = false;
			ss->excludedMove = MOVE_NONE;
			if (value < rBeta) {
				extension = ONE_PLY;
				
			}
			
		}
#endif
		newdepth = depth - ONE_PLY + extension;
		

		//前向き枝切り
		//これは差し手の延長を考える前にすべきではないか？？？？
		if (!RootNode
			&& !incheck
			&&bestvalue > Value_mated_in_maxply) {

			if (!CaptureorPropawn
				&& !givescheck) {

				if (move_count_pruning) { continue; }
				Depth predicted_depth = std::max(newdepth - reduction<PVNode>(improve, depth, movecount), DEPTH_ZERO);

				// Countermoves based pruning
				if (predicted_depth < 3 * ONE_PLY
					&& (!cmh || (*cmh)[moved_piece(move)][move_to(move)] < Value_Zero)
					&& (!fmh || (*fmh)[moved_piece(move)][move_to(move)] < Value_Zero)
					&& (!fmh2 || (*fmh2)[moved_piece(move)][move_to(move)] < Value_Zero || (cmh && fmh))) {
					continue;
				}

				//親ノードのfutility
				if (predicted_depth < 7 * ONE_PLY
					&&staticeval + 256 + 200 * predicted_depth / int(ONE_PLY) <= alpha) {
					continue;
				}

				if (predicted_depth < 8 * ONE_PLY) {
					Value see_v = predicted_depth < 4 * ONE_PLY ? Value_Zero :
						Value(-Eval::PawnValue * 2 * int(predicted_depth - 3 * ONE_PLY) / ONE_PLY);
					if (pos.see_sign(move)<see_v) { continue; }
				}

			}
			else if (depth < 3 * ONE_PLY
				&& (mp.see_sign() < 0 || (!mp.see_sign() && pos.see_sign(move) < Value_Zero))) {
				continue;
			}

		}
		//これ早くなるんだろうか？？
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif

		ss->currentMove = move;

		ss->counterMoves = &thisthread->CounterMoveHistory[moved_piece(move)][move_to(move)];

		
		pos.do_move(move, &si, givescheck);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif

#if 0
		doFullDepthSearch = (PVNode&&movecount == 1);

		//この方法だとnonPVnodeからPVnodeに移ってこれるのでPVがおかしくなる可能性があるか？？？？？
		/*
		学習があまり強くならないのは学習部にバグがあるのではなく探索が悪くて、つまりこれが原因か！？
		*/
		if (!doFullDepthSearch) {
			if (newdepth >= ONE_PLY) {
				//null window search
				value = -search<NonPV>(pos, ss + 1, -(alpha+1), -alpha, newdepth, !cutNode);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, DEPTH_ZERO);
			}
			doFullDepthSearch = (value > alpha);
		}


		if (doFullDepthSearch) {
			//ss->pvmove = move;
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;
			if (newdepth >= ONE_PLY) {
				value = -search<PV>(pos, ss + 1, -beta, -alpha, newdepth,false);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<PV>(pos, ss + 1, -beta, -alpha, DEPTH_ZERO);
			}
		}
#endif
#if 1
		// Step 15. Reduced depth search (LMR). If the move fails high it will be
		// re-searched at full depth.
		// 深さを減らした探索。もしこの指し手が良い差し手であることが分かれば完全な深さで探索しなおす
		/*
		絶対にここで枝を刈りすぎている

		nonPVの指し手はPVになれないのもいかがなものかと思う。
		でもnonPVが勝手にPVになったらそれはPVがおかしくなるか...????でもそれでレーティング下がってるしなぁ...

		事故対戦を見てみたがlmrを入れると指し手に切れ味がなくなって
		相手の玉をなかなか詰めることができていないように感じた。
		終盤は入れないほうがいいか？
		適当に不を鳴らせる指し手も目立ったのでPAWNPROMOTEは含めないほうがいいのかも

		あんまり条件を緩くしすぎてもレーティング下がってしまった。
		*/
		if (depth >= 3 * ONE_PLY
			&& ((pos.ply_from_startpos + (ss)->ply)<50)
			&&movecount > 1
			&& (!CaptureorPropawn || move_count_pruning)
			&&mp.ret_stage()!=Killers
			)
		{
			//PVではreducationはしない その他ではreducationする
			Depth r = reduction<PVNode>(improve, depth, movecount);

			if (CaptureorPropawn) {
				r -= r ? ONE_PLY : DEPTH_ZERO;
			}
			else {

				// Increase reduction for cut nodes
				if (cutNode) {r += 2*ONE_PLY;}
				//捕獲から逃れる指し手の場合はreducationを減らす
				if (pos.see(make_move(move_to(move), move_from(move), PAWN))<Value_Zero) {
					r -= 2 * ONE_PLY;
				}

				Value val = thisthread->history[moved_piece(move)][move_to(move)]
					+ (cmh ? (*cmh)[moved_piece(move)][move_to(move)] : Value_Zero)
					+ (fmh ? (*fmh)[moved_piece(move)][move_to(move)] : Value_Zero)
					+ (fmh2 ? (*fmh2)[moved_piece(move)][move_to(move)] : Value_Zero)
					+ thisthread->fromTo.get(opposite(pos.sidetomove()), move);

				int rHist = (val - 8000) / 20000;
				r = std::max(DEPTH_ZERO, (int(r / ONE_PLY) - rHist)*ONE_PLY);

			}

			Depth d = std::max(newdepth - r, ONE_PLY);
			value = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, d,true);
			//d==newdepthの場合は同じ条件で再探索することになってしまう
			doFullDepthSearch = (value > alpha&&d != newdepth);
		}
		else {

			doFullDepthSearch = (!PVNode || movecount > 1);
		}


		if (doFullDepthSearch) {
			if (newdepth >= ONE_PLY) {
				//null window search
				value = -search<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, newdepth,!cutNode);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, DEPTH_ZERO);
			}

		}


		if (PVNode && (movecount == 1 || (value>alpha && (RootNode || value<beta)))) {
			//ss->pvmove = move;
			(ss + 1)->pv = pv;
			(ss + 1)->pv[0] = MOVE_NONE;
			if (newdepth >= ONE_PLY) {
				value = -search<PV>(pos, ss + 1, -beta, -alpha, newdepth,false);
			}
			else {
				//value = Eval::eval(pos);
				value = -qsearch<PV>(pos, ss + 1, -beta, -alpha, DEPTH_ZERO);
			}
		}

#endif

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

		//history値をつけるためにalphaを超えられなかった悪いquietの指しては格納する
		if (CaptureorPropawn == false && move != bestMove&&quiets_count < 64) {
			Quiets_Moves[quiets_count++] = move;
		}


	}//指し手のwhile

	
	if (movecount == 0) {
		bestvalue= excludedmove?alpha: mated_in_ply(ss->ply);
	}
	else if (bestMove != MOVE_NONE) {

		//bestmmoveはalpha値を超えるような指してのこと
		//アルファ値を超えるような有効な指し手があった場合
		//（βカットを起こしてしまった指してに対しても探索が短くなるという意味で良い値をつけるべきである）

		//行先に駒がないまたは自分の駒ではない
		/*if (pos.piece_on(move_to(bestMove)) != NO_PIECE && piece_color(pos.piece_on(move_to(bestMove))) == pos.sidetomove()) {
			ASSERT(0);
		}*/
		// Quiet best move: update killers, history and countermoves
		//bestmoveがQUIETであった場合はbestmoveに良い値をつけて他の指してに悪い値をつける
		if (!pos.capture_or_propawn(bestMove)) {
			update_stats(pos, ss, bestMove, Quiets_Moves, quiets_count, bonus(depth));
		}
		//Extra penalty for a quiet TT move in previous ply when it gets refutedやり返されてしまったTTの差し手に悪い値をつける。
		if ((ss - 1)->moveCount == 1 && pos.state()->DirtyPiece[1] == NO_PIECE && (ss - 1)->currentMove != MOVE_NULL) {
			int d = depth / ONE_PLY;
			Value penalty = Value(d * d + 4 * d + 1);
			Square prevSq = move_to((ss - 1)->currentMove);
			update_cm_stats(ss - 1, moved_piece((ss - 1)->currentMove), prevSq, -penalty);//移動した後の駒であるのでこれでいい。しかし駒種はchessと違い成りが含まれるかもしれないので移動前の駒種を使う
		}
	}
	// Bonus for prior countermove that caused the fail low
	//alphaを超えるような差し手がなかったということは相手の差し手はよい差し手であったので相手の差し手のcountermoveに良い値をつける
	else if (depth >= 3 * ONE_PLY && pos.state()->DirtyPiece[1] == NO_PIECE&& is_ok((ss - 1)->currentMove)&& (ss - 1)->currentMove!=MOVE_NULL) {
		int d = depth / ONE_PLY;
		Value bonus = Value(d * d + 2 * d - 2);
		Square prevSq = move_to((ss - 1)->currentMove);
		update_cm_stats(ss - 1, moved_piece((ss - 1)->currentMove), prevSq, bonus);
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

	const bool PvNode = (NT == PV);
	ASSERT(depth <= DEPTH_ZERO);
	ASSERT(alpha < beta);
	ASSERT(pos.state()->lastmove != MOVE_NULL);

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove=MOVE_NONE;
	Value bestvalue;
	Value value;
	StateInfo si;
	Value staticeval,oldAlpha;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
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
	Value futilitybase = -Value_Infinite;
	Value futilityvalue;
	bool evasionPrunable;
	if (PvNode)
	{
		// To flag BOUND_EXACT when eval above alpha and no available moves
		//評価値がalpha超え、かつ合法手がない場合のflagとして用いる。
		oldAlpha = alpha; 
		(ss + 1)->pv = pv;
		ss->pv[0] = MOVE_NONE;
	}
	//時間通りに指し手を指してくれなかったので精子探索でも時間を見る
	//取り合いの途中で評価を返されると実にまずいのでできるだけしたくないのだが....
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
	
#ifdef USETT
	const Key posKey = pos.key();
	ASSERT((posKey & uint64_t(1)) == pos.sidetomove());
	tte = TT.probe(posKey, TThit);
	if (TThit) {
		ttValue = tte->value();
		ttdepth = tte->depth();
		ttEval = tte->eval();
		ttBound = tte->bound();
		ttMove = tte->move();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		ttMove = MOVE_NONE;
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
	if (!PvNode
		&&TThit
		&&ttdepth >= depth
		&&ttValue != Value_error//これ今のところいらない(ここもっと詳しく読む必要がある)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWERであるのでどちらの&も満たす
		) {
		return ttValue;
	}
#endif
	if (!incheck) {

		if (pos.mate1ply()) {
			return mate_in_ply(ss->ply + 1);
		}

	}



	//ここに前向き枝切りのコードを書く


	//静止探索は駒の取り合いのような評価値が不安定な局面を先延ばして探索を続けることで探索の末端評価値を補正し、より正確にするものであると思っている。
	(ss)->static_eval=bestvalue=staticeval = Eval::eval(pos);

	if (incheck) {
		ss->static_eval = Value_error;
		bestvalue = futilitybase = -Value_Infinite;
	}
	else {
		/*
		if (TThit) {
			if (ttValue != Value_error) {
				if (ttBound&(ttValue > bestvalue ? BOUND_LOWER : BOUND_UPPER)) { bestvalue = ttValue; }
			}
		}
		else {
			if ((ss - 1)->currentMove == MOVE_NULL){staticeval=bestvalue=  -(ss - 1)->static_eval + 2 * Value(20);}
		}*/

		if (bestvalue >= beta) {
#ifdef USETT
			if (!TThit) {
				tte->save(posKey, value_to_tt(bestvalue, ss->ply), BOUND_LOWER, DEPTH_NONE, MOVE_NONE, staticeval, TT.generation());
			}
#endif
			return bestvalue;

		}

		if (PvNode&&bestvalue > alpha) {
			alpha = bestvalue;
		}

		futilitybase = bestvalue + 128;
	}

#if 0
	if (!incheck) {

		//stand pat 方式を試してみる。（もしあまりよろしくなければ採用を取り消す。）
		/*
		stand pat(ポーカーで指し書のカードを変更せずに手を決めるという意味の単語)

		この局面の静的評価値をvalueのlower boundとして用いる。
		普通は少なくとも1つの指してはこのlower bound以上の指してがあると考えられる。
		これはnullmove observation（局面には必ず1つはpass以上に良い指し手があるはずであるという考え）に基づいている。
		これは現在Zugzwang(パスが最も良い局面)にいないと考える場合（将棋にはpassが最も良い指し手になる局面はなかなか出てこないらしい））
		https://chessprogramming.wikispaces.com/Null+Move+Observation
		http://ponanza.hatenadiary.jp/entry/2014/04/02/220239

		もし　lower bound(stand pat score) がすでに>=betaであれば、
		stand pat score(fail-soft)かbeta(fail-hard)をlowerboundとしてreturn できる。
		（局面の評価値はstandpat以上になるはずなので）

		[
		というかそんなことが許されるのなら前向き枝切りは全部eval>=betaで済んでしまうじゃないか....??? 
		standpatは静止探索でのみ許されるのか？？そんなことはないはず。よくわからんもっと考えないと...
		しかし静止探索にstandpatを入れると前のバージョンにかなり勝ち越すようになった（よくわからん）
		 ]

		 [
		 静止探索ではコレぐらい適当にバッサリ枝を切ってしまってもいいのか...???
		 
		 ]


		fali soft
		http://d.hatena.ne.jp/hiyokoshogi/20111128/1322409710
		https://ja.wikipedia.org/wiki/Negascout
		通常のアルファベータ法が返す値は探索窓の範囲内の値であるが、 子ノードを探索した結果が探索窓の範囲外だった場合、
		探索窓の境界値ではなく実際に出現した子ノードの最大値を返すと探索量が減る事がある。
		これは親ノードに伝わったときにβ値以上となってカットしたりα値を更新して探索窓を狭めたりできる可能性が高くなるためである。
		このような性質を Fail-Soft と言い、 Null Window Search などの狭い探索窓による探索や置換表を使った探索をする場合にその効果がよく現れる。
		*/
		
	}
#endif
//	
//#ifdef LEARN
//	//学習時は精子探索深さ３までで止める。（こんなことはせずバッサリ枝を切れる枝切り方法を導入したい）
//	if (depth < -3 * ONE_PLY) { return bestvalue; }
//#endif

	//コレで上手く前の指し手の移動先を与えられていると思う
	//ここでnullmoveが入ってきた場合のことも考えないといけない。
	//というかnullmoveが入ってきたら取リ返すなんてありえないのでここで評価値返すしか無いでしょ(王手も生成するなら話は別）
#ifdef USETT
	movepicker mp(pos, move_to(pos.state()->lastmove),ttMove);
#else
	movepicker mp(pos, move_to(pos.state()->lastmove), MOVE_NONE);
#endif

	while ((move = mp.return_nextmove()) != MOVE_NONE) {
//
//#ifdef LEARN
//		if (is_ok(move) == false) { continue; }
//#endif

		if (pos.is_legal(move) == false) { continue; }
		/*if (pos.check_nihu(move) == true) {

			cout << "nihu " << endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
			cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}*/
		
		bool givescheck = pos.is_gives_check(move);

		//弱くなった
		//futility 
		if (!incheck
			&&!givescheck  //これ条件として入れるべきだと思うけれど今のdomoveの仕様ではなかなか難しい。
			&&futilitybase > -Value_known_win
			) {
			futilityvalue = futilitybase + Value(Eval::piece_value[pos.piece_on(move_to(move))]);
			//取り合いなので一手目をとった時にalpha-128を超えられないようであればその指し手を考えない
			if (futilityvalue <= alpha) {
				bestvalue = std::max(bestvalue, futilityvalue);
				continue;
			}
			if (futilitybase <= alpha && pos.see(move) <= Value_Zero)
			{
				bestvalue = std::max(bestvalue, futilitybase);
				continue;
			}
		}
#if 1
		// Detect non-capture evasions that are candidates to be pruned
		evasionPrunable = incheck
			&&  bestvalue > Value_mated_in_maxply
			&& !pos.capture(move);
		// Don't search moves with negative SEE values
		if ((!incheck || evasionPrunable)
			&& !is_promote(move)
			&& pos.see_sign(move) < Value_Zero) {
			continue;
		}
#endif


		//これ早くなるんだろうか？？
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif


		movecount++;
		//pos.do_move(move, &si);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif
		
		pos.do_move(move, &si, givescheck);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();
#ifndef LEARN
		if (signal.stop.load(std::memory_order_relaxed)) {
			//時間切れはfail hardにしているがここもfail softにしておくべきか？
			return alpha;
		}
#endif
		if (value > bestvalue) {
			//取り合いを読んだ分評価値はより正確になった
			bestvalue = value;

			//alpha値は底上げされた
			if (value > alpha) {
				
				if (PvNode) {
					update_pv(ss->pv, move, (ss + 1)->pv);
				}

				if (PvNode && value < beta) 
				{
					//ここでalpha値を超えの処理をする。
					alpha = value;
					bestMove = move;
				}
				else 
				{
					//PVnode以外ではnull window searchなのでalpha超えとはbeta超えのことである
#ifdef USETT
					//valueがbetaを超えたということはこの点数以上の指し手がまだあるかもしれないということなのでBOUND_LOWER
					tte->save(posKey, value_to_tt(value, ss->ply), BOUND_LOWER,
						depth, move, staticeval, TT.generation());
#endif
					return value;
				}


			}
		}
	}
	//王手をかけられていて合法手がなければそれは詰み
	if (incheck&&movecount == 0) {
		return mated_in_ply(ss->ply);
	}

#ifdef USETT
	//PVnode&&bestvalueがoldalphaを超えられなかったということはoldalphaは上限であり,bestvalueもまた上限である。
	//PVnode&&bestvalue>oldalphaということはコレは正確な評価値である。
	//PVnodeではないということはβ超えは起こらなかったnullwindowのアルファ値を超えられなかったつまりUPPERである
	tte->save(posKey, value_to_tt(bestvalue, ss->ply),
		PvNode && bestvalue > oldAlpha ? BOUND_EXACT : BOUND_UPPER,
		depth, bestMove, staticeval, TT.generation());
#endif

	return bestvalue;
}


// update_stats() updates killers, history, countermove and countermove plus
// follow-up move history when a new quiet best move is found.
/*
bonus = Value(d * d + 2 * d - 2);
bonusはdepthの２乗に比例する


d 0 d^2+2d-2 -2  depth==0は静止探索であるのでdepth=0 -2のあたいがつくことはない！
d 1 d^2+2d-2 1
d 2 d^2+2d-2 6
d 3 d^2+2d-2 13
d 4 d^2+2d-2 22
d 5 d^2+2d-2 33
d 6 d^2+2d-2 46
d 7 d^2+2d-2 61
d 8 d^2+2d-2 78
d 9 d^2+2d-2 97
d 10 d^2+2d-2 118
d 11 d^2+2d-2 141
d 12 d^2+2d-2 166
d 13 d^2+2d-2 193
d 14 d^2+2d-2 222
d 15 d^2+2d-2 253
d 16 d^2+2d-2 286
d 17 d^2+2d-2 321
d 18 d^2+2d-2 358

ここで細かい値の補正などは考えても仕方ない
bonusの値はすぐにどんどん変わっていく。

*/
void update_stats(const Position& pos, Stack* ss,const Move bestmove,
	Move* quiets,const int quietsCnt,const Value bonus) {

	if (ss->killers[0] != bestmove)
	{
		ss->killers[1] = ss->killers[0];
		ss->killers[0] = bestmove;
	}



	//bestmoveに+=正のbonus
	Color c = pos.sidetomove();
	Thread* thisthread = pos.searcher();
	thisthread->history.update(moved_piece(bestmove), move_to(bestmove), bonus);
	thisthread->fromTo.update(c, bestmove, bonus);
	update_cm_stats(ss, moved_piece(bestmove), move_to(bestmove), bonus);

	//countermoveとcurrentmoveの格納はまだ
	if ((ss - 1)->counterMoves)
	{
		Square prevSq = move_to((ss - 1)->currentMove);
		thisthread->counterMoves.update(pos.piece_on(prevSq), prevSq, bestmove);
	}

	// Decrease all the other played quiet moves
	//alphaを更新しなかった指し手に対して+=負の値
	for (int i = 0; i < quietsCnt; ++i) {
		thisthread->fromTo.update(c, quiets[i], -bonus);
		thisthread->history.update(moved_piece(quiets[i]), move_to(quiets[i]), -bonus);
		update_cm_stats(ss, moved_piece(quiets[i]), move_to(quiets[i]), -bonus);
	}

}

void update_cm_stats(Stack* ss, Piece pc, Square s, Value bonus) {

	CounterMoveStats* cmh = (ss - 1)->counterMoves;
	CounterMoveStats* fmh1 = (ss - 2)->counterMoves;
	CounterMoveStats* fmh2 = (ss - 4)->counterMoves;

	if (cmh)
		cmh->update(pc, s, bonus);

	if (fmh1)
		fmh1->update(pc, s, bonus);

	if (fmh2)
		fmh2->update(pc, s, bonus);
}


/*
Root,
PV,
NonPV,
*/
//
//template Value search<Root>(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);
//template Value search<PV>(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);
//template Value search<NonPV>(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);
//template <Nodetype NT>
//Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);