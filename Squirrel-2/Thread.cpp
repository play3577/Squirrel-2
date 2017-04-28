#include "Thread.h"
#include <vector>
#include "makemove.h"
#include "search.h"
#include <algorithm>
#include <sstream>
#include "evaluate.h"
#include "tpt.h"
using namespace std;



const string print_value(Value v) {

	stringstream ss;

	if (v < Value_mated_in_maxply) {
		//�������C���Ƃ��Ă���ł����̂��͕s���@�{���I�ȃo�O��������Ȃ������ꂪ����������matedistance�����������Ȃ�̂�...
		ss << "mate " << int(Value_Mated  - v+1) ;
	}
	else if (v > Value_mate_in_maxply) {
		ss << "mate " << int(Value_Mate-v-1) ;
	}
	else {
		ss << int(v)*int(100)/int(Eval::PawnValue);
	}

	return ss.str();
}



void Thread::set(Position pos)
{
	/*end = RootMoves;
	rootpos = pos;
	end = test_move_generation(pos, RootMoves);*/
	//root�ɔ񍇖@�肪�����Ă���̂�h��


	rootpos = pos;
	ExtMove psuedo[600],*pe;
	pe = psuedo;
	pe = test_move_generation(pos, psuedo);
	ptrdiff_t num_move= pe - psuedo;
	int j = 0;
	for (int i = 0; i < num_move; i++) {

		if (pos.is_legal(psuedo[i].move)) {
			RootMoves[j] = psuedo[i];
			j++;
		}
	}
//	cout << "movenum " << j << endl;
	end = RootMoves + j;

	rootpos.set_searcherthread(this);


}

void Thread::sort_RootMove()
{
	std::stable_sort(RootMoves, end, [](const ExtMove& m1, const ExtMove& m2) { return m1.value > m2.value; });
}


 void Thread::print_pv(const int depth,Value v)
{
	TimePoint elapsed = (now() - limit.starttime + 1);//�[���ł����e�ɂ��邽�߂ɂP�𑫂��Ă���

	std::cout << "info depth " << depth <<"/"<<seldepth<< " score cp " <<print_value(v) << " time " << elapsed << " nodes " << rootpos.searched_nodes()
		<< " nps " << rootpos.searched_nodes() / uint64_t(elapsed) << "k"
#ifdef USETT
		<<" hashfull "<<TT.hashfull()
#endif
		;

	std::cout << " pv ";

	for (Move m : pv) {
		cout << " " << m;
	}
	std::cout << endl;

}

 /*
 ���̊֐��͒T�����I������Ƃ���pondermove��������Ȃ������ꍇ�ɌĂяo�����
 �Ⴆ��root��fail high���N���������Ȃǂł���@
 */
#ifdef USETT
 bool Thread::extract_ponder_from_tt(Position& pos) {

	 StateInfo st;
	 bool tthit;
	 if (abs(previousScore) > Value_mate_in_maxply) { return false; }

	 ASSERT(pv.size() == 1);
	 if (!is_ok(pv[0])||pv[0]==MOVE_NONE) { return 0; }

	 //pv�ň��i�߂Ă��̋ǖʂ�ttmove��T��
	 pos.do_move(pv[0], &st);
	 TPTEntry* tte = TT.probe(pos.key(), tthit);

	 if (tthit) {

		 Move m = tte->move();
		 if (!is_ok(m)) { return 0; }
		 if ( pos.pseudo_legal(m)&&pos.is_legal(m)) {
			 pv.push_back(m);
		 }
	 }
	 pos.undo_move();

	 return pv.size() > 1;
 }
#endif



