#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"

SearchLimit limit;
Signal signal;

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
	if (now_ - limit.endtime > 10) {
		signal.stop = true;
	}
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
	int maxdepth;



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
	while (++rootdepth <maxdepth) {

		//�����ŒT���֐����Ăяo���B
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY);

		sort_RootMove();

		if (signal.stop) {
			//cout << "signal stop" << endl;
			break;
		}

#ifndef LEARN
		print_pv(rootdepth, ss);
		cout <<" bestvalue"<< bestvalue << endl;
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
	//seldepth�̍X�V�������ōs��
	if (PVNode&&(thisthread->seldepth < pos.state()->ply_from_root+1)) {
		thisthread->seldepth = pos.state()->ply_from_root+1;
	}

	if (NT != Root) {
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

		alpha = std::max(mated_in_ply(pos.state()->ply_from_root), alpha);//alpha=max(-mate+ply,alpha)�@alpha�̒l�͌��݂܂���Ă���l�����������͐���Ȃ� �܂�alpha�͍ŏ��ł�-mate+ply
		beta = std::min(mate_in_ply(pos.state()->ply_from_root + 1), beta);//beta=min(mate-ply,beta)  beta�̒l�͎��̎w����ŋl�ޒl�����傫���͂Ȃ�Ȃ��@�܂�beta�͍ő�ł�mate-(ply+1)

		/*===================================================
		���݂�node�̐[����ply�Ƃ���
		���̃m�[�h��n��l�݂������Ă���ꍇalpha=mate-n
		n<ply+1�̏ꍇ��alpha>beta�ł�����return �ł���
		====================================================*/
		if (alpha >= beta) {
			return alpha;
		}

	}


	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

	//�]���֐��͖���Ăяo�����ق��������v�Z�ł���
	staticeval = Eval::eval(pos);

	//�����ɑO�����}�؂�̃R�[�h������

	

	movepicker mp(pos);

	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (pos.is_legal(move) == false) { continue; }
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

		if (NT == Root) {
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
		if (value > alpha) {
			alpha = value;

			if (PVNode&&NT != Root) {
				update_pv(ss->pv, move, (ss + 1)->pv);
			}

			if (alpha >= beta) {
				break;
			}
		}
	}//�w�����while

	if (movecount == 0) {
		return mated_in_ply(pos.state()->ply_from_root);
	}

	return alpha;

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
		movecount++;
		pos.do_move(move, &si);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();

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
		return mated_in_ply(pos.state()->ply_from_root);
	}

	return bestvalue;
}
