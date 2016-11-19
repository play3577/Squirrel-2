#include "benchmark.h"
#include "makemove.h"
#include "evaluate.h"
#include "misc.h"
#include <random>

//�w���萶�����x�v��
//�v�����@�����̃\�t�g�ƈႤ�Ɣ�r�ł��Ȃ��̂ő��̃\�t�g�ɍ��킹��B
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



//�����ł�depth�͎c��[���ł͂Ȃ��ĉ��Z���Ă���depth
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
		

		//�w����̐���
		end = test_move_generation(pos, moves_);
		Eval::eval(pos);
		ptrdiff_t num = end - moves_;
		if (num == 0) { break; }
		if (num == continue_count) { cout << "mated" << endl;  break; }
		//cout << num << endl;
		int rand = mt() % num;
		ASSERT(rand <= num);
		Move m = moves_[mt() % num];
		//if (pos.piece_on(move_to(m)) != NO_PIECE) { cout << "capture" << endl; }
		

		if (pos.is_legal(m) == false) { ++continue_count;  continue;  }//���ׂĂ̍����肪illegal�܂�l�݂̏�ԂɂȂ�΂���ł͂��������Ȃ��Ă��܂��̂ŏC��
		else { continue_count = 0; }
		//��񂿂���do-undo�ł��邩�m�F�����Ă���
		pos.do_move(m, &si[depth]);
		pos.undo_move();

		pos.do_move(m, &si[depth]);
		depth++;
		if (depth > maxdepth) {
			break;
		}
			
		
	}//�w�����while
}
