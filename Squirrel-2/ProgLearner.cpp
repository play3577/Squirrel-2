#define _CRT_SECURE_NO_WARNINGS 1
#include "progress.h"
#include "misc.h"
#include "game_database.h"

#include <random>
#include <iostream>
#include <fstream>
#include <omp.h>
#include <mutex>

#ifdef Prog_LEARN
namespace Progress {

	using namespace Eval;

	struct  ProgdJ
	{
		double dJ[SQ_NUM][fe_end];

		void update_dJ(const Position& pos, const double diff) {
			const auto list1 = pos.evallist();

			const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
			const Square bksq = pos.ksq(BLACK), wksq = hihumin_eye(pos.ksq(WHITE));
			for (int j = 0; j < 38; j++) {
				dJ[bksq][list_fb[j]] += diff;
				dJ[wksq][list_fw[j]] += diff;
			}			
		}

		void clear() { memset(this, 0, sizeof(*this)); }

		void add(ProgdJ& data) {
			for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
				for (BonaPiece bp = BONA_PIECE_ZERO; bp < fe_end; bp++) {
					dJ[sq][bp] += data.dJ[sq][bp];
				}
			}
		}
	};

	ProgdJ sum_dJs;
	std::vector<ProgdJ> dJs;
	std::vector<Game> games;
	std::vector<Game> testset;

	int maxthreadnum;
	int numgames;
	int num_parse2;
	double objective_function = 0;


	void learn_body(const int index);
	void renewalPP(ProgdJ& data);
	std::mutex mutex_;
	int index_ = 0;

	int lock_index_inclement() {
		std::unique_lock<std::mutex> lock(mutex_);
		if (index_ > numgames) { cout << "o" << endl; }
		else { printf("."); }

		return index_++;
	}



	void learner()
	{
		int readgames = 30000;
		int numtestset = 500;
		maxthreadnum = omp_get_max_threads();

		numgames = 100;
		int numiteration = 1000;

		cout << "readgames " << readgames << endl;

		cout << "numgames:>>";
		cin >> numgames;
		cout << "numiteration?>>";
		cin >> numiteration;

		ifstream gamedata(gamedatabasefile);
		GameDataStream gamedatastream(gamedata);
		//棋譜読み込み
		for (int i = 0; i < readgames; ++i) {
			Game game;

			if (gamedatastream.read_onegame(&game)
				&& game.ply >= 60
				&& game.result != draw)
			{
				games.push_back(game);
			}
		}
		for (int i = 0; i < numtestset; ++i) {
			Game game;

			if (gamedatastream.read_onegame(&game)
			&& game.ply >= 60
			&& game.result != draw)
			{
				testset.push_back(game);
			}
		}

		cout << "teacher games " << games.size() << endl;
		cout << "test games " << testset.size() << endl;
		cout << "read kihu OK!" << endl;


		//ログファイル開く
		std::string str, filepath;
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		str = asctime(timeinfo);

		size_t hoge;
		while ((hoge = str.find_first_of(" ")) != string::npos) {
			str.erase(hoge, 1);
		}
		while ((hoge = str.find_first_of("\n")) != string::npos) {
			str.erase(hoge, 1);
		}
		while ((hoge = str.find_first_of(":")) != string::npos) {
			str.erase(hoge, 1);
		}

		filepath = "c:/book2/log/" + str+"prog" + ".txt";
		ofstream ofs(filepath);
		if (ofs.fail()) { cout << "log fileError"; }


		//dJをスレッド分だけ用意する
		for (size_t i = 0; i < maxthreadnum; i++) {
			dJs.emplace_back();
		}


		//iteration開始。
		for (int iteration = 0; iteration < numiteration; iteration++) {
			//時間計測の開始
			cout << "iteration" << iteration << endl;
			limit.starttime = now();

			objective_function = 0;
			//maxmoves = 0;
			//huicchi_moves = 0;
			sum_dJs.clear();
			for (auto& datas : dJs) {
				datas.clear();
			}

			std::random_device rd;
			std::mt19937 g_mt(rd());
			std::shuffle(games.begin(), games.end(), g_mt);//棋譜のシャッフル

			vector<std::thread> threads(maxthreadnum - 1);//maxthreadnum-1だけstd::threadをたてる。
			index_ = 0;

			//並列してgradを計算する。
			for (int i = 0; i < maxthreadnum - 1; ++i) {
				threads[i] = std::thread([i] {learn_body(i); });
			}
			learn_body(maxthreadnum - 1);

			//gradをたしあげる
			for (auto& dj : dJs) {
				sum_dJs.add(dj);
			}
			for (auto& th : threads) { th.join(); }



			renewalPP(sum_dJs);

			write_KP();
			read_KP();

			//出力
			std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min ";
			ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
		}
		
		cout << "finish learning progress" << endl;

	}



	void learn_body(const int index) {
		Position pos;
		StateInfo si[500];
		Thread th;

		int didmoves = 0;
		int diddepth = 0;

		for (int g = lock_index_inclement(); g < numgames; g = lock_index_inclement()) {
			didmoves = 0;
			auto thisgame = games[g];
			pos.set_hirate();

			for (int ply = 0, n = thisgame.moves.size(); ply < n; ++ply) {
				diddepth = ply;

				double teacher = double(ply) / double(n);//教師データは現在の手数/試合終了手数！！
				double actual = calc_prog(pos);//現在の進行度計算
				double diff = actual - teacher;//差分を計算(actualがteacherより大きかった場合（diff>0）はactualを引き下げる. )(actualがteacherよりちいさかった場合（diff<0）はactualを引きあげる. )
				objective_function += diff;
				const Move teacher_move = thisgame.moves[ply];

				dJs[index].update_dJ(pos, diff*dsigmoid(actual));
				

				if (pos.is_legal(teacher_move) == false || pos.pseudo_legal(teacher_move) == false) { goto NEXT_GAME; }
				pos.do_move(teacher_move, &si[ply]);
			}
NEXT_GAME:;
		}



	}



	constexpr double row=0.95, epsiron = 0.0001;
	double lastEg[SQ_NUM][fe_end] = { 0.0 }, lastdelta_x[SQ_NUM][fe_end] = { 0.0 };
	void renewalPP(ProgdJ& data) {

		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
			for (BonaPiece bp = BONA_PIECE_ZERO; bp < fe_end; bp++) {
				//dJの方向にパラメーター動かす
				/*
				しかしどれだけ動かすのが最適なんだろう？？
				bonanza methodのようにあんまり大きく動かすとPVが変化してしまうのでちまちまうごかすような方法は必要ないと思うし...
				dJの大きさによって動かすのをおおきくするのも底がなだらかに傾いている谷の場合に良くない気がするし...
				う～～む adadeltaを使うか...
				*/
				double gt = data.dJ[sq][bp];
				/*double gt2 = gt*gt;
				double Egt=lastEg[sq][bp] = (row)*lastEg[sq][bp] + (1-row)*gt2;
				double delta_x = -sqrt(lastdelta_x[sq][bp] + epsiron) / sqrt(Egt + epsiron);
				lastdelta_x[sq][bp] = (row)*lastdelta_x[sq][bp] + (1-row)*delta_x;

				prog_KP[sq][bp] += sign(delta_x);*/
				prog_KP[sq][bp] += -sign(gt);
			}
		}
	}

}
#endif