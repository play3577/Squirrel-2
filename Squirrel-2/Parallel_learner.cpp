#define _CRT_SECURE_NO_WARNINGS




#include "learner.h"
#ifdef LEARN
#include <random>
#include <vector>
#include <time.h>       /* time_t, struct tm, time, localtime */
#include <fstream>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
#include "usi.h"
#include <omp.h>
#include <thread>
#include <mutex>
using namespace Eval;
using namespace std;



#define LOG

//struct  Parse2data;

struct  dJValue
{
	double dJ[fe_end2][fe_end2];

	void update_dJ(const Position& pos, const double diff) {
		const auto list1 = pos.evallist();

		const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				dJ[list_fb[i]][list_fb[j]] += diff;
				dJ[list_fw[i]][list_fw[j]] -= diff;
				//PP対称性を考えて
				dJ[list_fb[j]][list_fb[i]] += diff;
				dJ[list_fw[j]][list_fw[i]] -= diff;
			}
		}
	}
	void clear() { memset(this, 0, sizeof(*this));}


	void add(dJValue& data) {

		for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
			for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
				dJ[bp1][bp2] += data.dJ[bp1][bp2];
			}
		}

	}
};




struct Parse2Data {
	
	dJValue gradJ;
	void clear() {
		gradJ.clear();
	}
};


void renewal_PP(dJValue &data) {

	std::random_device rd;
	std::mt19937 mt(rd());

	int h;

	//こんなんでいいのか？
	h = std::abs(int(mt())) % 3;

	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			if (PP[i][j]>0) { data.dJ[i][j] -= double(0.2 / double(FV_SCALE)); }
			else if (PP[i][j]<0) { data.dJ[i][j] += double(0.2 / double(FV_SCALE)); }

			int inc = h*sign(data.dJ[i][j]);
			PP[i][j] += inc;
		}
	}
}

void param_sym_leftright(dJValue &data) {

	//l反転前, r反転語
	BonaPiece il, ir, jl, jr;

	//static bool check[fe_end2*fe_end2 / 2] = {false};
	//t2.microは耐えてくれるだろうか....
	static bool check[fe_end2][fe_end2] = { false };
	memset(check, false, sizeof(check));
	//PPの一つ目のindexについて
	for (il = f_hand_pawn; il < fe_end2; il++) {

		//持ち駒の場合はそのままでいい。（左右対称になんて出来ないから）
		if (il < fe_hand_end) { ir = il; }
		else {
			//盤上の駒は左右を反転させる。
			ir = Eval::sym_rightleft(il);
		}

		//PPの二つ目のindexについて
		//for (jl = f_hand_pawn; jl <= il; jl++) {
		for (jl = f_hand_pawn; jl <fe_end2; jl++) {

			//持ち駒の場合はそのままでいい。（左右対称になんて出来ないから）
			if (jl < fe_hand_end) { jr = jl; }
			else {
				//盤上の駒は左右を反転させる。
				jr = Eval::sym_rightleft(jl);
			}

			if ((il == ir) && (jl == jr)) { continue; }//反転させてもおんなじなのでコレは対称性を考える意味はない（両方５筋の駒||両方手駒）
			if (check[il][jl] == true) { ASSERT(check[ir][jr] == true); continue; }
			//if(check[il*(il+1)/2+jl]==true){ ASSERT(check[ir*(ir + 1) / 2 + jr] == true); continue; }
			//同じ関係なので２つ分のdJ/dviを用いることができる！！
			/*
			ここで(iljl,irjr)と(irjr,iljl)でdJを２重に計算してしまい、値がおかしくなってしまうことが起こりうる。
			コレを何とかして防がなければならない。三角テーブル対称性が壊れてしまった
			*/

			/*
			for (il = f_hand_pawn; il < fe_end2; il++) なので
			このままでは
			dJ[il][jl] = dJ[ir][jr]= (dJ[il][jl] + dJ[ir][jr]);をした後さらに
			dJ[ir][jr] = dJ[il][jl]= (dJ[il][jl] + dJ[ir][jr]);をしてdJ[][]の値がおかしくなってしまうことが起こりうる。

			これを防ぐためにはsym_rightleft()でsqが盤面の右側であればreturn -1をすれば良いかもしれないが,左右反転させるだけの関数にそのような機能をもたせるのはあんまりしたくない

			bool check[fe_end2][fe_end2]で一度計算した組み合わせかどうか確認するようにさせる。
			この方法ではcheckを確保することができなかった。（staticにして無理やり解決。）
			*/

			data.dJ[il][jl] = data.dJ[ir][jr] = (data.dJ[il][jl] + data.dJ[ir][jr]);
			//check[il*(il + 1) / 2 + jl] = check[ir*(ir + 1) / 2 + jr] = true;
			check[il][jl] = check[ir][jr] = true;

		}
	}
}





Parse2Data sum_parse2Datas;
std::vector<Parse2Data> parse2Datas;
std::vector<Game> games;



//目的関数。コレが小さくなってくれば学習が進んでいるとみなせる。(はずっ！)
/*
学習が進むにつれて教師手の価値がほかの差し手よりも大きくなるので目的関数は大きくなっていく。
この値がほぼ変わらなくても強くなることはあるのでこの値は全く参考にならない
*/
double objective_function = 0;

int maxthreadnum;
int numgames;
int num_parse2;
void Eval::parallel_learner() {

	//初期化
	int readgames = 100000;
	numgames = 2000;
	int numiteration = 1000;
	maxthreadnum = omp_get_max_threads();

	cout << "numgames:>>";
	cin >> numgames;
	cout << "numiteration?>>";
	cin >> numiteration;

	/*bonanzaではparse2を３２回繰り返すらしいんでそれを参考にする。
	学習の損失の現象が進むに連れてnum_parse2の値を減らしていく
	*/
	num_parse2 = 32;
	ifstream gamedata(fg2800_2ch);
	//ifstream gamedata(nichkihu);
	GameDataStream gamedatastream(gamedata);
	

	


	//棋譜を読み込む
	for (int i = 0; i < readgames; ++i) {
		Game game;

		if (gamedatastream.read_onegame(&game)
			&& game.ply >= 60
			&& game.result != draw)
		{
			games.push_back(game);
		}
	}

	cout << "read kihu OK!" << endl;


#ifdef LOG
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

#if defined(_MSC_VER)
	filepath = "c:/book2/log/" + str + ".txt";
#endif
#if defined(__unix__) 
	filepath = "/home/daruma/fvPP/" + str + ".txt";
#endif
	//filepath = "c:/book2/log/" + str + ".txt";
	ofstream ofs(filepath);
	if (ofs.fail()) { cout << "log fileError"; }

#endif
	
	//vector<Thread> threads(maxthreadnum-1);
	//siやmoves,positionは並列させる関数内で生成する

	for (size_t i = 0; i < maxthreadnum; i++) {
		Parse2Data piyo;
		parse2Datas.push_back(piyo);
	}

	for (int iteration = 0; iteration < numiteration; iteration++) {
		//時間計測の開始
		limit.starttime = now();
		//過去の情報をクリアする
		objective_function = 0;
		for (auto& datas : parse2Datas) {
			datas.clear();
		}
		//学習
		learnphase1();
		learnphase2();


		std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << std::endl;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
#endif
	}



}


//http://kaworu.jpn.org/cpp/std::mutex
//アクセス競合の防止
//これを用いることで学習を並列に進めることができる(from Apery)
std::mutex mutex_;
int index_ = 0;

int lock_index_inclement() {
	std::unique_lock<std::mutex> lock(mutex_);
	return index_++;
}


void learnphase1() {

	std::random_device rd;
	std::mt19937 g_mt(rd());
	std::shuffle(games.begin(), games.end(), g_mt);
	vector<std::thread> threads(maxthreadnum - 1);//maxthreadnum-1だけstd::threadをたてる。
	index_ = 0;
	//並列学習開始
	for (int i = 0; i < maxthreadnum - 1; ++i) {
		threads[i] = std::thread([i]{learnphase1body(i); });
	}
	learnphase1body(maxthreadnum-1);

	for (auto& th : threads) { th.join(); }

}


/*
lockingindexincrementはどう実装する？？
*/
void learnphase1body(int number) {

	//ここで宣言したいのだけれどこれでいいのだろうか？
	Position pos;
	StateInfo si[256];
	ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;


	int didmoves = 0;
	int diddepth = 0;
	//============================================================
	//ここlockingindexincrementにする必要がある！！
	//=============================================================
	for (int g = lock_index_inclement(); g < numgames; g=lock_index_inclement()){
		didmoves = 0;
		auto thisgame = games[g];
		pos.set_hirate();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {
			diddepth = ply;
			minfo_list.clear();

			const Color rootColor = pos.sidetomove();

			//この局面の差し手を生成する
			memset(moves, 0, sizeof(moves));//初期化
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;


			//棋譜の差し手を指し手リストの一番最初に持ってくる。
			Move teacher_move = thisgame.moves[ply];
			//if (pos.is_legal(teacher_move) == false) { goto ERROR_OCCURED; }
			//合法てリストに教師手が入っていなければここから先おかしくなってしまうのでこの棋譜を使っての学週はここで中断

			//is_okで落ちてくれるのは最後まで利用できたということなのでありがたい
			if (is_ok(teacher_move) == false) { cout << "is not ok" << endl; goto ERROR_OCCURED; }

			if (!swapmove(moves, int(num_moves), teacher_move)) {
				cout << "cant swap" << endl;
				
				goto ERROR_OCCURED;
			}
			if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }

			//差し手に対して探索を行う。探索をしたら探索したPVと指し手と探索した評価値をペアにして格納
			//これは少し制限して一局面最大（15手ぐらいにした方がいいか？）
			//↑局面あたりの指し手の数の制限はすべきではないかもしれない
			//bonanzaでは全合法手に対しておこなっていたのでSquirrelでもやらないことに決めた
			//num_moves = std::min(int(num_moves), 15);
			//cout << "num_moves " << num_moves << endl;

			for (int move_i = 0; move_i < num_moves; move_i++) {

				Move m = moves[move_i];
				if (pos.is_legal(m) == false) { continue; }
				if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				didmoves++;
				pos.do_move(m, &si[ply]);
				th.set(pos);
				Value  score = th.think();
				if (abs(score) < Value_mate_in_maxply) {
					minfo_list.push_back(MoveInfo(m, th.pv, score));
				}
				else { pos.undo_move(); break; }
				pos.undo_move();
			}
			//ここでこの局面の指し手による探索終了
			
			//ここからsum_gradJの更新
			{


				/*
				教師手ではない指してによる局面に現れる特徴ベクトルは価値を低く、
				教師手による局面に現れる特徴ベクトルは価値を高く、
				両方に出てくる特徴ベクトルに対しては価値を動かさないようにしたい。

				そのためのsum_diff。
				どの差し手を指してもつまされることが分かった場合もlistsize==0になる！
				*/
				double sum_diff = Value_Zero;
				if (minfo_list.size() == 0) {
					cout << "listsize==0" << endl;
					/*cout << pos << endl;
					check_move(teacher_move);
					cout << "aa" << endl;*/
					goto ERROR_OCCURED;
				}

				//教師手以外の指してに対して
				for (int i = 1; i < minfo_list.size(); i++) {

					//評価値と教師手の差分を取る。
					const Value diff = minfo_list[i].score - minfo_list[0].score;

					/*
					http://kifuwarabe.warabenture.com/2016/11/23/%E3%81%A9%E3%81%86%E3%81%B6%E3%81%A4%E3%81%97%E3%82%87%E3%81%86%E3%81%8E%E3%82%92%E3%83%97%E3%83%AD%E3%82%B0%E3%83%A9%E3%82%82%E3%81%86%E3%81%9C%E2%98%86%EF%BC%88%EF%BC%92%EF%BC%94%EF%BC%89%E3%80%80/
					シグモイド関数に値を入れるのは教師手の価値と同じぐらいの価値の指し手に対して学習をし、
					悪い手を更に悪く、教師手を更に良く学習させないようにし、
					値が大きく離れたらそのパラメーターをいじったりしないようにし、収束させるためである。

					しかし教師手よりもかなり高く良い手だとコンピューターが誤判断している指し手がシグモイド関数の微分に入ってきてしまった場合それに対する出て来る値も小さくなってしまい、
					値が下げられないのではないか？
					それは良くない気がするのだけれど実際どうなんだろうか？
					*/


					//=======================================================================================================================
					//bonanzaなどでは手番の色によってdiffsigの符号を変えているがそんなことする必要あるのか？？なぜ？？
					//=======================================================================================================================

					/*
					この指し手の価値が教師手よりも大きかったため、価値を下げたい場合を考える。
					黒番の場合は最終的にupdate_dJに入るのは-dsig
					白番の場合は最終的にupdate_dJに入るのはdsigになる。

					詳しい解説
					https://twitter.com/daruma3940/status/801638488130994177
					*/
					double diffsig = dsigmoid(diff);
					diffsig = (rootColor == BLACK ? diffsig : -diffsig);

					sum_diff += diffsig;
					objective_function += sigmoid(diff);


					StateInfo si2[3];//最大でも深さ３
					int j = 0;
					//教師手とPVで局面をすすめる
					//if (pos.is_legal(minfo_list[i].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[i].move, &si[ply]);

					for (Move m : minfo_list[i].pv) {
						if (pos.is_legal(m) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
						pos.do_move(m, &si2[j]);
						j++;
					}
					parse2Datas[number].gradJ.update_dJ(pos, -diffsig);//pvの先端（leaf node）に移動してdJを計算するのがTD-leafということ？？
											 //局面を戻す
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}//end of (教師手以外の指してに対して)


				StateInfo si2[3];//最大でも深さ３
				int j = 0;
				//教師手に対してdJ/dviを更新
				if (pos.is_legal(minfo_list[0].move) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				if (pos.state()->lastmove == minfo_list[0].move) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				pos.do_move(minfo_list[0].move, &si[ply]);
				//pvの指し手が非合法手になっている事がある？？
				for (Move m : minfo_list[0].pv) {
					if (pos.is_legal(m) == false) { cout << games[g].black_P << " " << games[g].white_P;  ASSERT(0); }
					if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					pos.do_move(m, &si2[j]);
					j++;
				}
				parse2Datas[number].gradJ.update_dJ(pos, sum_diff);
				for (int jj = 0; jj < j; jj++) {
					pos.undo_move();
				}
				pos.undo_move();

			}//勾配計算

			 //次の局面へ
			pos.do_move(teacher_move, &si[ply]);
		
		}
	ERROR_OCCURED:;
		cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
	}

}

void learnphase2() {
	sum_parse2Datas.clear();
	//たしあげる
	for (auto& parse2 : parse2Datas) {
		sum_parse2Datas.gradJ.add(parse2.gradJ);
	}

	//左右対称性を考える。
	param_sym_leftright(sum_parse2Datas.gradJ);

	//num_parse2回パラメーターを更新する。コレで-64から+64の範囲内でパラメーターが動くことになる。
	//bonanzaではこの32回の間にdJが罰金項によってどんどんゼロに近づけられている。
	for (int i = 0; i < num_parse2; i++) {
		renewal_PP(sum_parse2Datas.gradJ);
	}

	//書き出し読み込みをここで行って値の更新
	write_PP();
	read_PP();
}

#endif//learn
