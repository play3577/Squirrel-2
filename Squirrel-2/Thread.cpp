#include "Thread.h"
#include <vector>
#include "makemove.h"
#include "search.h"
#include <algorithm>

using namespace std;

void Thread::set(Position pos)
{
	end = RootMoves;
	rootpos = pos;
	end = test_move_generation(pos, RootMoves);
	rootpos.set_searcherthread(this);


}

void Thread::sort_RootMove()
{
	sort(RootMoves, end, [](const ExtMove& m1, const ExtMove& m2) { return m1.value > m2.value; });
}


void Thread::print_pv(const int depth, Stack* ss)
{
	TimePoint elapsed = (now() - limit.starttime + 1);//ƒ[ƒ‚Å‚í‚ç“à—e‚É‚·‚é‚½‚ß‚É‚P‚ğ‘«‚µ‚Ä‚¨‚­

	std::cout << "info depth " << depth << " score cp " << int(RootMoves[0].value*100/Eval::PawnValue) << " time " << elapsed << " nodes " << rootpos.searched_nodes()
		<< " nps " << rootpos.searched_nodes() / uint64_t(elapsed) << "k";

	std::cout << " pv ";

	for (Move m : pv) {
		cout << " " << m;
	}
	std::cout << endl;

}