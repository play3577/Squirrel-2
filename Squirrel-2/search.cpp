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
void update_stats(const Position& pos, Stack* ss, Move move, Move* quiets, int quietsCnt, Value bonus);


template <Nodetype NT>Value search(Position &pos, Stack* ss, Value alpha, Value beta, Depth depth);

template <Nodetype NT>
Value qsearch(Position& pos, Stack* ss, Value alpha, Value beta, Depth depth);



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
int Reductions[3][2][64][64];  // [pv][improving][depth][moveNumber]

template <bool PvNode> Depth reduction(bool i, Depth d, int mn) {
	return Reductions[PvNode][i][std::min(int(d / ONE_PLY), 63)][std::min(mn, 63)] * ONE_PLY;
}

//�T���搔�̏�����
//�l�������Ɛ^���Ɍ��Ă���
void search_init() {

	for (int imp = 0; imp <= 1; ++imp) {
		for (int d = 1; d < 64; ++d) {
			for (int mc = 1; mc < 64; ++mc)
			{
				double r = log(d) * log(mc) / 2;
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
	TimePoint now_ = now();
	if (now_  > limit.endtime -20) {
		signal.stop = true;
	}
}







Value Thread::think() {

	Stack stack[MAX_PLY + 7], *ss = stack + 5;
	std::memset(stack, 0, (MAX_PLY + 7) * sizeof(Stack));
	Value bestvalue, alpha, beta;

	if (end == RootMoves) {
#ifndef LEARN
		cout << "bestmove resign" << endl;
#endif // !LEARN
		return Value_Mated;
	}

	Move pondermove;

	//������book��T���R�[�h������
	if (Options["usebook"]) {

		auto bookentry = book.find(rootpos.make_sfen());

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
			goto ID_END;
		}//book���������ꍇ

		
	}//end book

	history.clear();
#ifdef USETT
	TT.new_search();
#endif
	
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

ID_END:

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
	Move bestMove=MOVE_NONE;
	Move excludedmove=MOVE_NONE;
	Move Quiets_Moves[64];
	int quiets_count=0;
	Value value,null_value;
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

	//ss�͉ߋ��̏��������Ă����K�v������
	(ss + 1)->excludedMove = MOVE_NONE;
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
		//ttMove = tte->move();
	}
	else {
		ttValue = Value_error;
		ttdepth = DEPTH_NONE;
		ttEval = Value_error;
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
		//&& (pos.piece_on(move_to(ttMove)) == NO_PIECE || piece_color(pos.piece_on(move_to(ttMove))) != pos.sidetomove())//�����ł��̋ǖʂŔ񍇖@�肾������Ȃ�
		) {

		//ttMove��quiet��ttvalue>=beta�ł����history���X�V���邱�Ƃ��ł���B
		if (ttValue >= beta&&ttMove != MOVE_NONE) {
			//hash�̋��R��v�ł����Ńo�O���ė����邱�Ƃ��N���������A���A�P�[�X�ł��邽�ߖ��������邱�Ƃɂ���

			/*if (pos.piece_on(move_to(ttMove)) != NO_PIECE && piece_color(pos.piece_on(move_to(ttMove))) == pos.sidetomove()) {

				ASSERT(0);
			}*/
			if (pos.capture_or_propawn(ttMove)==false) {
				update_stats(pos, ss, ttMove, nullptr, 0, bonus(depth));
			}
		}

		return ttValue;
	}
#endif


	//�]���֐��͖���Ăяo�����ق��������v�Z�ł���
	//����]���֐����Ăяo���̂�SF�̂悤��TT��eval�Ƃǂ��炪�M�p�������邩��r����K�v�͂Ȃ��Ǝv���B
	//�����Ȃ��tt->eval()�͕ۑ�����K�v��������...???���`�`�������������tt.eval()��p�����ق��������b�g���L��̂�������Ȃ�...�i�����͌�ł悭�l���悤�j
	ss->static_eval=staticeval = Eval::eval(pos);

	//�����ɑO�����}�؂�̃R�[�h������

	//���肪�������Ă���ꍇ�͑O�����}�؂�͂��Ȃ�
	//�i�����O�����}�؂肪�o���Ă��܂����炱�̃m�[�h�ŋl��ł��܂��j
	if (incheck) {
		//	cout << "incheck" << endl;
		goto moves_loop;
	}
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
		return staticeval - futility_margin(depth);
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
		ASSERT(staticeval >= beta);

		//R�̒l�͏ꍇ���������Č�ŏڂ�������
		Depth R= Depth(((823 + 67 * int(depth / ONE_PLY)) / 256 + std::min(int(staticeval - beta) / Eval::PawnValue, 3)) * ONE_PLY);

		pos.do_nullmove(&si);
		(ss + 1)->skip_early_prunning = true;//�O�����}�؂�͂��Ȃ��B�i�Q��A���p�X�͂�낵���Ȃ��j

		//nullmove�͎�荇�����N����Ȃ��̂ŐÎ~�T�����Ă΂Ȃ��B�i�Î~�T���ŉ�����T������悤�ɐ���Ύ�����ύX����j
		null_value = (depth - R) < ONE_PLY ? staticeval : -search<NonPV>(pos, ss + 1, -(beta), -beta + 1, depth - R);
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
			Value v= (depth - R) < ONE_PLY ? staticeval : search<NonPV>(pos, ss ,beta-1, beta, depth - R);
			ss->skip_early_prunning = false;

			if (v >= beta) {
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

				pos.do_move(move, &si);
				value = -search<NonPV>(pos, (ss + 1), -rbeta, -rbeta + 1, rdepth);
				pos.undo_move();

				if (value >= rbeta) { return value; }
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
		search<NT>(pos, ss, alpha, beta, d);
		ss->skip_early_prunning = false;
		tte = TT.probe(poskey, TThit);
		//TT��������Ă��܂��ꍇ���l���Ă�������
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



	//���肪�������Ă���ꍇ�͑O�����}�؂�͂��Ȃ�
	//�i�����O�����}�؂肪�o���Ă��܂����炱�̃m�[�h�ŋl��ł��܂��j
moves_loop:
	

//#define EXTENSION
		
#ifdef EXTENSION
			//�ÓI�]���l���O�̎����̎�Ԃ����悭�Ȃ��Ă��邩�܂��͈ȑO�̕]���l�����݂��Ȃ�����
	improve = ss->static_eval >= (ss - 2)->static_eval || (ss - 2)->static_eval == Value_error;
	
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
		&&excludedmove != MOVE_NONE
		&& (ttBound&BOUND_LOWER)//LOWER or EXACT
		 && ttdepth >= depth - 3 * ONE_PLY;
#endif
		
		




#ifdef USETT
	movepicker mp(pos,ss,ttMove);
#else 
	movepicker mp(pos, ss, MOVE_NONE);
#endif
	while ((move = mp.return_nextmove()) != MOVE_NONE) {

		if (move == ss->excludedMove) {continue;}
		if (!RootNode) {
			if (pos.is_legal(move) == false) { continue; }
		}
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

		//���ꑁ���Ȃ�񂾂낤���H�H
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif

		/*
		capture propawn�̎w����ɂȂ肤��̂�cappropawn�̃X�e�[�W��EVERSION�̃X�e�[�W����
		*/
		Stage st = mp.ret_stage();
		if (st == CAP_PRO_PAWN||st==BAD_CAPTURES) {
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

		++movecount;



		//check_move(move);
		/*bool givescheck=pos.is_gives_check(move);
		pos.do_move(move, &si,givescheck);*/

		pos.do_move(move, &si);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif
		doFullDepthSearch = (PVNode&&movecount == 1);


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
		//�s��ɋ�Ȃ��܂��͎����̋�ł͂Ȃ�
		/*if (pos.piece_on(move_to(bestMove)) != NO_PIECE && piece_color(pos.piece_on(move_to(bestMove))) == pos.sidetomove()) {
			ASSERT(0);
		}*/
		if (!pos.capture_or_propawn(bestMove)) {
			update_stats(pos, ss, bestMove, Quiets_Moves, quiets_count, bonus(depth));
		}
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
	//Value futilityvalue;
	if (PvNode)
	{
		// To flag BOUND_EXACT when eval above alpha and no available moves
		//�]���l��alpha�����A�����@�肪�Ȃ��ꍇ��flag�Ƃ��ėp����B
		oldAlpha = alpha; 
		(ss + 1)->pv = pv;
		ss->pv[0] = MOVE_NONE;
	}
	//���Ԓʂ�Ɏw������w���Ă���Ȃ������̂Ő��q�T���ł����Ԃ�����
	//��荇���̓r���ŕ]����Ԃ����Ǝ��ɂ܂����̂łł��邾���������Ȃ��̂���....
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
		return ttValue;
	}
#endif



	//�����ɑO�����}�؂�̃R�[�h������


	//�Î~�T���͋�̎�荇���̂悤�ȕ]���l���s����ȋǖʂ�扄�΂��ĒT���𑱂��邱�ƂŒT���̖��[�]���l��␳���A��萳�m�ɂ�����̂ł���Ǝv���Ă���B
	bestvalue=staticeval = Eval::eval(pos);

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

	
#ifdef LEARN
	//�w�K���͐��q�T���[���R�܂łŎ~�߂�B�i����Ȃ��Ƃ͂����o�b�T���}��؂��}�؂���@�𓱓��������j
	if (depth < -3 * ONE_PLY) { return bestvalue; }
#endif
	//�R���ŏ�肭�O�̎w����̈ړ����^�����Ă���Ǝv��
	//������nullmove�������Ă����ꍇ�̂��Ƃ��l���Ȃ��Ƃ����Ȃ��B
	//�Ƃ�����nullmove�������Ă�����惊�Ԃ��Ȃ�Ă��肦�Ȃ��̂ł����ŕ]���l�Ԃ����������ł���(�������������Ȃ�b�͕ʁj
#ifdef USETT
	movepicker mp(pos, move_to(pos.state()->lastmove),ttMove);
#else
	movepicker mp(pos, move_to(pos.state()->lastmove), MOVE_NONE);
#endif

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
		
		

		//�キ�Ȃ���
		//futility 
		//if (!incheck
		//	//&&!givescheck  ��������Ƃ��ē����ׂ����Ǝv������Ǎ���domove�̎d�l�ł͂Ȃ��Ȃ�����B
		//	&&futilitybase > -Value_known_win
		//	) {
		//	futilityvalue = futilitybase + Value(Eval::piece_value[pos.piece_on(move_to(move))]);
		//	//��荇���Ȃ̂ň��ڂ��Ƃ�������alpha-128�𒴂����Ȃ��悤�ł���΂��̎w������l���Ȃ�
		//	if (futilityvalue <= alpha) {
		//		bestvalue = std::max(bestvalue, futilityvalue);
		//		continue;
		//	}
		//}

		//���ꑁ���Ȃ�񂾂낤���H�H
#ifdef PREFETCH
		TT.prefetch(pos.key_after_move(move));
#endif


		movecount++;
		pos.do_move(move, &si);
#ifdef PREF2
		TT.prefetch(pos.key());
#endif
		/*bool givescheck = pos.is_gives_check(move);
		pos.do_move(move, &si, givescheck);*/
		value = -qsearch<NT>(pos, ss + 1, -beta, -alpha, depth - ONE_PLY);
		pos.undo_move();
#ifndef LEARN
		if (signal.stop.load(std::memory_order_relaxed)) {
			//���Ԑ؂��fail hard�ɂ��Ă��邪������fail soft�ɂ��Ă����ׂ����H
			return alpha;
		}
#endif
		if (value > bestvalue) {
			//��荇����ǂ񂾕��]���l�͂�萳�m�ɂȂ���
			bestvalue = value;

			//alpha�l�͒�グ���ꂽ
			if (value > alpha) {
				
				if (PvNode && value < beta) 
				{
					//������alpha�l�𒴂��̏���������B
					alpha = value;
					bestMove = move;
					update_pv(ss->pv, move, (ss + 1)->pv);
				}
				else 
				{
					//PVnode�ȊO�ł�null window search�Ȃ̂�alpha�����Ƃ�beta�����̂��Ƃł���
#ifdef USETT
					//value��beta�𒴂����Ƃ������Ƃ͂��̓_���ȏ�̎w���肪�܂����邩������Ȃ��Ƃ������ƂȂ̂�BOUND_LOWER
					tte->save(posKey, value_to_tt(value, ss->ply), BOUND_LOWER,
						depth, move, staticeval, TT.generation());
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
		depth, bestMove, staticeval, TT.generation());
#endif

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
	Thread* thisthread = pos.searcher();
	thisthread->history.update(moved_piece(bestmove), move_to(bestmove), bonus);

	// Decrease all the other played quiet moves
	//alpha���X�V���Ȃ������w����ɑ΂���+=���̒l
	for (int i = 0; i < quietsCnt; ++i) {
		thisthread->history.update(moved_piece(quiets[i]), move_to(quiets[i]), -bonus);
	}

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