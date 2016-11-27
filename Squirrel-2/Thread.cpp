#include "Thread.h"
#include <vector>
#include "makemove.h"
#include "search.h"
#include <algorithm>

using namespace std;

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
	cout << "movenum " << j << endl;
	end = RootMoves + j;

	rootpos.set_searcherthread(this);


}

void Thread::sort_RootMove()
{
	sort(RootMoves, end, [](const ExtMove& m1, const ExtMove& m2) { return m1.value > m2.value; });
}


void Thread::print_pv(const int depth, Stack* ss)
{
	TimePoint elapsed = (now() - limit.starttime + 1);//ƒ[ƒ‚Å‚í‚ç“à—e‚É‚·‚é‚½‚ß‚É‚P‚ğ‘«‚µ‚Ä‚¨‚­

	std::cout << "info depth " << depth <<"/"<<seldepth<< " score cp " << int(RootMoves[0].value*100/Eval::PawnValue) << " time " << elapsed << " nodes " << rootpos.searched_nodes()
		<< " nps " << rootpos.searched_nodes() / uint64_t(elapsed) << "k";

	std::cout << " pv ";

	for (Move m : pv) {
		cout << " " << m;
	}
	std::cout << endl;

}