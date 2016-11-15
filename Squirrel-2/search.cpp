#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"

SearchLimit limit;

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

		//�����ŒT���֐����Ăяo���B
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY);

		sort_RootMove();

		print_pv(rootdepth, ss);
	}

	cout << "bestmove " << RootMoves[0] << endl;

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
	//if (incheck) {
	//	cout << "incheck" << endl;
	//}

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

//�Î~�T��
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


	//���q�T���͋�̎�荇�������扄�΂��ĒT���𑱂��邱�ƂŒT���̖��[�]���l��␳���A��萳�m�ɂ�����̂ł���Ǝv���Ă���B
	bestvalue=staticeval = Eval::eval(pos);

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
