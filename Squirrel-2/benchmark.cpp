#include "benchmark.h"
#include "makemove.h"
#include "misc.h"


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
	}
	std::cout << std::endl;


}
