#include "search.h"
#include "movepicker.h"
#include "Thread.h"
#include "evaluate.h"
#include "tpt.h"
#include "book.h"
#include <random>
#include "TimeManeger.h"


#if defined(_MSC_VER)
#endif
#if defined(__GNUC__) 
#include <algorithm>
#endif

#ifdef USETT
#define Probcut
#endif

//aspiration�T��
#ifndef LEARN
#define ASP
#endif

#define MATEONE
//#define MATETEST

#ifdef MATETEST
#include "makemove.h"
#endif
//#define PREF2
SearchLimit limit;
Signal signal;
TimeManeger TimeMan;

typedef std::vector<int> Row;
/*
[�X���b�hid][�[��]�B
razy smp�Ŕ����i���̒T�����P�X�L�b�v����Ƃ��Ɏg���B

�l�̌��ߕ��͂悭�킩���......
*/
const Row HalfDensity[] = {
	{ 0, 1 },
	{ 1, 0 },
	{ 0, 0, 1, 1 },
	{ 0, 1, 1, 0 },
	{ 1, 1, 0, 0 },
	{ 1, 0, 0, 1 },
	{ 0, 0, 0, 1, 1, 1 },
	{ 0, 0, 1, 1, 1, 0 },
	{ 0, 1, 1, 1, 0, 0 },
	{ 1, 1, 1, 0, 0, 0 },
	{ 1, 1, 0, 0, 0, 1 },
	{ 1, 0, 0, 0, 1, 1 },
	{ 0, 0, 0, 0, 1, 1, 1, 1 },
	{ 0, 0, 0, 1, 1, 1, 1, 0 },
	{ 0, 0, 1, 1, 1, 1, 0 ,0 },
	{ 0, 1, 1, 1, 1, 0, 0 ,0 },
	{ 1, 1, 1, 1, 0, 0, 0 ,0 },
	{ 1, 1, 1, 0, 0, 0, 0 ,1 },
	{ 1, 1, 0, 0, 0, 0, 1 ,1 },
	{ 1, 0, 0, 0, 0, 1, 1 ,1 },
};

const size_t HalfDensitySize = std::extent<decltype(HalfDensity)>::value;

inline Value bonus(const Depth d1) { int d = d1 / ONE_PLY; return Value(d*d + 2 * d - 2); }
void update_cm_stats(Stack* ss, Piece pc, Square s, Value bonus);
void update_stats(const Position& pos, Stack* ss, Move move, Move* quiets, int quietsCnt, Value bonus);


template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);

#ifdef LEARN
//�w�K�p�̎}�؂��}�����T���֐�

template <Nodetype NT>Value lsearch(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

template <Nodetype NT>
Value lqsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);

#endif


/*
// EasyMoveManager structure is used to detect an 'easy move'. When the PV is
// stable across multiple search iterations, we can quickly return the best move.
�����肪�ł܂����炻��ȏ�͒T�������ɍ������Ԃ��Ă��܂����߂ɕK�v�ƂȂ�B
*/
struct EasyMoveManager {

	int stableCnt;
	Key expectedPosKey;
	Move pv[3];

	void clear() {
		stableCnt = 0;
		expectedPosKey = 0;
		pv[0] = pv[1] = pv[2] = MOVE_NONE;
	}

	Move get(Key key)const {
		return expectedPosKey == key ? pv[2] : MOVE_NONE;
	}

	void update(Position &pos, const std::vector<Move>& newPv) {
		ASSERT(newPv.size() >= 3);
		
		//pv[2]������ł���񐔂��L�^����
		//�������Ȃ��pv[0]����v���Ă�̂����Ȃ��񂾁H����pv[0]����@��ł����C������
		stableCnt = (newPv[2] == pv[2]) ? stableCnt + 1 : 0;

		//newPv�O�`�R��PV�ƈ�v���Ă��Ȃ���ΐV����pv��expectedposkey���i�[
		if (!std::equal(newPv.begin(), newPv.begin() + 3, pv))
		{
			std::copy(newPv.begin(), newPv.begin() + 3, pv);

			StateInfo st[2];
			pos.do_move(newPv[0], &st[0]);
			pos.do_move(newPv[1], &st[1]);
			expectedPosKey = pos.key();
			pos.undo_move();
			pos.undo_move();
		}
	}

};

EasyMoveManager EasyMove;

//d��Ŕ҉�ł���]���l�̗\��(d�͍ő�ł�6)
//�U���܂ł��̊֐��ŗ\���ł���Ƃ����̂͂ǂ��Ȃ񂾂낤��...���������󂢂ق��������C������...
//��͐i�s�x�ɂ���Ă��̒l�������邱�Ƃ��ł���C������...
//���ՂP�S�O�I��160�݂�����....
/*
�i�s�x�ɂ����margin�l��ς��邽�߂ɂ͌��w�̂悤�Ɋw�K����margin��10+258*progress�݂����ɐi�s�x���傫���Ȃ�ɘA��đ傫�����邱�Ƃ��K�v���H�H
�i�s�x���傫���Ȃ�ɘA���sigmoid�̌X�΂����݂���͈͂��L������̂��㔼�̊����ȊO�̎w�����PP�l�����t��̂���Ƒ傫�������悤�ɂ��邽�߂ɂ��肩������Ȃ�...
*/
Value futility_margin(Depth d) { ASSERT(d < 7*ONE_PLY); return Value(150 * d / ONE_PLY); }

//d��Ŕ҉�ł���_���ł���̂ł��񂾂�Ƒ傫���Ȃ�Ȃ��Ƃ�������.(�Ǐ��œK���Ɋׂ��Ă���H�iby��������j)
const int razor_margin[4] = { 483, 570, 603, 554 };


//�T���p�̒萔
/*
root��3�ɂ����ق����������̐ߖ�ɂȂ��Ă������H�H����ς��߂���
*/
int FutilityMoveCounts[2][16]; // [improving][depth]
//����͏������ق����������ɂ�
int Reductions[2][2][64][64];  // [(bool)isPV][improving][depth][moveNumber]

template <bool isPvNode> Depth reduction(bool i, Depth d, int mn) {
	return Reductions[isPvNode][i][std::min(int(d / ONE_PLY), 63)][std::min(mn, 63)] * ONE_PLY;
}

//������Ń}���`�X���b�h�悤�ɕς���
void search_clear(Thread& th) {

#ifndef LEARN
	TT.clear();
#endif
	th.CounterMoveHistory.clear();

	
	th.fromTo.clear();
	th.history.clear();
	th.counterMoves.clear();

}

//�T���搔�̏�����
//�l�������Ɛ^���Ɍ��Ă���
//�l�͏������ق����}�؂肪�ɂ�
void search_init() {

	for (int imp = 0; imp <= 1; ++imp) {
		for (int d = 1; d < 64; ++d) {
			for (int mc = 1; mc < 64; ++mc)
			{
				//double r = log(d) * log(mc) / 2.0;
				////double r = log(d) * log(mc) / 2.5;//�����ɂ����Ă݂�B
				//if (r < 0.80) {
				//	continue;
				//}

				double r = log(d) * log(mc) / 1.95;


				//0 1�@�� is_pv
				Reductions[0][imp][d][mc] = int(std::round(r));
				Reductions[1][imp][d][mc] = std::max(Reductions[0][imp][d][mc] - 1, 0);

				// Increase reduction for non-PV nodes when eval is not improving
				if (!imp && Reductions[0][imp][d][mc] >= 2) {
					Reductions[0][imp][d][mc]++;
				}
			}
		}
	}
	for (int d = 0; d < 16; ++d)
	{
		FutilityMoveCounts[0][d] = int(2.4 + 0.773 * pow(d + 0.00, 1.8));
		FutilityMoveCounts[1][d] = int(2.9 + 1.045 * pow(d + 0.49, 1.8));
		//�������ɂ����Ă݂�
		//���ꂪ�傫���ق����������ɂ�
		//FutilityMoveCounts[0][d] = int(3.4 + 0.773 * pow(d + 0.00, 1.8));
		//FutilityMoveCounts[1][d] = int(3.9 + 1.045 * pow(d + 0.49, 1.8));
	}

}


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
	if (limit.is_inponder) { return; }
	TimePoint now_ = now();
	if (now_  > limit.endtime -20) {
		signal.stop = true;
	}
}


#ifndef LEARN
//---------------------------�T��������
Value Thread::think() {

	MainThread* mainThread = (this == Threadpool.main() ? Threadpool.main() : nullptr);

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	std::memset(ss - 5, 0, 8 * sizeof(Stack));//�܂�8��������������H�H
	Value bestvalue, alpha, beta, delta;
	Move easyMove = MOVE_NONE;
	pv.clear();

	bool findbook = false;

	if (mainThread) {
		easyMove = EasyMove.get(rootpos.key());
		EasyMove.clear();
		TT.new_search();
		mainThread->easyMovePlayed = mainThread->failedLow = false;
		mainThread->bestMoveChanges = 0;
	}


	bestvalue = delta = alpha = -Value_Infinite;
	beta = Value_Infinite;
	rootdepth = 0;
	int maxdepth;

	seldepth = 0;

	maxdepth = MAX_DEPTH;


	//	Eval::eval(rootpos);



	while (++rootdepth <maxdepth && !signal.stop) {

		if (!mainThread)
		{
			const Row& row = HalfDensity[(idx - 1) % HalfDensitySize];
			if (row[(rootdepth + rootpos.ply_from_startpos) % row.size()])
				continue;
		}

		if (mainThread) { mainThread->bestMoveChanges *= 0.505, mainThread->failedLow = false; }

		previousScore = RootMoves[0].value;
#ifdef ASP
		/*
		<1:info depth 1/2 score cp 2808 time 1 nodes 340 nps 340k pv  G*7f
		<1:info depth 2/4 score cp mate 4 time 1 nodes 562 nps 562k pv  6h8h 9g8h 7g7h+
		<1:alpha:0 beta:0 previous value:31996 delta:-39996
		<1:Error!!
		<1:info string file:search.cpp line:292 alpha < beta

		���̃o�O��rootdepth=3�Ŕ������Ă���I�I�I�I�I�I
		rootdepth>=5���甽���i���Ȃ̂ł���͔����i���̖��ł͂Ȃ��I�ق��̂Ƃ���Ɍ���������͂��I�I�I

		rootdepth<5�̎��� bestvalue��valueInfinity�ɂȂ��Ă��܂��Ă����I�H�H�H
		�ǂ��ł���Ȓl���A���Ă��Ă��܂��Ă���̂����ׂ�

		*/
		if (rootdepth >= 5) {
			delta = Value(40);
			alpha = std::max(previousScore - delta, -Value_Infinite);
			beta = std::min(previousScore + delta, Value_Infinite);
		}
		
#endif
	research:
		//�����ŒT���֐����Ăяo���B
		if (alpha >= beta) {
			cout << "alpha:" << alpha << " beta:" << beta << " previous value:" << previousScore << " bestvalue:" << bestvalue << " delta:" << delta << endl;
		}
		ASSERT(alpha < beta);



#ifndef	LEARN
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY, false);
#else
		bestvalue = lsearch<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY, false);
#endif

		ASSERT(abs(bestvalue) < Value_Infinite);

		sort_RootMove();

		/*
		mate�𔭌�������63�񃋁[�v����낤�Ƃ��Đ������Ԃ𒴂��Ă��܂����Ƃ��N���肤��
		�i���̑O�̑��ł���ŕ����Ă��܂����B�j
		�̂ł�����mate���������炷���ɕԂ������ɂ���

		��������mate��������Ȃ��̂�20�܂ł͒T��������

		*/
		//������signal stop��true�ɂ��Ă����ׂ��H�H
		if (rootdepth > 20 && abs(bestvalue) > Value_mate_in_maxply) { 
			
			if (limit.is_inponder) { signal.stopOnPonderHit = true; }
			else { signal.stop = true; }
			goto ID_END;
		}


		if (signal.stop) {
			//cout << "signal stop" << endl;
			
			break;
		}
#ifdef ASP
		if (bestvalue <= alpha)
		{
			beta = (alpha + beta) / 2;
			alpha = std::max(bestvalue - delta, -Value_Infinite);
			delta += delta / 4 + 5;

			if (mainThread) {
				mainThread->failedLow = true;//���l�������Ƃ͂��̃m�[�h�ɂ͍őP��͂Ȃ��Ǝv���Ă���Ƃ�������
				signal.stopOnPonderHit = false;//���l��������Ă���Ƃ���stoponponderhit�����Ă��܂��̂͂�΂�
			}


			ASSERT(alpha >= -Value_Infinite&&beta <= Value_Infinite);
			/*if (delta <= 0) {
				cout << alpha << " " << beta << " " << delta << " " << bestvalue << endl;
				ASSERT(0);
			}*/
			goto research;
		}
		else if (bestvalue >= beta)
		{
			alpha = (alpha + beta) / 2;
			beta = std::min(bestvalue + delta, Value_Infinite);
			delta += delta / 4 + 5;


			ASSERT(alpha >= -Value_Infinite&&beta <= Value_Infinite);
			/*if (delta <= 0) {
				cout << alpha << " " << beta << " " << delta << " " << bestvalue << endl;
				ASSERT(0);
			}*/


			goto research;
		}
#endif

		if (!signal.stop) completedDepth = rootdepth;


		if (!mainThread) { continue; }


		

#if !defined(LEARN) || !defined(MAKEBOOK)
		if (idx == 0) { print_pv(rootdepth, bestvalue); }

#endif
		//�T���𑱂���ׂ�������Ƃ���߂Ă��܂��Ă����̂�
		if ((bool)Options["use_defined_time"] == false) {
			if (!signal.stop && !signal.stopOnPonderHit)
			{
				// Stop the search if only one legal move is available, or if all
				// of the available time has been used, or if we matched an easyMove
				// from the previous search and just did a fast verification.
				/*
				���@�肪1�����Ȃ������A�g���鎞�Ԃ��g���؂��Ă��܂����A
				�ȑO�̒T����easymove�ƍ��v����
				�����̏ꍇ�͒T�����I������
				*/
				const int F[] = { mainThread->failedLow,
					bestvalue - mainThread->previousScore };
				//failed low���N�������񐔂��傫���Abestvalue-previousscore�����ɑ傫���ق�improvefacotr�͑傫���Ȃ�B
				//����͑傫���Ȃ�ׂ��ł͂Ȃ��l�ł���
				int improvingFactor = std::max(229, std::min(715, 357 + 119 * F[0] - 6 * F[1]));
				double unstablePvFactor = 1 + mainThread->bestMoveChanges;//�����pv���s����Ƃ����Ӗ��Ȃ̂ő傫���Ȃ�ׂ��l�łȂ�

				bool doEasyMove = (pv[0] == easyMove)&&(mainThread->bestMoveChanges<0.03)&&(TimeMan.elasped()>TimeMan.optimum() * 5 / 44);//5/44�͏���������Ǝv���񂾂�Ȃ�...

				//�T�����I�����Ă��܂��Ă�������
				if (end-RootMoves==1
					|| TimeMan.elasped()>TimeMan.optimum()*unstablePvFactor*improvingFactor/628
					|| (mainThread->easyMovePlayed = doEasyMove, doEasyMove)) {

					if (limit.is_inponder) { signal.stopOnPonderHit = true; }
					else { signal.stop = true; }

				}


			}
		}//�T�����I�����邩�ǂ���

		if (pv.size() >= 3) {
			EasyMove.update(rootpos, pv);
		}
		else {
			EasyMove.clear();
		}


	}//end of �����[��
	sort_RootMove();
ID_END:

	previousScore = RootMoves[0].value;

	if (!mainThread)
		return bestvalue;
	//�������牺��mainthread�݂̂̑���

	// Clear any candidate easy move that wasn't stable for the last search
	// iterations; the second condition prevents consecutive fast moves.
	if (EasyMove.stableCnt < 6 || mainThread->easyMovePlayed) { EasyMove.clear(); }


	return bestvalue;

}


//===========
//�T���̂Ƃ�܂Ƃߊ֐�
//===========
Value MainThread::think() {

	Move pondermove=MOVE_NONE;

	bool findbook = false;
	pv.clear();
	Thread* bestthread = this;


	//�l��ł�Ƃ��͉������Ȃ��ŋA��
	if (end == RootMoves/*&& limit.is_inponder == false*/) {
#ifndef LEARN
		if (!signal.stop&&limit.is_inponder) {
			signal.stopOnPonderHit = true;
			wait(signal.stop);
		}
		cout << "bestmove resign" << endl;
#endif // !LEARN
		return Value_Mated;
	}
	//���ʐ錾����
	if (rootpos.ply_from_startpos>200&&limit.is_inponder == false&&rootpos.is_nyugyoku() ) {
		cout << "bestmove win" << endl;
		return Value_Mate;
	}

	//������book��T���R�[�h������
	if (Options["usebook"] == true && limit.is_inponder == false) {

		const string sfen = rootpos.make_sfen();
		auto bookentry = book.find(sfen);

		if (bookentry != book.end()) {
			findbook = true;
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


			cout << "info book move" << endl;
			goto S_END;

		}//book���������ꍇ

	}//end book

	//===============
	//�T���J�n
	//==============
	signal.stop = false;
	this->resetCalls = false;
	this->call_count = 0;
#ifndef  LEARN
	if ((bool)Options["use_defined_time"] == true) {
		limit.endtime = Options["defined_time"] + limit.starttime;
	}
	else{
		TimeMan.init(limit, rootpos.sidetomove(), rootpos.ply_from_startpos);
		if (TimeMan.maximum()< limit.byoyomi) {
			limit.endtime = limit.byoyomi + limit.starttime;
		}
		else {
			limit.endtime = TimeMan.maximum() + limit.starttime;
		}
	}
	
#endif // ! LEARN


	for (Thread* th : Threadpool) {
		if (th != this) { th->start_searching();}
	}
	Thread::think(); // Let's start searching!


	//ponder���ɋl�݂��������ꍇ�ɍ������Ԃ��Ă��܂�Ȃ��悤�ɂ����ő҂�����
	if (!signal.stop&&limit.is_inponder) {
		signal.stopOnPonderHit = true;
		wait(signal.stop);
	}


	signal.stop = true;
	for (Thread* th : Threadpool) {
		if (th != this) {
			th->wait_for_search_finished();
		}
	}


	
	//bestThread�܂�I��
	if (pv[0] != MOVE_NONE) {

		for (Thread* th : Threadpool) {
			if (th->completedDepth > bestthread->completedDepth
				&&th->RootMoves[0].value > bestthread->RootMoves[0].value) {
				bestthread = th;
			}
		}

	}



	bestthread->print_pv(bestthread->completedDepth, bestthread->RootMoves[0].value);


	S_END:;
	cout << "bestmove " << bestthread->RootMoves[0];
	
	if (Options["USI_Ponder"] == true) {
		if (findbook == false) {
			if (pv.size() > 1 || extract_ponder_from_tt(rootpos)) { pondermove = pv[1]; }
		}
		if (pondermove != MOVE_NONE&&is_ok(pondermove)) {
			cout << " ponder " << pondermove;
		}

	}

	cout << endl;
	cout << "node all  " << Threadpool.nodes_searched() << endl;;
	return Value(0);
}

#else


Value Thread::think() {

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	std::memset(ss - 5, 0, 8 * sizeof(Stack));//�܂�8��������������H�H
	Value bestvalue, alpha, beta, delta;
	pv.clear();

	bool findbook = false;

	if (end == RootMoves) {
#ifndef LEARN
		cout << "bestmove resign" << endl;
#endif // !LEARN
		return Value_Mated;
	}

	Move pondermove;



	bestvalue = delta = alpha = -Value_Infinite;
	beta = Value_Infinite;
	rootdepth = 0;
	int maxdepth;

	seldepth = 0;

	//���Ԑ���
	signal.stop = false;
	this->resetCalls = false;
	this->call_count = 0;

	//cout << limit.endtime << endl;
#if defined(LEARN) || defined(MAKEBOOK)
	maxdepth = l_depth;//���̒l-1�����ۂɒT�������[��
	alpha = this->l_alpha;
	beta = this->l_beta;

#else
	maxdepth = MAX_DEPTH;
#endif // !LEARN


	//	Eval::eval(rootpos);



	while (++rootdepth <maxdepth && !signal.stop) {

		previousScore = RootMoves[0].value;

	research:
		//�����ŒT���֐����Ăяo���B
		if (alpha >= beta) {
			cout << "alpha:" << alpha << " beta:" << beta << " previous value:" << previousScore << " bestvalue:" << bestvalue << " delta:" << delta << endl;
		}
		ASSERT(alpha < beta);



#if  defined(MAKETEACHER) || defined(MAKESTARTPOS)
		bestvalue = search<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY, false);
#else
		bestvalue = lsearch<Root>(rootpos, ss, alpha, beta, rootdepth*ONE_PLY, false);
#endif


		ASSERT(abs(bestvalue) < Value_Infinite);

		sort_RootMove();

		/*
		mate�𔭌�������63�񃋁[�v����낤�Ƃ��Đ������Ԃ𒴂��Ă��܂����Ƃ��N���肤��
		�i���̑O�̑��ł���ŕ����Ă��܂����B�j
		�̂ł�����mate���������炷���ɕԂ������ɂ���

		��������mate��������Ȃ��̂�20�܂ł͒T��������

		*/
		if (rootdepth > 20 && abs(bestvalue) > Value_mate_in_maxply) { goto ID_END; }


		if (signal.stop) {
			//cout << "signal stop" << endl;
			break;
		}

	}//end of �����[��
	sort_RootMove();
ID_END:

	previousScore = RootMoves[0].value;

	//�w�K���͎��Ԃ�return 0�������肷�邱�Ƃ͂Ȃ��̂�bestmove�ɂ͂����ƒl�������Ă���͂�
	return bestvalue;
}

#endif




template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth,bool cutNode) {

	ASSERT(-Value_Infinite<=alpha&&alpha < beta&&beta<=Value_Infinite);
	


	//������
	const bool PVNode = (NT == PV || NT == Root);
	const bool RootNode = (NT == Root);

	ASSERT(PVNode || (alpha == beta - 1));
	ASSERT(DEPTH_ZERO < depth&&depth < MAX_DEPTH*(int)ONE_PLY);
	ASSERT(!(PVNode&&cutNode));



	bool doFullDepthSearch = false;



	Move pv[MAX_PLY + 1];
	Move move = MOVE_NONE;
	Move bestMove=MOVE_NONE;
	Move excludedmove=MOVE_NONE;
	Move Quiets_Moves[64];
	int quiets_count=0;
	Value value,null_value;
	StateInfo si;
	si.clear_stPP();
	Value staticeval=Value_Zero;
	int movecount=ss->moveCount = 0;
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
	//Value ttEval;
	Bound ttBound;
	Depth ttdepth;
	bool CaptureorPropawn;
	bool givescheck, improve, singler_extension, move_count_pruning;
	Depth extension, newdepth;

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
	if (PVNode&&(thisthread->seldepth < ss->ply)) {
		thisthread->seldepth = ss->ply;
	}

	if (!RootNode) {
		//step2
		if (signal.stop.load(std::memory_order_relaxed)||ss->ply>=(MAX_PLY-3)) {
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

	//����128��܂œǂ�ł��܂��Ȃ�ĐM�����Ȃ��̂���...�����������ʂ��o����ɕύX�_�͂��������Ȃ��̂ŐM���邵���Ȃ���
	//�ǂ������extention�̂��߂����������悤...??
	ASSERT(0<=ss->ply&&ss->ply<MAX_PLY)

	//ss�͉ߋ��̏��������Ă����K�v������
	ss->currentMove = (ss + 1)->excludedMove=bestMove = MOVE_NONE;
	ss->counterMoves = nullptr;
	(ss + 1)->skip_early_prunning = false;
	(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;

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
	//ASSERT((poskey & uint64_t(1)) == pos.sidetomove());
	//cout << (poskey>>32) << endl;
	tte = TT.probe(poskey, TThit);
	/*if (TThit) {
		cout << "TThit" << endl;
	}*/
	//�A�N�Z�X�������|���̂Ő�ɑS���ǂݏo���Ă����B
	if (TThit) {
		ttValue = value_from_tt(tte->value(),ss->ply);
		ttdepth = tte->depth();
	//	ttEval = tte->eval();
		ttBound = tte->bound();
		//ttMove = tte->move();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
	//	ttEval = Value_error;
		ttBound = BOUND_NONE;
		//ttMove = MOVE_NONE;
	}
	//Root�̂Ƃ��͈Ⴄ���������Ȃ���΂Ȃ�Ȃ�
	ttMove = RootNode ? thisthread->RootMoves[0].move : TThit ? tte->move() : MOVE_NONE;



	//���̏���������ƃN�b�\�x���Ȃ��Ă��܂�(�L�_�`)�@�܂��r����TT�̒l������������Ă��܂��Ƃ������Ƃɂ��Ă͍l���Ȃ��ق��������̂�������Ȃ�
	//�����ł���key32�̒l���ς���Ă��܂��Ă����ꍇ��tt�̒l�������������Ă��܂��Ă���̂Œl���Ȃ��������Ƃɂ���iidea from �ǂݑ��j(���̂Ƃ���one thread�Ȃ̂ł��܂�Ӗ��͂Ȃ�)
	/*if (TThit && (poskey >> 32) != tte->key()) {

		cout << "Access Conflict" << endl;
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
		ttBound = BOUND_NONE;
		TThit = false;
	}*/

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
		&&ttdepth >= depth
		&&ttValue != Value_error//���ꍡ�̂Ƃ��낢��Ȃ�(���������Əڂ����ǂޕK�v������)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWER�ł���̂łǂ����&��������
		//���̋ǖʂ�ttmove���񍇖@�肾������Ȃ��Ȃǂ����ق��������̂ł́H�H
		) {

		if (ttMove != MOVE_NONE) {

			if (ttValue >= beta) {
				if (!pos.capture_or_propawn(ttMove)) { update_stats(pos, ss, ttMove, nullptr, 0, bonus(depth)); }

				if ((ss - 1)->moveCount == 1 && pos.state()->DirtyPiece[1] == NO_PIECE && (ss - 1)->currentMove != MOVE_NULL)
				{
					Square prevSq = move_to((ss - 1)->currentMove);
					update_cm_stats(ss - 1, moved_piece((ss - 1)->currentMove), prevSq, -bonus(depth + ONE_PLY));
				}

			}
			else if (!pos.capture_or_propawn(ttMove)) {
				Value penalty = -bonus(depth + ONE_PLY);
				Piece pc = moved_piece(ttMove);
				Square sq = move_to(ttMove);
				thisthread->history.update(pc, sq, penalty);
				thisthread->fromTo.update(pos.sidetomove(), ttMove, penalty);
				update_cm_stats(ss, pc, sq, penalty);

			}
		}

		ASSERT(ttValue > -Value_Infinite&&ttValue < Value_Infinite);
		return ttValue;
	}



#endif

	//���ʐ錾
	if (pos.ply_from_startpos > 200&&!RootNode) {
		if (pos.is_nyugyoku()) {
			return mate_in_ply((ss->ply));
		}
	}

//mate
#ifdef MATEONE
	if (!RootNode && !incheck) {
		Move mate;
		if ((mate=pos.mate1ply())!=MOVE_NONE) {
#ifdef MATETEST
			
			
			pos.do_move(mate, &si);
			ExtMove moves_[600], *end;
			end = moves_;
			end = test_move_generation(pos, moves_);
			for (ExtMove* i = moves_; i < end; i++) {
				if (pos.is_legal(i->move)) {
					pos.undo_move();
					cout << pos << endl;
					cout << "checkmove" << endl;
					check_move(mate);
					cout << "recovermove" << endl;
					check_move(i->move);
					ASSERT(0);
				}
			}
			pos.undo_move();
#endif

			ss->static_eval = bestvalue = mate_in_ply((ss->ply)+1);
#ifdef USETT
			//���`�񂱂���move���i�[���Ă�����mateoneply�Ŏ}��؂�̂�ttmove�͕K�v�Ȃ������ʂ��H�H�H
			tte->save(poskey, value_to_tt(bestvalue, ss->ply), BOUND_EXACT, depth,mate/*, ss->static_eval*/, TT.generation());
#endif
			ASSERT(bestvalue > -Value_Infinite&&bestvalue < Value_Infinite);
			return bestvalue;
		}
	}
#endif //mateone


	

	//�]���֐��͖���Ăяo�����ق��������v�Z�ł���
	//����]���֐����Ăяo���̂�SF�̂悤��TT��eval�Ƃǂ��炪�M�p�������邩��r����K�v�͂Ȃ��Ǝv���B
	//�����Ȃ��tt->eval()�͕ۑ�����K�v��������...???���`�`�������������tt.eval()��p�����ق��������b�g���L��̂�������Ȃ�...�i�����͌�ł悭�l���悤�j
	ss->static_eval=staticeval = Eval::eval(pos);

	//stockfish���ɂ��Ă݂�
#if 0
	//���肪�������Ă���ꍇ�͑O�����}�؂�͂��Ȃ�
	//�i�����O�����}�؂肪�o���Ă��܂����炱�̃m�[�h�ŋl��ł��܂��j
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
	/*else if ((ss - 1)->currentMove == MOVE_NULL) {
		staticeval = ss->static_eval = -(ss - 1)->static_eval + 2 * (Value)20;
	}*/
#endif


	if (ss->skip_early_prunning == true) {
		goto moves_loop;
	}
#ifdef USETT
	// Step 6. Razoring (skipped when in check)
	/*
	ttMove�����݂���
	�c��[�����S�ȉ���
	�Î~�T���̒l��razormargin[]�𑫂��Ă�alpha������Ȃ��ꍇ�͎}�؂�ł���

	�Î~�T���̒l��Ԃ��̂Ő��m�Ȗ߂�l�����҂ł���H�H�i���̃m�[�h�ɂ�����j

	�������������F�X�l�����邯��razoring�������Ă�ELO-10�ɂ�������Ȃ������炵���̂ł����܂ŗ͂����ă`���[�j���O���郁���b�g�͂Ȃ�
	*/
	if (!PVNode
		&&  depth < 4 * ONE_PLY
		&&  ttMove == MOVE_NONE
		&&  staticeval + razor_margin[depth / ONE_PLY] <= alpha)
	{

		//�c�肪���ȉ��ŐÓI�]���l��razormargin[3]�𑫂��Ă�alpha������Ȃ��ꍇ�͎}�؂�ł���
		//�����Ȃ��margin[3]??? ���̒l��p�����ق��������̂ł́H
		//razormargin[3]����ӏ��Ŏg���Ă���̂ŕςȒl�ɂȂ��Ă��܂��Ă���H�H

		//stack��ss+1�łȂ�ss�ł����̂��H�H�H�ido_move�����Ă��Ȃ��̂ō\��Ȃ��iss->staticEval�Ȃǂ̏����g���܂킹��I�I�I�I�j�j
		if (depth <= ONE_PLY) {
			return qsearch<NonPV>(pos, ss, alpha, beta, DEPTH_ZERO);
		}

		Value ralpha = alpha - razor_margin[depth / ONE_PLY];
		Value v = qsearch<NonPV>(pos, ss, ralpha, ralpha + 1, DEPTH_ZERO);
		if (v <= ralpha) {
			ASSERT(v > -Value_Infinite&&v < Value_Infinite);
			return v;
		}
	}

#endif

	// Step 7. Futility pruning: child node (skipped when in check)
	/*------------------------------------------------------------------------------------------------------------
	���̎}�͂����Ŋ�����iby�o������j
	�܂葼�̎}�؂���撣���ă`���[�j���O������������̎}�؂���撣���ă`���[�j���O���Ă��ق������ʂ��傫���B


	d��Ŕ҉�ł���]���l�̗\����futilitymargin.(d�͍ő�ł�6)
	�����c��T���[���ő��肪�҉�ł���_�������݂̐ÓI�]���l��������Ă�beta�𒴂��Ă���ꍇ�͂����Ŏ}��؂��Ă��܂��Ă������B
	futility margin ���d�v�ɂȂ��Ă���B�]���l�͐[���ɐ��`�ɑ傫���Ȃ��Ă���

	//�U���܂ł��̊֐��ŗ\���ł���Ƃ����̂͂ǂ��Ȃ񂾂낤��...���������󂢂ق��������C������...
	//��͐i�s�x�ɂ���Ă��̒l�������邱�Ƃ��ł���C������...
	//���ՂP�S�O�I��160�݂�����....
	-------------------------------------------------------------------------------------------------------------*/
	if (!RootNode
		&&depth < 7 * ONE_PLY
		&&staticeval - futility_margin(depth) >= beta
		&& staticeval < Value_known_win
		//nonpawnmaterial�Ƃ��������͏����ɂ͊֌W�Ȃ��͂�
		)
	{
		return staticeval;
	}

	// Step 8. Null move search with verification search
	/*
	�i���̒��ł̗����j
	�ÓI�]���l��beta�𒴂��Ă���ǖʁi���肪�w�}�����Ă����Ǝv����ǖʁi�����I�Ɍ���Όv�Z���ꂽ�肩������Ȃ��j�j�ł����炪����Ɨǂ����Ԃ����Ƃ��Đ󂢒T�������Ă݂�B
	�T������A���Ă������ʂ�����ł�beta�𒴂��Ă����ꍇ�i�܂�z���g�ɑ��肪�w�}�������Ƃ킩�����j�͂���͎}�؂���ł���B
	�p�X��beta�𒴂����Ƃ������Ƃ́A�p�X�����ǂ��w���Ă��w���Ă݂Ă�beta�𒴂���Ƃ������Ƃł���B�i�ǖʂ�Zugzwang�ł͂Ȃ��Ɖ��肷��j
	����Ɨǂ���Ƃ����̂ɂ̓p�X��������p����B�w�ǂ̎w���Ă̓p�X�ɗ��B
	*/

	if (!PVNode
		&&staticeval >= beta
		&& (staticeval >= beta - 35 * (int(depth / ONE_PLY) - 6) || depth >= 13 * ONE_PLY)
		) {



		ss->currentMove = MOVE_NULL;
		ss->counterMoves = nullptr;

		
		//ASSERT((int(staticeval) - int(beta) >= 0));
		

		//R�̒l�͏ꍇ���������Č�ŏڂ�������
		//staticeval�ɑ傫�Ȓl���t�������Ă���R���傫���Ȃ肷���Ă���!
		//beta�̒l���傫���̂�(staticeval - beta) ��int32_t�Ɏ��܂肫���Ă��Ȃ������I�I(����Ȃ��Ƃ͂Ȃ�����)
		//
		Depth R= Depth((823 + 67 * (depth / ONE_PLY)) / 256 + std::min((int(staticeval) - int(beta)) / Eval::PawnValue, 3) * ONE_PLY);
		ASSERT(R > 0);
		pos.do_nullmove(&si);
		(ss + 1)->skip_early_prunning = true;//�O�����}�؂�͂��Ȃ��B�i�Q��A���p�X�͂�낵���Ȃ��j

		//nullmove�͎�荇�����N����Ȃ��̂ŐÎ~�T�����Ă΂Ȃ��B�i�Î~�T���ŉ�����T������悤�ɐ���Ύ�����ύX����j
		null_value = (depth - R) < ONE_PLY ? staticeval : -search<NonPV>(pos, ss + 1, -(beta), -beta + 1, depth - R,!cutNode);
		(ss + 1)->skip_early_prunning = false;
		pos.undo_nullmove();

		//null_value��beta�𒴂����ꍇ
		if (null_value >= beta) {

			//NULLMOVE�Ȃ̂ŋl�݂������Ă����̂܂܋A���Ƃ��������Ȃ�
			if (null_value >= Value_mate_in_maxply) {
				null_value = beta;
			}

			//(staticeval >= beta - 35 * int(depth / ONE_PLY - 6)�̏������������Ă���Ƃ��Ȃ̂Ńz���g��beta�𒴂��邩�̃`�F�b�N�͂��Ȃ��Ă�����
			if (depth < 12 * ONE_PLY&&std::abs(beta) < Value_known_win) {
				return null_value;
			}

			//�c��depth���傫���Ƃ��ł���̂ł����Ɗm�F���Ă����B
			//�܂������ɓ����Ă��Ȃ��悤�ɑO�����}�؂�ɂ͓���Ȃ��悤�ɂ���B
			ss->skip_early_prunning = true;
			//do_move�����Ă��Ȃ��̂�ss,beta�ł悢�B
			Value v= (depth - R) < ONE_PLY ? staticeval : search<NonPV>(pos, ss ,beta-1, beta, depth - R,false);
			ss->skip_early_prunning = false;

			if (v >= beta) {
				ASSERT(null_value > -Value_Infinite&&null_value < Value_Infinite);
				return null_value;
			}
		}

	}

#ifdef   multicut 
	//step9 multicut 
	//nullmove�����キ�Ȃ��Ă��܂����I
	//�����Ə������l����
	/*
	SDT4�Ńc�c�J�i��Selene���s���Ă����Ƃ�����@
	�悭�킩��񂯂�probcut�Ǝ����l���Ɋ�Â��}�؂�@���Ǝv���̂ł����ōs���Ă݂�B
	StockFish�ł�multicut��Probcut(Stockfish����)���ǂ����ʂ��c�������Ƃ͂Ȃ��݂���
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

		//�����łǂ�Ȏw���Ă𐶐����ׂ��Ȃ̂�....
		//capturepropawn??killer���܂߂Ă݂�̂����肩������Ȃ�
		movepicker mp_prob(pos,beta);

		int c = 0;
		const int overbeta= 3;
		
		
		Value bestvalue2 = -Value_Infinite;

		while ((move = mp_prob.return_nextmove()) != MOVE_NONE) {
			//�������ꂽ�w���Ă̐������Ȃ������ꍇ��multicut�͏o���Ȃ��B
			//domove����O��goto����
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
		
		//����probcut�悤�ɕς���
		movepicker mp(pos,ttMove,rbeta-staticeval);

		while ((move = mp.return_nextmove()) != MOVE_NONE) {

			if (pos.is_legal(move)) {
				ss->currentMove = move;
				ss->counterMoves = &thisthread->CounterMoveHistory[moved_piece(move)][move_to(move)];
				pos.do_move(move, &si);
				value = -search<NonPV>(pos, (ss + 1), -rbeta, -rbeta + 1, rdepth,!cutNode);
				pos.undo_move();

				if (value >= rbeta) { ASSERT(value > -Value_Infinite&&value < Value_Infinite);  return value; }
			}

		}
	}




#endif

	//���������[��
	/*
	https://chessprogramming.wikispaces.com/Internal+Iterative+Deepening

	���������[���́i�ߋ���PV�܂���TT�j����v���O������bestmove���������Ȃ������Q�[���c���[�̃m�[�h�ōs����.
	IID�͌��݂̋ǖʂŐ[�������炵�ĒT�������邱�Ƃł悢������������邽�߂Ɏg����B
	IID�͕ی��̂悤�Ȃ��̂��B�قƂ�ǂ̏ꍇ����͕K�v�ł͂Ȃ��A���������̃R�X�g�͔��ɏ������B�@�����đ傫�Ȏ��Ԃ̖��ʂ�����邱�Ƃ��ł���B
	*/
	/*
	�[����6�ȏ�
	ttMove�����݂��Ȃ�
	PVNode�����݂��Ȃ�||�ÓI�]���l��beta+256�@�̂Ƃ�

	�O�����}�؂���I�t�ɂ��āA��ɏ����T��������ttmove��p�ӂ���


	���ꂷ��Ƃ����Ȃ����炢�T���[���������܂��˂��I�I�I
	�����ƍ��ꂩ�痝�����ׂ���
	*/
#ifdef USETT
	if (depth >= 6 * ONE_PLY
		&&ttMove == MOVE_NONE
		&& (PVNode || staticeval + 256 >= beta)
		) {

		Depth d = (3 * (int)depth / (4 * ONE_PLY) - 2)*ONE_PLY;
		ss->skip_early_prunning = true;
		//�����ŋA���Ă����l�������ɗ��p�ł��Ȃ����H�H�H
		search<NT>(pos, ss, alpha, beta, d,cutNode);
		ss->skip_early_prunning = false;
		tte = TT.probe(poskey, TThit);
		//TT��������Ă��܂��ꍇ���l���Ă�������
		if (TThit) {
			ttValue = tte->value();
			ttdepth = tte->depth();
			//ttEval = tte->eval();
			ttBound = tte->bound();
			ttMove = tte->move();
		}
		else {
			ttValue = Value_error;
			ttdepth = DEPTH_NONE;
			//ttEval = Value_error;
			ttBound = BOUND_NONE;
			ttMove = MOVE_NONE;
		}
	}




#endif



	//���肪�������Ă���ꍇ�͑O�����}�؂�͂��Ȃ�
	//�i�����O�����}�؂肪�o���Ă��܂����炱�̃m�[�h�ŋl��ł��܂��j
moves_loop:
	
#ifndef  LEARN

#define EXTENSION

#endif // ! LEARN
	const CounterMoveStats* cmh = (ss - 1)->counterMoves;
	const CounterMoveStats* fmh = (ss - 2)->counterMoves;
	const CounterMoveStats* fmh2 = (ss - 4)->counterMoves;

			//�ÓI�]���l���O�̎����̎�Ԃ����悭�Ȃ��Ă��邩�܂��͈ȑO�̕]���l�����݂��Ȃ�����
	improve = ss->static_eval >= (ss - 2)->static_eval || (ss - 2)->static_eval == Value_error;
#ifdef EXTENSION
		/*=================================================================================
				singularExtension
				���̋ǖʂŗǂ������肪������Ȃ��Ƃ��ɂ��̎w���������������
			
				����
			
				rootnode�ł͂Ȃ��B�i�܂�����Ⴛ�����j
				�[����8�ȏ�i����͂ǂ����낤....���܂�l�����킩��Ȃ��j
				ttMove�����݂���ittMove�͂悢������ɂȂ肤��͂��Ȃ̂ł��̎w����������������j
				�]���l������ȏ�悭�Ȃ肤��\��������
				�������ꂽ��������܂���������̂͂�낵���Ȃ�
				ttvalue��ttdepth��depth-3*ONE_PLY�Ŋ��ƕۏ؂���Ă���
		===================================================================================*/
	singler_extension = !RootNode
		 &&depth >= 8 * ONE_PLY
		&&ttMove != MOVE_NONE
		&&excludedmove == MOVE_NONE
		&&abs(ttValue)<Value_known_win
		&& (ttBound&BOUND_LOWER)//LOWER or EXACT
		 && ttdepth >= depth - 3 * ONE_PLY;
#endif
		
		




#ifdef USETT
	movepicker mp(pos,ss,ttMove,depth);
#else 
	movepicker mp(pos, ss, MOVE_NONE,depth);
#endif
	//�w�K��countermoves�ɂ������Ȏw���肪�����Ă���
	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (move == ss->excludedMove) {continue;}
	
		/*if (!RootNode) {
			if (pos.is_legal(move) == false) { continue; }
		}
		*/

		if (NT == Root&&thisthread->find_rootmove(move) == nullptr) { continue; }
		
		ss->moveCount = ++movecount;

		if (PVNode) { (ss + 1)->pv = nullptr; }

		/*
		capture propawn�̎w����ɂȂ肤��̂�cappropawn�̃X�e�[�W��EVERSION�̃X�e�[�W����
		*/
		Stage st = mp.ret_stage();
		if (st == CAP_PRO_PAWN||st==BAD_CAPTURES) {
			ASSERT(pos.capture_or_propawn(move) == true); //���̂Ƃ���assert�Ȃ��Ă����v����
			CaptureorPropawn = true;
		}
		else if (st == QUIET) {
			ASSERT(pos.capture_or_propawn(move) == false);
			CaptureorPropawn = false;
		}
		else {
			CaptureorPropawn = pos.capture_or_propawn(move);
		}

		//ss->moveCount=++movecount;
		extension = DEPTH_ZERO;
		givescheck = pos.is_gives_check(move);

		 move_count_pruning = depth < 16 * ONE_PLY&&movecount >= FutilityMoveCounts[improve][depth / ONE_PLY];

#ifdef EXTENSION
					//=====================
					//������̉���
					//=====================
			
			
				// Step 12. Extend checks
					/*=========================================================================================================
					�����������w�������������
					
					����
					�����������w����
					���̂ق��̍�����łȂ�
					see�����ł���
					
					�Z�I�ł͉����������ꂽ�ǖʂɋ�����鍇�@��̐��͏��Ȃ��̂�see�����̋ǖʂł������HALF_PLY��������炵������ɏK���B
					==========================================================================================================*/
			
			/*if (givescheck
				 && !move_count_pruning) {
				if (pos.see_ge(move,Value_Zero)) { extension = ONE_PLY; }
				else { extension = HALF_PLY; }
			}*/
			//singler extension
			/*
			���̋ǖʂ�ttmove�ȊO�̍�����ȊO�ɗǂ������肪�Ȃ����ttmove����������
			���X��depth����������Ă��܂���128��ڂ܂œ��B���Ă��܂��Ă���o�O���������Ă���
			*/
			if (singler_extension
				 &&move == ttMove
				//&& !extension
				&&pos.is_legal(move)) {
				ASSERT(ttValue!=Value_error)
				//Value rBeta = std::max(ttValue - 4 * depth,Value_Mated);
				Value rBeta = std::max(ttValue - 8*(ss->ply), Value_Mated);
				//Depth d = (depth / (2 * int(ONE_PLY)))*int(ONE_PLY);
				Depth d = depth / 2;
				ss->excludedMove = move;
				ss->skip_early_prunning = true;
				value = search<NonPV>(pos, ss, rBeta - 1, rBeta, d,cutNode);
				ss->skip_early_prunning = false;
				ss->excludedMove = MOVE_NONE;
				if (value < rBeta) { extension = ONE_PLY;	}
			
			}
			else if (givescheck
				&& !move_count_pruning) {
				if (pos.see_ge(move, Value_Zero)) { extension = ONE_PLY; }
				else { extension = HALF_PLY; }
			}
#endif
		newdepth = depth - ONE_PLY + extension;//������-ONE_PLY+ONE_PLY�����肩������Ă��܂��Ė����ɒT�����Ă���I�I
		

		//�O�����}�؂�
		//����͍�����̉������l����O�ɂ��ׂ��ł͂Ȃ����H�H�H�H
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

				//�e�m�[�h��futility
				if (predicted_depth < 7 * ONE_PLY
					&&staticeval + 256 + 200 * predicted_depth / int(ONE_PLY) <= alpha) {
					continue;
				}

				if (predicted_depth < 8 * ONE_PLY) {
					Value see_v = predicted_depth < 4 * ONE_PLY ? Value_Zero :
						Value(-Eval::PawnValue * 2 * int(predicted_depth - 3 * ONE_PLY) / ONE_PLY);
					if (!pos.see_ge(move,see_v)) { continue; }
				}

			}
			else if (depth < 3 * ONE_PLY
				&& (mp.see_sign() < 0 || (!mp.see_sign() && !pos.see_ge(move,Value_Zero)))) {
				continue;
			}

		}
		//���ꑁ���Ȃ�񂾂낤���H�H
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif
		/*
		domove�̒��O��legal�`�F�b�N
		legal�ŏ������LMR�ȂǂŊ������ق��������Č����I�Ƃ������Ƃ��H�H�H
		�������񍇖@���SEE�������肷��̂ŃG���[���N���Ȃ����S�z����...
		�����is_legal��SF�Ɗ��S�ɓ����ł͂Ȃ���...

		LMR��continue���N�����Ă��܂���movecount��--����Ȃ��̂�movecount���s���ɑ傫���Ȃ��Ă��܂��������Ǝv���̂������....
		*/
		if (!RootNode) {
			if (pos.is_legal(move) == false) { ss->moveCount = --movecount; continue; }
		}

		ss->currentMove = move;

		ss->counterMoves = &thisthread->CounterMoveHistory[moved_piece(move)][move_to(move)];

		
		pos.do_move(move, &si, givescheck);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif

#if 0
		doFullDepthSearch = (PVNode&&movecount == 1);

		//���̕��@����nonPVnode����PVnode�Ɉڂ��Ă����̂�PV�����������Ȃ�\�������邩�H�H�H�H�H
		/*
		�w�K�����܂苭���Ȃ�Ȃ��̂͊w�K���Ƀo�O������̂ł͂Ȃ��T���������āA�܂肱�ꂪ�������I�H
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
		// �[�������炵���T���B�������̎w���肪�ǂ�������ł��邱�Ƃ�������Ί��S�Ȑ[���ŒT�����Ȃ���
		/*
		��΂ɂ����Ŏ}�����肷���Ă���

		nonPV�̎w�����PV�ɂȂ�Ȃ��̂��������Ȃ��̂��Ǝv���B
		�ł�nonPV�������PV�ɂȂ����炻���PV�����������Ȃ邩...????�ł�����Ń��[�e�B���O�������Ă邵�Ȃ�...

		���̑ΐ�����Ă݂���lmr������Ǝw����ɐ؂ꖡ���Ȃ��Ȃ���
		����̋ʂ��Ȃ��Ȃ��l�߂邱�Ƃ��ł��Ă��Ȃ��悤�Ɋ������B
		�I�Ղ͓���Ȃ��ق����������H
		�K���ɕs��点��w������ڗ������̂�PAWNPROMOTE�͊܂߂Ȃ��ق��������̂���

		����܂�������ɂ��������Ă����[�e�B���O�������Ă��܂����B
		*/
		if (depth >= 3 * ONE_PLY
			&&movecount > 1
			&& (!CaptureorPropawn || move_count_pruning)
			)
		{
			//PV�ł�reducation�͂��Ȃ� ���̑��ł�reducation����
			Depth r = reduction<PVNode>(improve, depth, movecount);

			if (CaptureorPropawn) {
				r -= r ? ONE_PLY : DEPTH_ZERO;
			}
			else {

				// Increase reduction for cut nodes
				if (cutNode) {r += 2*ONE_PLY;}
				//�ߊl���瓦���w����̏ꍇ��reducation�����炷
				//����PAWN����_���I��̐F�łǂ����turn�����f����̂�
				if (!pos.see_ge(make_move(move_to(move), move_from(move), add_color(PAWN,opposite(pos.sidetomove()))),Value_Zero)) {
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
			//d==newdepth�̏ꍇ�͓��������ōĒT�����邱�ƂɂȂ��Ă��܂�
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

		ASSERT(value > -Value_Infinite&&value < Value_Infinite);

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

				/*
				bestmove���ύX���ꂽ�񐔂��L�^����B����͎��ԊǗ��ɗp������
				����nestmove���p�ɂɕύX����Ă���΂����Ǝ��Ԃ��g�����Ƃ�����
				*/
				if (movecount > 1 && thisthread == Threadpool.main()) {
					++static_cast<MainThread*>(thisthread)->bestMoveChanges;
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

		//history�l�����邽�߂�alpha�𒴂����Ȃ���������quiet�̎w���Ă͊i�[����
		if (CaptureorPropawn == false && move != bestMove&&quiets_count < 64) {
			Quiets_Moves[quiets_count++] = move;
		}


	}//�w�����while

	
	if (movecount == 0) {
		bestvalue= excludedmove?alpha: mated_in_ply(ss->ply);
	}
	else if (bestMove != MOVE_NONE) {

		//bestmmove��alpha�l�𒴂���悤�Ȏw���Ă̂���
		//�A���t�@�l�𒴂���悤�ȗL���Ȏw���肪�������ꍇ
		//�i���J�b�g���N�����Ă��܂����w���Ăɑ΂��Ă��T�����Z���Ȃ�Ƃ����Ӗ��ŗǂ��l������ׂ��ł���j

		//�s��ɋ�Ȃ��܂��͎����̋�ł͂Ȃ�
		/*if (pos.piece_on(move_to(bestMove)) != NO_PIECE && piece_color(pos.piece_on(move_to(bestMove))) == pos.sidetomove()) {
			ASSERT(0);
		}*/
		// Quiet best move: update killers, history and countermoves
		//bestmove��QUIET�ł������ꍇ��bestmove�ɗǂ��l�����đ��̎w���ĂɈ����l������
		if (!pos.capture_or_propawn(bestMove)) {
			update_stats(pos, ss, bestMove, Quiets_Moves, quiets_count, bonus(depth));
		}
		//Extra penalty for a quiet TT move in previous ply when it gets refuted���Ԃ���Ă��܂���TT�̍�����Ɉ����l������B
		if ((ss - 1)->moveCount == 1 && pos.state()->DirtyPiece[1] == NO_PIECE && (ss - 1)->currentMove != MOVE_NULL) {
			int d = depth / ONE_PLY;
			Value penalty = Value(d * d + 4 * d + 1);
			Square prevSq = move_to((ss - 1)->currentMove);
			update_cm_stats(ss - 1, moved_piece((ss - 1)->currentMove), prevSq, -penalty);//�ړ�������̋�ł���̂ł���ł����B����������chess�ƈႢ���肪�܂܂�邩������Ȃ��̂ňړ��O�̋����g��
		}
	}
	// Bonus for prior countermove that caused the fail low
	//alpha�𒴂���悤�ȍ����肪�Ȃ������Ƃ������Ƃ͑���̍�����͂悢������ł������̂ő���̍������countermove�ɗǂ��l������
	else if (depth >= 3 * ONE_PLY && pos.state()->DirtyPiece[1] == NO_PIECE&& is_ok((ss - 1)->currentMove)&& (ss - 1)->currentMove!=MOVE_NULL) {
		int d = depth / ONE_PLY;
		Value bonus = Value(d * d + 2 * d - 2);
		Square prevSq = move_to((ss - 1)->currentMove);
		update_cm_stats(ss - 1, moved_piece((ss - 1)->currentMove), prevSq, bonus);
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

	if (excludedmove == MOVE_NONE) {
		tte->save(poskey, value_to_tt(bestvalue, ss->ply),
			bestvalue >= beta ? BOUND_LOWER :
			PVNode&&bestMove ? BOUND_EXACT : BOUND_UPPER,
			depth, bestMove/*, staticeval*/, TT.generation());
	}

	//tte->save(poskey, value_to_tt(bestvalue, ss->ply),
	//	bestvalue >= beta ? BOUND_LOWER :
	//	PVNode&&bestMove ? BOUND_EXACT : BOUND_UPPER,
	//	depth, bestMove/*, staticeval*/, TT.generation());
#endif
	ASSERT(bestvalue > -Value_Infinite&&bestvalue < Value_Infinite);

	return bestvalue;


}

//�Î~�T��(�������Ȃ莞�Ԃ�H�����߂ł��邾���}�؂肵����)
//http://yaneuraou.yaneu.com/2016/11/03/%E6%8E%A2%E7%B4%A2%E3%81%AB%E3%81%8A%E3%81%91%E3%82%8B%E6%9E%9D%E5%88%88%E3%82%8A%E3%81%AE%E9%9A%8E%E5%B1%A4%E6%80%A7/
template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	const bool PvNode = (NT == PV);
	ASSERT( depth <= DEPTH_ZERO);
	ASSERT(alpha >= -Value_Infinite&&alpha < beta&&beta<=Value_Infinite);
	ASSERT(PvNode || (alpha == beta - 1));
	ASSERT(pos.state()->lastmove != MOVE_NULL);

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove;
	Value bestvalue;
	Value value;
	StateInfo si;
	si.clear_stPP();
	Value staticeval,oldAlpha;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
	//-----TT�֘A
	//�ǂݑ��ł�TT����ǂݍ���ł���r����TT���j�󂳂ꂽ�ꍇ�̂��Ƃ��l���Ă����̂ŁA������Q�l�ɂ���
	//Key poskey;
	bool TThit;
	TPTEntry* tte;
	Move ttMove;
	Value ttValue;
	//Value ttEval;
	Bound ttBound;
	Depth ttdepth;
	Value futilitybase = -Value_Infinite;
	Value futilityvalue;
	bool evasionPrunable;




	if (PvNode)
	{
		// To flag BOUND_EXACT when eval above alpha and no available moves
		//�]���l��alpha�����A�����@�肪�Ȃ��ꍇ��flag�Ƃ��ėp����B
		oldAlpha = alpha; 
		(ss + 1)->pv = pv;
		ss->pv[0] = MOVE_NONE;
	}
	ss->currentMove = bestMove = MOVE_NONE;
	ss->ply = (ss - 1)->ply + 1;
	//���Ԓʂ�Ɏw������w���Ă���Ȃ������̂Ő��q�T���ł����Ԃ�����
	//��荇���̓r���ŕ]����Ԃ����Ǝ��ɂ܂����̂łł��邾���������Ȃ��̂���....
#ifndef LEARN
	//timer thread��p�ӂ����ɂ����Ŏ��Ԃ��m�F����B
	//stockfish����
	//if (thisthread->resetCalls.load(std::memory_order_relaxed)) {

	//	thisthread->resetCalls = false;
	//	thisthread->call_count = 0;

	//}
	//
	//if (++thisthread->call_count > 4096) {
	//	//���񉻂̂Ƃ��͂��ׂẴX���b�h�ł��̑�����s���B
	//	thisthread->resetCalls = true;
	//	check_time();
	//}
#endif
	
#ifdef USETT
	const Key posKey = pos.key();
	ASSERT((posKey & uint64_t(1)) == pos.sidetomove());
	tte = TT.probe(posKey, TThit);
	if (TThit) {
		ttValue = value_from_tt(tte->value(), ss->ply);
		ttdepth = tte->depth();
		//ttEval = tte->eval();
		ttBound = tte->bound();
		ttMove = tte->move();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		//ttEval = Value_error;
		ttBound = BOUND_NONE;
		ttMove = MOVE_NONE;
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
	if (!PvNode
		&&TThit
		&&ttdepth >= depth
		&&ttValue != Value_error//���ꍡ�̂Ƃ��낢��Ȃ�(���������Əڂ����ǂޕK�v������)
		&& (ttValue >= beta ? (ttBound&BOUND_LOWER) : (ttBound&BOUND_UPPER))//BOUND_EXACT = BOUND_UPPER | BOUND_LOWER�ł���̂łǂ����&��������
		) {
		ss->currentMove = ttMove;
		ASSERT(ttValue > -Value_Infinite&&ttValue < Value_Infinite);
		return ttValue;
	}
#endif
#ifdef MATEONE
	if (!incheck) {
		Move mate;
		if ((mate=pos.mate1ply())!=MOVE_NONE) {
#ifdef MATETEST
			pos.do_move(mate, &si);
			
			ExtMove moves_[600], *end;
			end = moves_;
			end = test_move_generation(pos, moves_);
			for (ExtMove* i = moves_; i < end; i++) {
				if (pos.is_legal(i->move)||!pos.is_incheck()) {
					pos.undo_move();
					cout << pos << endl;
					cout << "checkmove" << endl;
					check_move(mate);
					cout << "recovermove" << endl;
					check_move(i->move);
					ASSERT(0);
				}
			}
			pos.undo_move();

			
#endif
			return mate_in_ply(ss->ply + 1);
		}

	}
#endif


	//�����ɑO�����}�؂�̃R�[�h������


	//�Î~�T���͋�̎�荇���̂悤�ȕ]���l���s����ȋǖʂ�扄�΂��ĒT���𑱂��邱�ƂŒT���̖��[�]���l��␳���A��萳�m�ɂ�����̂ł���Ǝv���Ă���B
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
				tte->save(posKey, value_to_tt(bestvalue, ss->ply), BOUND_LOWER, DEPTH_NONE, MOVE_NONE/*, staticeval*/, TT.generation());
			}
#endif
			ASSERT(bestvalue > -Value_Infinite&&bestvalue < Value_Infinite);

			return bestvalue;

		}

		if (PvNode&&bestvalue > alpha) {
			alpha = bestvalue;
		}

		futilitybase = bestvalue + 128;
	}

#if 0
	if (!incheck) {

		//stand pat �����������Ă݂�B�i�������܂��낵���Ȃ���΍̗p���������B�j
		/*
		stand pat(�|�[�J�[�Ŏw�����̃J�[�h��ύX�����Ɏ�����߂�Ƃ����Ӗ��̒P��)

		���̋ǖʂ̐ÓI�]���l��value��lower bound�Ƃ��ėp����B
		���ʂ͏��Ȃ��Ƃ�1�̎w���Ă͂���lower bound�ȏ�̎w���Ă�����ƍl������B
		�����nullmove observation�i�ǖʂɂ͕K��1��pass�ȏ�ɗǂ��w���肪����͂��ł���Ƃ����l���j�Ɋ�Â��Ă���B
		����͌���Zugzwang(�p�X���ł��ǂ��ǖ�)�ɂ��Ȃ��ƍl����ꍇ�i�����ɂ�pass���ł��ǂ��w����ɂȂ�ǖʂ͂Ȃ��Ȃ��o�Ă��Ȃ��炵���j�j
		https://chessprogramming.wikispaces.com/Null+Move+Observation
		http://ponanza.hatenadiary.jp/entry/2014/04/02/220239

		�����@lower bound(stand pat score) �����ł�>=beta�ł���΁A
		stand pat score(fail-soft)��beta(fail-hard)��lowerbound�Ƃ���return �ł���B
		�i�ǖʂ̕]���l��standpat�ȏ�ɂȂ�͂��Ȃ̂Łj

		[
		�Ƃ���������Ȃ��Ƃ��������̂Ȃ�O�����}�؂�͑S��eval>=beta�ōς�ł��܂�����Ȃ���....??? 
		standpat�͐Î~�T���ł̂݋������̂��H�H����Ȃ��Ƃ͂Ȃ��͂��B�悭�킩�������ƍl���Ȃ���...
		�������Î~�T����standpat������ƑO�̃o�[�W�����ɂ��Ȃ菟���z���悤�ɂȂ����i�悭�킩���j
		 ]

		 [
		 �Î~�T���ł̓R�����炢�K���Ƀo�b�T���}��؂��Ă��܂��Ă������̂�...???
		 
		 ]


		fali soft
		http://d.hatena.ne.jp/hiyokoshogi/20111128/1322409710
		https://ja.wikipedia.org/wiki/Negascout
		�ʏ�̃A���t�@�x�[�^�@���Ԃ��l�͒T�����͈͓̔��̒l�ł��邪�A �q�m�[�h��T���������ʂ��T�����͈̔͊O�������ꍇ�A
		�T�����̋��E�l�ł͂Ȃ����ۂɏo�������q�m�[�h�̍ő�l��Ԃ��ƒT���ʂ����鎖������B
		����͐e�m�[�h�ɓ`������Ƃ��Ƀ��l�ȏ�ƂȂ��ăJ�b�g�����胿�l���X�V���ĒT���������߂���ł���\���������Ȃ邽�߂ł���B
		���̂悤�Ȑ����� Fail-Soft �ƌ����A Null Window Search �Ȃǂ̋����T�����ɂ��T����u���\���g�����T��������ꍇ�ɂ��̌��ʂ��悭�����B
		*/
		
	}
#endif
//	
//#ifdef LEARN
//	//�w�K���͐��q�T���[���R�܂łŎ~�߂�B�i����Ȃ��Ƃ͂����o�b�T���}��؂��}�؂���@�𓱓��������j
//	if (depth < -3 * ONE_PLY) { return bestvalue; }
//#endif

	//�R���ŏ�肭�O�̎w����̈ړ����^�����Ă���Ǝv��
	//������nullmove�������Ă����ꍇ�̂��Ƃ��l���Ȃ��Ƃ����Ȃ��B
	//�Ƃ�����nullmove�������Ă�����惊�Ԃ��Ȃ�Ă��肦�Ȃ��̂ł����ŕ]���l�Ԃ����������ł���(�������������Ȃ�b�͕ʁj
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

		//if (pos.is_legal(move) == false) { continue; }



		/*if (pos.check_nihu(move) == true) {

			cout << "nihu " << endl;
			cout << move << endl;
			cout << pos << endl;
			cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
			cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
			UNREACHABLE;
		}*/
		
		bool givescheck = pos.is_gives_check(move);

		
		//futility 
		if (!incheck
			&&!givescheck  //��������Ƃ��ē����ׂ����Ǝv������Ǎ���domove�̎d�l�ł͂Ȃ��Ȃ�����B
			&&futilitybase > -Value_known_win
			) {
			futilityvalue = futilitybase + Value(Eval::piece_value[pos.piece_on(move_to(move))]);
			//��荇���Ȃ̂ň��ڂ��Ƃ�������alpha-128�𒴂����Ȃ��悤�ł���΂��̎w������l���Ȃ�
			if (futilityvalue <= alpha) {
				bestvalue = std::max(bestvalue, futilityvalue);
				continue;
			}
			if (futilitybase <= alpha && !pos.see_ge(move,Value_Zero+1))
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
			&& !pos.see_ge(move,Value_Zero)) {
			continue;
		}
#endif


		//���ꑁ���Ȃ�񂾂낤���H�H
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif


		//pos.do_move(move, &si);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif
		//domove�̒��O��legalcheck
		if (pos.is_legal(move) == false) { continue; }
		movecount++;
		pos.do_move(move, &si, givescheck);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();

		ASSERT(value > -Value_Infinite&&value < Value_Infinite);

#ifndef LEARN
		//if (signal.stop.load(std::memory_order_relaxed)) {
		//	//���Ԑ؂��fail hard�ɂ��Ă��邪������fail soft�ɂ��Ă����ׂ����H
		//	return alpha;
		//}
#endif
		if (value > bestvalue) {
			//��荇����ǂ񂾕��]���l�͂�萳�m�ɂȂ���
			bestvalue = value;

			//alpha�l�͒�グ���ꂽ
			if (value > alpha) {
				
				if (PvNode) {
					update_pv(ss->pv, move, (ss + 1)->pv);
				}

				if (PvNode && value < beta) 
				{
					//������alpha�l�𒴂��̏���������B
					alpha = value;
					bestMove = move;
				}
				else 
				{
					//PVnode�ȊO�ł�null window search�Ȃ̂�alpha�����Ƃ�beta�����̂��Ƃł���
#ifdef USETT
					//value��beta�𒴂����Ƃ������Ƃ͂��̓_���ȏ�̎w���肪�܂����邩������Ȃ��Ƃ������ƂȂ̂�BOUND_LOWER
					tte->save(posKey, value_to_tt(value, ss->ply), BOUND_LOWER,
						depth, move/*, staticeval*/, TT.generation());
#endif
					return value;
				}


			}
		}
	}
	//������������Ă��č��@�肪�Ȃ���΂���͋l��
	if (incheck&&movecount == 0) {
		return mated_in_ply(ss->ply);
	}

#ifdef USETT
	//PVnode&&bestvalue��oldalpha�𒴂����Ȃ������Ƃ������Ƃ�oldalpha�͏���ł���,bestvalue���܂�����ł���B
	//PVnode&&bestvalue>oldalpha�Ƃ������Ƃ̓R���͐��m�ȕ]���l�ł���B
	//PVnode�ł͂Ȃ��Ƃ������Ƃ̓������͋N����Ȃ�����nullwindow�̃A���t�@�l�𒴂����Ȃ������܂�UPPER�ł���
	tte->save(posKey, value_to_tt(bestvalue, ss->ply),
		PvNode && bestvalue > oldAlpha ? BOUND_EXACT : BOUND_UPPER,
		depth, bestMove/*, staticeval*/, TT.generation());
#endif
	ASSERT(bestvalue > -Value_Infinite&&bestvalue < Value_Infinite);
	return bestvalue;
}


// update_stats() updates killers, history, countermove and countermove plus
// follow-up move history when a new quiet best move is found.
/*
bonus = Value(d * d + 2 * d - 2);
bonus��depth�̂Q��ɔ�Ⴗ��


d 0 d^2+2d-2 -2  depth==0�͐Î~�T���ł���̂�depth=0 -2�̂������������Ƃ͂Ȃ��I
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

�����ōׂ����l�̕␳�Ȃǂ͍l���Ă��d���Ȃ�
bonus�̒l�͂����ɂǂ�ǂ�ς���Ă����B

*/
void update_stats(const Position& pos, Stack* ss,const Move bestmove,
	Move* quiets,const int quietsCnt,const Value bonus) {

	if (ss->killers[0] != bestmove)
	{
		ss->killers[1] = ss->killers[0];
		ss->killers[0] = bestmove;
	}



	//bestmove��+=����bonus
	Color c = pos.sidetomove();
	Thread* thisthread = pos.searcher();
	thisthread->history.update(moved_piece(bestmove), move_to(bestmove), bonus);
	thisthread->fromTo.update(c, bestmove, bonus);
	update_cm_stats(ss, moved_piece(bestmove), move_to(bestmove), bonus);

	//countermove��currentmove�̊i�[�͂܂�
	if ((ss - 1)->counterMoves)
	{
		Square prevSq = move_to((ss - 1)->currentMove);
		thisthread->counterMoves.update(pos.piece_on(prevSq), prevSq, bestmove);
	}

	// Decrease all the other played quiet moves
	//alpha���X�V���Ȃ������w����ɑ΂���+=���̒l
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

#ifdef LEARN
//�w�K���Ɏg���}�؂���ɗ͗}�����T���B�i�e�X�g�j

template <Nodetype NT>Value lsearch(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode) {


	ASSERT(alpha < beta);



	//������
	const bool PVNode = (NT == PV || NT == Root);
	const bool RootNode = (NT == Root);
	bool doFullDepthSearch = false;

	Move pv[MAX_PLY + 1];
	Move move = MOVE_NONE;
	Move bestMove = MOVE_NONE;
	Move excludedmove = MOVE_NONE;
	Move Quiets_Moves[64];
	int quiets_count = 0;
	Value value, null_value;
	StateInfo si;
	si.clear_stPP();
	Value staticeval = Value_Zero;
	int movecount = ss->moveCount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
	ss->ply = (ss - 1)->ply + 1;
	Value bestvalue = -Value_Infinite;
	//-----TT�֘A
	//�ǂݑ��ł�TT����ǂݍ���ł���r����TT���j�󂳂ꂽ�ꍇ�̂��Ƃ��l���Ă����̂ŁA������Q�l�ɂ���
	//Key poskey;
	
	bool CaptureorPropawn;
	bool givescheck, improve, singler_extension, move_count_pruning;
	Depth extension, newdepth;

	//seldepth�̍X�V�������ōs��
	if (PVNode && (thisthread->seldepth < ss->ply)) {
		thisthread->seldepth = ss->ply;
	}

	if (!RootNode) {
		//step2
		if (signal.stop.load(std::memory_order_relaxed)) {
			return Eval::eval(pos);
		}
		alpha = std::max(mated_in_ply(ss->ply), alpha);//alpha=max(-mate+ply,alpha)�@alpha�̒l�͌��݂܂���Ă���l�����������͐���Ȃ� �܂�alpha�͍ŏ��ł�-mate+ply
		beta = std::min(mate_in_ply(ss->ply + 1), beta);//beta=min(mate-ply,beta)  beta�̒l�͎��̎w����ŋl�ޒl�����傫���͂Ȃ�Ȃ��@�܂�beta�͍ő�ł�mate-(ply+1)

		if (alpha >= beta) {
			return alpha;
		}
	}
		ss->currentMove = (ss + 1)->excludedMove = bestMove = MOVE_NONE;
		ss->counterMoves = nullptr;
		(ss + 1)->skip_early_prunning = false;
		(ss + 2)->killers[0] = (ss + 2)->killers[1] = MOVE_NONE;


		ss->static_eval = staticeval = Eval::eval(pos);
		if (incheck) {
			//	cout << "incheck" << endl;
			
		}
		else if ((ss - 1)->currentMove == MOVE_NULL) {
			staticeval = ss->static_eval = -(ss - 1)->static_eval + 2 * (Value)20;
		}
		movepicker mp(pos, ss, MOVE_NONE, depth);


		while ((move = mp.return_nextmove()) != MOVE_NONE) {
			if (move == ss->excludedMove) { continue; }

			if (!RootNode) {
				if (pos.is_legal(move) == false) { continue; }
			}


			if (NT == Root&&thisthread->find_rootmove(move) == nullptr) { continue; }

			Stage st = mp.ret_stage();
			if (st == CAP_PRO_PAWN || st == BAD_CAPTURES) {
				//ASSERT(pos.capture_or_propawn(move) == true); ���̂Ƃ���assert�Ȃ��Ă����v����
				CaptureorPropawn = true;
			}
			else if (st == QUIET) {
				//ASSERT(pos.capture_or_propawn(move) == false);
				CaptureorPropawn = false;
			}
			else {
				CaptureorPropawn = pos.capture_or_propawn(move);
			}
			ss->moveCount = ++movecount;
			ss->currentMove = move;

			ss->counterMoves = &thisthread->CounterMoveHistory[moved_piece(move)][move_to(move)];

			givescheck = pos.is_gives_check(move);


			pos.do_move(move, &si, givescheck);

			newdepth = depth - ONE_PLY;


			
			if (newdepth >= ONE_PLY) {
				//null window search
				value = -lsearch<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, newdepth, !cutNode);
			}
			else {
				//value = Eval::eval(pos);
				value = -lqsearch<NonPV>(pos, ss + 1, -(alpha + 1), -alpha, DEPTH_ZERO);
			}

			if (PVNode && (movecount == 1 || (value>alpha && (RootNode || value<beta)))) {
				//ss->pvmove = move;
				(ss + 1)->pv = pv;
				(ss + 1)->pv[0] = MOVE_NONE;
				if (newdepth >= ONE_PLY) {
					value = -lsearch<PV>(pos, ss + 1, -beta, -alpha, newdepth, false);
				}
				else {
					//value = Eval::eval(pos);
					value = -lqsearch<PV>(pos, ss + 1, -beta, -alpha, DEPTH_ZERO);
				}
			}

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
		}
		if (movecount == 0) {
			bestvalue = excludedmove ? alpha : mated_in_ply(ss->ply);
		}
	return bestvalue;

}

template <Nodetype NT>
Value lqsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth) {

	const bool PvNode = (NT == PV);
	ASSERT(depth <= DEPTH_ZERO);
	ASSERT(alpha < beta);
	ASSERT(pos.state()->lastmove != MOVE_NULL);

	Move pv[MAX_PLY + 1];
	Move move;
	Move bestMove = MOVE_NONE;
	Value bestvalue;
	Value value;
	StateInfo si;
	Value staticeval, oldAlpha;
	int movecount = 0;
	bool incheck = pos.is_incheck();
	Thread* thisthread = pos.searcher();
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
	Value futilitybase = -Value_Infinite;
	Value futilityvalue;
	bool evasionPrunable;
	if (PvNode)
	{
		// To flag BOUND_EXACT when eval above alpha and no available moves
		//�]���l��alpha�����A�����@�肪�Ȃ��ꍇ��flag�Ƃ��ėp����B
		oldAlpha = alpha;
		(ss + 1)->pv = pv;
		ss->pv[0] = MOVE_NONE;
	}

	(ss)->static_eval = bestvalue = staticeval = Eval::eval(pos);

	if (incheck) {
		ss->static_eval = Value_error;
		bestvalue = futilitybase = -Value_Infinite;
	}
	else {
		

		if (bestvalue >= beta) {

			return bestvalue;

		}

		if (PvNode&&bestvalue > alpha) {
			alpha = bestvalue;
		}

		futilitybase = bestvalue + 128;
	}

	movepicker mp(pos, move_to(pos.state()->lastmove), MOVE_NONE);
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


		//futility 
		if (!incheck
			&& !givescheck  //��������Ƃ��ē����ׂ����Ǝv������Ǎ���domove�̎d�l�ł͂Ȃ��Ȃ�����B
			&&futilitybase > -Value_known_win
			) {
			futilityvalue = futilitybase + Value(Eval::piece_value[pos.piece_on(move_to(move))]);
			//��荇���Ȃ̂ň��ڂ��Ƃ�������alpha-128�𒴂����Ȃ��悤�ł���΂��̎w������l���Ȃ�
			if (futilityvalue <= alpha) {
				bestvalue = std::max(bestvalue, futilityvalue);
				continue;
			}
			if (futilitybase <= alpha && /*pos.see(move) <= Value_Zero*/pos.see_ge(move,Value_Zero+1))
			{
				bestvalue = std::max(bestvalue, futilitybase);
				continue;
			}
		}

		movecount++;

		pos.do_move(move, &si, givescheck);
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();

		if (value > bestvalue) {
			//��荇����ǂ񂾕��]���l�͂�萳�m�ɂȂ���
			bestvalue = value;

			//alpha�l�͒�グ���ꂽ
			if (value > alpha) {

				if (PvNode) {
					update_pv(ss->pv, move, (ss + 1)->pv);
				}

				if (PvNode && value < beta)
				{
					//������alpha�l�𒴂��̏���������B
					alpha = value;
					bestMove = move;
				}
				else
				{
					//PVnode�ȊO�ł�null window search�Ȃ̂�alpha�����Ƃ�beta�����̂��Ƃł���

					return value;
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




#endif