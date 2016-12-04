#include "Thread.h"
#include <vector>
#include "makemove.h"
#include "search.h"
#include <algorithm>
#include <sstream>
#include "evaluate.h"
using namespace std;



const string print_value(Value v) {

	stringstream ss;

	if (v < Value_mated_in_maxply) {
		ss << "mated " << int(Value_Mated  - v) ;
	}
	else if (v > Value_mate_in_maxply) {
		ss << "mate " << int(Value_Mate-v) ;
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
	//root‚É”ñ‡–@è‚ª“ü‚Á‚Ä‚­‚é‚Ì‚ğ–h‚®


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
	sort(RootMoves, end, [](const ExtMove& m1, const ExtMove& m2) { return m1.value > m2.value; });
}


 void Thread::print_pv(const int depth,Value v)
{
	TimePoint elapsed = (now() - limit.starttime + 1);//ƒ[ƒ‚Å‚í‚ç“à—e‚É‚·‚é‚½‚ß‚É‚P‚ğ‘«‚µ‚Ä‚¨‚­

	std::cout << "info depth " << depth <<"/"<<seldepth<< " score cp " <<print_value(v) << " time " << elapsed << " nodes " << rootpos.searched_nodes()
		<< " nps " << rootpos.searched_nodes() / uint64_t(elapsed) << "k";

	std::cout << " pv ";

	for (Move m : pv) {
		cout << " " << m;
	}
	std::cout << endl;

}