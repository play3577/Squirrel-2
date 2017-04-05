#include "benchmark.h"
#include "makemove.h"
#include "evaluate.h"
#include "misc.h"
#include <random>
#include "progress.h"
//指し手生成速度計測
//計測方法が他のソフトと違うと比較できないので他のソフトに合わせる。
void speed_genmove(const Position & pos)
{
	cout << pos << endl;

	ExtMove moves_[600];
	ExtMove *end = moves_;
	const int64_t num_gen = 5000000;
	TimePoint start = now();

	if (pos.is_incheck()) {

		for (int i = 0; i < num_gen; i++) {
			end = moves_;
			end = move_eversion(pos, moves_);
		}
	}
	else {

		for (int i = 0; i < num_gen; i++) {
			end = moves_;
			end = move_generation<Cap_Propawn>(pos, moves_);
			end = move_generation<Quiet>(pos, end);
			end = move_generation<Drop>(pos, end);
		}
	}
	const TimePoint elapsed = now() - start;
	std::cout << "elapsed = " << elapsed << " [msec]" << std::endl;
	if (elapsed != 0) {
		std::cout << "times/s = " << num_gen / elapsed << " [k times/sec]" << std::endl;
	}

	const ptrdiff_t count = end - moves_;
	std::cout << "num of moves = " << count << std::endl;
	for (int i = 0; i < count; ++i) {
		std::cout << moves_[i].move << ", ";
		check_move(moves_[i].move);
	}
	std::cout << std::endl;


}

void wrap_randomwalker()
{
	Position pos;

	int num_played = 0;


	while (num_played < 10000) {
		pos.set_hirate();
		randomwalker(pos, 499);
		num_played++;
		if (num_played % 100 == 0) {
			cout << "," ;
		}
	}
	cout << endl << "finished"<<endl;
}



//ここでのdepthは残り深さではなくて加算していくdepth
void randomwalker(Position & pos, int maxdepth)
{
	ExtMove moves_[600], *end;
	end = moves_;
	std::random_device rd;
	std::mt19937 mt(rd());
	int depth = 0;
	StateInfo si[500];
	int continue_count = 0;
	while (1) {
		

		//指し手の生成
		end = test_move_generation(pos, moves_);
		Eval::eval(pos);
		ptrdiff_t num = end - moves_;
		if (num == 0) { break; }
		if (num == continue_count) { /*cout << "mated" << endl;*/  break; }
		//cout << num << endl;
		int rand = mt() % num;
		ASSERT(rand <= num);
		Move m = moves_[mt() % num];
		//cout<<"see"<<pos.see(m)<<endl;
		//if (pos.piece_on(move_to(m)) != NO_PIECE) { cout << "capture" << endl; }
		
		Key k = pos.key_after_move(m);




		if (pos.is_legal(m) == false) { ++continue_count;  continue;  }//すべての差し手がillegalつまり詰みの状態になればこれではおかしくなってしまうので修正
		else { continue_count = 0; }
		//一回ちゃんとdo-undoできるか確認をしてから
		//Key hash1 = pos.key();
		//pos.do_move(m, &si[depth]);
		bool givescheck = pos.is_gives_check(m);
		pos.do_move(m, &si[depth], givescheck);
		Key hash3 = pos.key();
		pos.check_bplist();
		pos.evallist().list_check();
		pos.undo_move();
		pos.check_bplist();
		Progress::calc_diff_prog(pos);
		pos.mate1ply();
		//Key hash2 = pos.key();
		ASSERT(pos.evallist().bplist_fb[39] >= Eval::fe_end);
		ASSERT(pos.evallist().bplist_fb[38] >= Eval::fe_end);
		//ASSERT(k == hash3);
		if (k != hash3) {

			cout << pos << endl;
			check_move(m);

			ASSERT(0);

		}
		//Value v = pos.see_sign(m);
		bool v_ge = pos.see_ge(m,Value(0));

		/*if (v >= 0) { if (!v_ge) { cout << pos << endl; cout << m << endl; }; }
		else { if (v_ge) { cout << pos << endl; cout << m << endl; }; }*/

		pos.do_move(m, &si[depth]);
		depth++;
		if (depth > maxdepth) {
			break;
		}
			
		
	}//指し手のwhile
}
