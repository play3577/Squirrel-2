#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"
#include "tpt.h"
SearchLimit limit;
Signal signal;


/*
���̊֐��ŋl�݊֘A�̃X�R�A���uroot node���炠�Ɖ���ŋl�ނ��v����u���̋ǖʂ���㉽��ŋl�ނ��v�ɕϊ�������B
�Ȃ����Ƃ����ƒu���\����l�����o���Ƃ���rootnode����̎萔���Ⴄ�ǖʂ�����o����邱�Ƃ�����̂ŁA
root node���炠�Ɖ���ŋl�ނ��̏��ł͂��������Ȃ��Ă��܂����Ƃ����邩��ł���B
TT����ǂݏo���Ƃ���value_from_tt�����܂����Ƃł��̂������␳����B
*/
Value value_to_tt(Value v,int ply) {
	
	return v >= Value_mate_in_maxply ? v + ply :
		v <= Value_mated_in_maxply ? v - ply : v;
}

//���ꂨ��������������Ȃ�
Value value_from_tt(Value v, int ply) {

	return v >= Value_mate_in_maxply ? v - ply :
		v <= Value_mated_in_maxply ? v + ply : v;
}

void update_pv(Move* pv, Move move, Move* childPv) {

	//pv�̐擪�|�C���^��move�ɂ���
	//childpv��nullptr�łȂ��AMOVENONE�łȂ���
	//pv��childmove���i�[���Ă���
	//pv�͍��̋ǖʂ����pv
	for (*pv++ = move; childPv && *childPv != MOVE_NONE; )
		*pv++ = *childPv++;
	//pv�̍Ō��MOVENONE
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

	//���Ԑ���
	signal.stop = false;
	this->resetCalls = false;
	this->call_count = 0;
#ifndef  LEARN
	//���e�L�g�[�Ȏ��Ԑ���̃R�[�h���Ƃŉ��Ƃ�����
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

		//�����ŒT���֐����Ăяo���B
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY);

		sort_RootMove();

		if (signal.stop) {
			//cout << "signal stop" << endl;
			break;
		}

#ifndef LEARN
		print_pv(rootdepth,bestvalue);
		cout <<"�]���l"<< int(bestvalue)*100/int(Eval::PawnValue) << endl;
#endif
	}//end of �����[��
#ifndef LEARN
	cout << "bestmove " << RootMoves[0] << endl;
#endif // !LEARN

	return bestvalue;
}

template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	ASSERT(alpha < beta);

	//������
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
	//-----TT�֘A
	//�ǂݑ��ł�TT����ǂݍ���ł���r����TT���j�󂳂ꂽ�ꍇ�̂��Ƃ��l���Ă����̂ŁA������Q�l�ɂ���
	//Key poskey;
	bool TThit;
	TPTEntry* tte;
	Move ttMove;
	Value ttValue;
	Value ttEval;
	Bound ttBound;
	Depth ttdepth;

#ifndef LEARN
	//timer thread��p�ӂ����ɂ����Ŏ��Ԃ��m�F����B
	//stockfish����
	if (thisthread->resetCalls.load(std::memory_order_relaxed)) {

		thisthread->resetCalls = false;
		thisthread->call_count = 0;

	}
	//�Ȃ��Ȃ����Ԓʂ�Ɏw���Ă���Ȃ������̂�2048�ōs��
	if (++thisthread->call_count > 4096) {
		//���񉻂̂Ƃ��͂��ׂẴX���b�h�ł��̑�����s���B
		thisthread->resetCalls = true;
		check_time();
	}
#endif
	//seldepth�̍X�V�������ōs��
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
		���Ƃ����̎w����ő�����l�܂��邱�Ƃ��o���ĕ]���l��mate-(ply+1)�ɂȂ����Ƃ��Ă�
		�������̃Q�[���؂̏�ł��Z���萔�ł̋l�݂��������Ă���alpha�l�����łɂ��̒l�imate-(ply+1)�j�����傫���Ȃ��Ă����
		���݂̃A���t�@�l�𒴂��邱�Ƃ͌����ĂȂ��̂ŃR���ȏ�T��������K�v�͂Ȃ�

		�����_���i�����𔽓]�����āj�ŋl�݂̑���ɂ܂����ꍇ���l���邱�Ƃ��ł���B

		���̏ꍇfail-high�̒l��Ԃ��B

		�i�v��j�S��l�߂��������Ă���ꍇ�ɂT��l�߂�T���K�v�͂Ȃ�
		===============================================================================================================================*/

		//=====================================================================================================================
		//state->plyfrom_root�ł͑ʖځI�Istartpos����̎萔�ɂȂ��Ă��܂��I�I�I�iStack��root����̎萔���i�[���邵���������j
		//=====================================================================================================================
		alpha = std::max(mated_in_ply(ss->ply), alpha);//alpha=max(-mate+ply,alpha)�@alpha�̒l�͌��݂܂���Ă���l�����������͐���Ȃ� �܂�alpha�͍ŏ��ł�-mate+ply
		beta = std::min(mate_in_ply(ss->ply + 1), beta);//beta=min(mate-ply,beta)  beta�̒l�͎��̎w����ŋl�ޒl�����傫���͂Ȃ�Ȃ��@�܂�beta�͍ő�ł�mate-(ply+1)

		/*===================================================
		���݂�node�̐[����ply�Ƃ���
		���̃m�[�h��n��l�݂������Ă���ꍇalpha=mate-n
		n<ply+1�̏ꍇ��alpha>beta�ł�����return �ł���
		====================================================*/
		if (alpha >= beta) {
			return alpha;
		}

	}
	// Step 4. Transposition table lookup. We don't want the score of a partial
	// search to overwrite a previous full search TT value, so we use a different
	// position key in case of an excluded move.
	/*
	���O����肪�Ȃ��T���̒u���\���㏑�����邽�߂ɓ���̎����O���Ă̒T�����ʂ͂ق����Ȃ��̂ŁA
	exclude move������ꍇ�ɂ͈قȂ�hashkey��p����
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
	//�A�N�Z�X�������|���̂Ő�ɑS���ǂݏo���Ă����B
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
	//�����ł���key32�̒l���ς���Ă��܂��Ă����ꍇ��tt�̒l�������������Ă��܂��Ă���̂Œl���Ȃ��������Ƃɂ���iidea from �ǂݑ��j(���̂Ƃ���one thread�Ȃ̂ł��܂�Ӗ��͂Ȃ�)
	if (TThit && (poskey >> 32) != tte->key()) {
		
		cout << "Access Conflict" << endl;
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		TThit = false;
	}

	// At non-PV nodes we check for an early TT cutoff
	//non pv node�Œu���\�̒l�Ŏ}�؂�o���Ȃ��������B
	/*
	PV�m�[�h�ł͂Ȃ�
	�u���\�̎c��T���[���̂ق����傫���idepth�͎c��[���ł���̂Ő[���T���ɂ�錋�ʂł���Ƃ������ƁH�j
	ttvalue��ValueNone�łȂ�

	ttvalue=>beta�̂Ƃ���BOUND_LOWER|BOUNDEXACT�ł���ΐ^�̒l��beta�ȏ�ł���Ƃ݂Ȃ���̂Ŏ}�؂�ł���B
	ttvalue<beta�̂Ƃ���BOUND_UPPER|BOUNDEXACT�ł���ΐ^�̒l��beta�ȉ��ł��邱�Ƃ��m�肵�Ă���
	�inonPVnode�ł���̂ł����nullwindowsearch�ł���alpha�ȉ��ł��邱�Ƃ��m�肵���̂ł����Œl��Ԃ��Ă��ǂ��j
	*/
	if (!PVNode
		&&TThit
		&&ttdepth>=depth
		&&ttValue != Value_error//���ꍡ�̂Ƃ��낢��Ȃ�(���������Əڂ����ǂޕK�v������)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWER�ł���̂łǂ����&��������
		) {
		return ttValue;
	}
#endif
	

	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

	//�]���֐��͖���Ăяo�����ق��������v�Z�ł���
	staticeval = Eval::eval(pos);

	//�����ɑO�����}�؂�̃R�[�h������

	

	movepicker mp(pos);

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }

		//����������Ă��Ȃ����Ƃ͊m�F����
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

		//���Ԑ؂�Ȃ̂�bestmove��PV�������Ȃ������ɒl��Ԃ��B
		if (signal.stop.load(std::memory_order_relaxed)) {
			return Value_Zero;
		}

		if (RootNode) {
			ExtMove* rm = thisthread->find_rootmove(move);
			if (movecount == 1 || value > alpha) {
				rm->value = value;//�w����̃X�R�A�����O
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

		//alpha�����̏���
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
				//PVnode�łȂ���΂Ȃ�Ȃ��̂�nonPV�ł�nullwindowsearch������
				if (PVNode&&value < beta) {
					alpha = value;
				}
				else {
					//�����PV�ł�nonPV�ł����藧�inull window�j
					ASSERT(value >= beta);
					break;
				}
			}
		}

	}//�w�����while

	
	if (movecount == 0) {
		bestvalue= excludedmove?alpha: mated_in_ply(ss->ply);
	}

#ifdef USETT
	//http://yaneuraou.yaneu.com/2015/02/24/stockfish-dd-search-%E6%8E%A2%E7%B4%A2%E9%83%A8/ �@������p
	// value_to_tt�͋l�݂̃X�R�A�̏ꍇ�A���݂̋ǖʂ���̎萔��\�����l�ɕϊ����邽�߂̃w���p�[�֐��B
	// beta cut���Ă���Ȃ�BOUND_LOWER(���ɂ����Ƃ����l���L�^����w���肪���邩���m��Ȃ��̂�)
	// PvNode�ł���bestMove����̓I�Ȏw����Ƃ��đ��݂���Ȃ�ABOUND_EXACT
	// non PV node�Ƃ��AbestMove���Ȃ�(�K���Ȍ��ς���Ŏ}���肵���ꍇ�Ȃ�)�́A���̏��

	// non PV���ƁAnull window�ŒT������̂ŁA���ɂ��ꂪ[��-1,��]�͈̔͂ł���΁A
	// ���𒴂��邩�����Ȃ��������킩��Ȃ��B���������l���Ԃ��Ă����Ȃ�A����node�̐^�̒l�́A
	// ���̒l�ȉ��ł��邩��ABOUND_UPPER�A�܂��A��������l���Ԃ��Ă�����A����node�̐^�̒l�́A
	// ���̒l�ȏ�ł��낤����ABOUND_LOWER�ł���B
	// ���̂悤�ɁAnon PV�ɂ����ẮABOUND_UPPER�ABOUND_LOWER�������݂��Ȃ��B(aki.����)
	tte->save(poskey, value_to_tt(bestvalue, ss->ply),
		bestvalue >= beta ? BOUND_LOWER :
		PVNode&&bestMove ? BOUND_EXACT : BOUND_UPPER,
		depth, bestMove, staticeval, TT.generation());
#endif

	return bestvalue;


}

//�Î~�T��(�������Ȃ莞�Ԃ�H�����߂ł��邾���}�؂肵����)
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


	//���Ԓʂ�Ɏw������w���Ă���Ȃ������̂Ő��q�T���ł����Ԃ�����
#ifndef LEARN
	//timer thread��p�ӂ����ɂ����Ŏ��Ԃ��m�F����B
	//stockfish����
	if (thisthread->resetCalls.load(std::memory_order_relaxed)) {

		thisthread->resetCalls = false;
		thisthread->call_count = 0;

	}
	
	if (++thisthread->call_count > 4096) {
		//���񉻂̂Ƃ��͂��ׂẴX���b�h�ł��̑�����s���B
		thisthread->resetCalls = true;
		check_time();
	}
#endif
	
	//�����ɑO�����}�؂�̃R�[�h������


	//�Î~�T���͋�̎�荇���̂悤�ȕ]���l���s����ȋǖʂ�扄�΂��ĒT���𑱂��邱�ƂŒT���̖��[�]���l��␳���A��萳�m�ɂ�����̂ł���Ǝv���Ă���B
	bestvalue=staticeval = Eval::eval(pos);

	
#ifdef LEARN
	//�w�K���͐��q�T���[���R�܂łŎ~�߂�B�i����Ȃ��Ƃ͂����o�b�T���}��؂��}�؂���@�𓱓��������j
	if (depth < -3 * ONE_PLY) { return bestvalue; }
#endif
	//�R���ŏ�肭�O�̎w����̈ړ����^�����Ă���Ǝv��
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
			//��荇����ǂ񂾕��]���l�͂�萳�m�ɂȂ���
			bestvalue = value;

			//alpha�l�͒�グ���ꂽ
			if (value > alpha) {
				alpha = value;

				//beta cut!
				if (alpha >= beta) {
					return alpha;
				}
			}
		}
	}
	//������������Ă��č��@�肪�Ȃ���΂���͋l��
	if (incheck&&movecount == 0) {
		return mated_in_ply(ss->ply);
	}

	return bestvalue;
}
