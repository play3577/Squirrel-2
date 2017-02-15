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
#include <cstdio>
using namespace Eval;
using namespace std;

//#define test_learn

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



/*
次元下げされた要素。
PP（p0,p1）=PP絶対(左右対称、手番対称)+PP相対(平行移動、手番対称)＋P絶対（左右対称、手番対象）
*/
struct lowerDimPP
{
	int32_t absolute_pp[fe_end2][fe_end2];//絶対PP　左右対称にするためにi0は盤の右半分だけを示すようにさせる。
	int32_t relative_pp[PC_ALL][PC_ALL][17][17];//相対PP
	int32_t absolute_p[fe_end2];//絶対P 左右対称にするためにi0は盤の右半分だけを示すようにさせる。
};
lowerDimPP lowdimPP;

void lower__dimPP(lowerDimPP& lowdim,const dJValue& gradJ);
void weave_lowdim_to_gradj(dJValue& newgradJ, const lowerDimPP& lowdim);

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

			/*if (PP[i][j]>0) { data.dJ[i][j] -= double(0.2 / double(FV_SCALE)); }
			else if (PP[i][j]<0) { data.dJ[i][j] += double(0.2 / double(FV_SCALE)); }*/

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






//
//int64_t maxmoves=0;
//std::mutex mutex_m;
//std::mutex mutex_h;
//int64_t huicchi_moves = 0;
//
//
//void lock_maxmoves_inclement(int moves) {
//	std::unique_lock<std::mutex> lock(mutex_m);
//	maxmoves+=moves;
//}
//
//void lock_huicchimoves_inclement() {
//	std::unique_lock<std::mutex> lock(mutex_h);
//	 huicchi_moves ++;
//}



Parse2Data sum_parse2Datas;
std::vector<Parse2Data> parse2Datas;
std::vector<Game> games;
std::vector<Game> testset;


/*
学習中に一致率を計算させていたがなんかバグってcpu使用率が０になってしまうことが頻発したので
一致率計算の関数を用意して最後に一致率を計算する。

どこにバグがある？？？？？？？？？？


学習用の寄付で一致率を図る。学習を進めると過学習を起こすはずであるのでそれで学習にバグがないかどうか確かめる

*/
double concordance() {
	/*std::random_device rd;
	std::mt19937 t_mt(rd());*/
	//std::shuffle(testset.begin(), testset.end(), t_mt);//シャッフルさせてるがしないほうがいい？？

	int num_tests = 1000;//1000棋譜で確認する
	/*if (num_tests > testset.size()) { num_tests = testset.size(); }
	ASSERT(num_tests <= testset.size());*/
	if (num_tests > games.size()) { num_tests = games.size(); }
	ASSERT(num_tests <= games.size());

	//ここで宣言したいのだけれどこれでいいのだろうか？
	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;

	int64_t num_concordance_move = 0;
	int64_t num_all_move = 0;

	for (int g = 0; g < num_tests; g++) {

		//auto thisgame = testset[g];//ここ学習用のデータと区別しておいたほうがいいか？？
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {

			const Color rootColor = pos.sidetomove();

			//この局面の差し手を生成する
			memset(moves, 0, sizeof(moves));//初期化
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;


			//棋譜の差し手
			Move teacher_move = thisgame.moves[ply];
			//棋譜の差し手は合法手か？
			if (is_ok(teacher_move) == false) { cout << "is not ok" << endl; goto ERROR_OCCURED; }
			if (!swapmove(moves, int(num_moves), teacher_move)) {
				cout << "cant swap" << endl;
				goto ERROR_OCCURED;
			}
			if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }
			//探索を行ってpv[0]がteachermoveか確認する。
			
			th.set(pos);
			Value  score = th.think();
			if (th.pv[0] == teacher_move) { num_concordance_move++; }

			pos.do_move(teacher_move, &si[ply]);
			num_all_move++;
		}
	ERROR_OCCURED:;
	}
	
	//cout <<"一致率 "<< double(num_concordance_move * 100 / num_all_move)<<" %"<<endl;
	return double((double)num_concordance_move * 100 / (double)num_all_move);
}










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
#ifdef test_learn
	int readgames = 20;
	int numtestset = 19;
#else
	int readgames = 50000;
	int numtestset = 500;
#endif
	
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
	//ifstream gamedata(fg2800_2ch);
	ifstream gamedata(nichkihu);//ソフト棋譜の水平線効果を学習させないために2ch棋譜のみを使用して学習することにする。(技巧やapery(sdt4を除く)もそれで学習していたはずなので大丈夫だと思う)
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
	for (int i = 0; i < numtestset; ++i) {
		Game game;

		if (gamedatastream.read_onegame(&game)
			&& game.ply >= 60
			&& game.result != draw)
		{
			testset.push_back(game);
		}
	}
	cout << "read kihu OK!" << endl;

#define Test_icchiritu

#ifdef Test_icchiritu
	cout<<concordance()<<endl;
#endif


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
		//Parse2Data piyo;
		//parse2Datas.push_back(piyo);
		parse2Datas.emplace_back();
	}

	for (int iteration = 0; iteration < numiteration; iteration++) {
		//時間計測の開始
		cout << "iteration" << iteration << endl;
		limit.starttime = now();
		//過去の情報をクリアする
		objective_function = 0;
		//maxmoves = 0;
		//huicchi_moves = 0;
		for (auto& datas : parse2Datas) {
			datas.clear();
		}
		//学習
		learnphase1();
		learnphase2();


		std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << std::endl;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
		//ofs << "一致率 " << ((maxmoves- huicchi_moves) * 100 / (maxmoves + 1)) << endl;
		//cout << "一致率 " << ((maxmoves - huicchi_moves) * 100 / (maxmoves + 1)) <<" %"<< endl;
		cout << "calcurate 一致率" << endl;
		double icchiritu = concordance();
		ofs << "一致率 " <<icchiritu << " %" << endl;
		cout << "一致率 " << icchiritu <<" %"<< endl;
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
	if (index_ > numgames) { cout << "o" << endl; }
	else { printf("."); }
	
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
	StateInfo si[500];
	ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;


	int didmoves = 0;
	int diddepth = 0;
	//============================================================
	//ここlockingindexincrementにする必要がある！！
	//=============================================================
	for (int g = lock_index_inclement(); g < numgames; g = lock_index_inclement()) {
		didmoves = 0;
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
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

			//Value teachervalue;
			//bool huicchi_firsttime = true;
			ASSERT(moves[0].move == teacher_move);
			for (int move_i = 0; move_i < num_moves; move_i++) {

				Move m = moves[move_i];
				if (pos.is_legal(m) == false) { continue; }
				if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				didmoves++;
				pos.do_move(m, &si[ply]);
				
				th.set(pos);
				Value  score = th.think();
				//if (m==teacher_move) { teachervalue = score; }
				if (abs(score) < Value_mate_in_maxply) {
					minfo_list.push_back(MoveInfo(m, th.pv, score));
				}
				else { pos.undo_move(); break; }
				pos.undo_move();
				//if (score > teachervalue&&huicchi_firsttime == true) { huicchi_firsttime = false;  lock_huicchimoves_inclement(); }
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


					StateInfo si2[64];//最大でも深さ３
					int j = 0;
					//教師手とPVで局面をすすめる
					//if (pos.is_legal(minfo_list[i].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[i].move, &si[ply]);

					for (Move m : minfo_list[i].pv) {
						if (j >= 64) { break; }
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


				StateInfo si2[64];//最大でも深さ３
				int j = 0;
				//教師手に対してdJ/dviを更新
				if (pos.is_legal(minfo_list[0].move) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				if (pos.state()->lastmove == minfo_list[0].move) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				pos.do_move(minfo_list[0].move, &si[ply]);
				//pvの指し手が非合法手になっている事がある？？
				for (Move m : minfo_list[0].pv) {
					if (j >= 64) { break; }
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
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}


}

void learnphase2() {
	sum_parse2Datas.clear();
	//たしあげる
	for (auto& parse2 : parse2Datas) {
		sum_parse2Datas.gradJ.add(parse2.gradJ);
	}

	//左右対称性を考える。
	//左右対称性もgradJに与えるのではなく、gradJを左側にまとめて評価関数を更新してから、評価関数を左右対称にすべきなのかもしれない。
	//これやったらよわくなってしまったので、バグがある。
	

	//num_parse2回パラメーターを更新する。コレで-64から+64の範囲内でパラメーターが動くことになる。
	//bonanzaではこの32回の間にdJが罰金項によってどんどんゼロに近づけられている。
	for (int i = 0; i < num_parse2; i++) {
		renewal_PP(sum_parse2Datas.gradJ);
	}

	//書き出し読み込みをここで行って値の更新
#ifndef test_learn
	write_PP();
	read_PP();
#endif

	
}

//gradJは既に左右対称性を持たされているものとする。
//(bp1,bp2)要素について次元下げを行う
/*
次元下げについて全然わからないので直感でコード書いてる（これが一番いけない気がする）
（まず次元下げの理念ぐらいちゃんと理解しておいたほうがいいのではないか？）
まず一回どんなものか自分で書いてみたほうがほかの人の書いたコードを見たとき理解するのが早いのでまず一回書いてみる。
*/
void each_PP(lowerDimPP & lowdim, const dJValue& gradJ,const BonaPiece bp1,const BonaPiece bp2) {
	if (bp1 == bp2) { return; }//一致する場所はevaluateで見ないのでgradJも0になっている。これは無視していい。
	/*Piece pc;
	Square sq;*/
	//絶対PP
	lowdim.absolute_pp[bp1][bp2]=gradJ.dJ[bp1][bp2];

	//絶対P
	lowdim.absolute_p[bp1] += gradJ.dJ[bp1][bp2];
	lowdim.absolute_p[bp2] += gradJ.dJ[bp1][bp2];
	
	//相対PP（盤上の駒だけ）
	if (bp1 >= fe_hand_end&&bp2 >= fe_hand_end) {
		Piece pc1 = bp2piece.bp_to_piece(bpwithoutsq(bp1)), pc2 = bp2piece.bp_to_piece(bpwithoutsq(bp2));//bp1,bp2の駒種
		Square sq1=bp2sq(bp1), sq2=bp2sq(bp2);//bp1,bp2の駒の位置
		lowdim.relative_pp[pc1][pc2][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8]+= gradJ.dJ[bp1][bp2];
	}
}

void lower__dimPP(lowerDimPP & lowdim,dJValue& gradJ)
{
	//gradjを左右対称に
	param_sym_leftright(gradJ);

	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
			each_PP(lowdim, gradJ, bp1, bp2);
		}
	}
}

//この関数で次元下げされた値をgradJに織り込んでいく。（weave：織り込む）
void weave_eachPP(dJValue& newgradJ, const lowerDimPP& lowdim, const BonaPiece bp1, const BonaPiece bp2) {
	if (bp1 == bp2) { return; }//一致する場所はevaluateで見ないのでgradJも0でいい。これは無視していい。
	//絶対PP
	newgradJ.dJ[bp1][bp2] = lowdim.absolute_pp[bp1][bp2];

	//絶対P
	newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[bp1];
	newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[bp2];

	//相対PP（盤上の駒だけ）
	if (bp1 >= fe_hand_end&&bp2 >= fe_hand_end) {
		Piece pc1=bp2piece.bp_to_piece(bpwithoutsq(bp1)), pc2 = bp2piece.bp_to_piece(bpwithoutsq(bp2));//bp1,bp2の駒種
		Square sq1=bp2sq(bp1), sq2=bp2sq(bp2);//bp1,bp2の駒の位置
		newgradJ.dJ[bp1][bp2]+=lowdim.relative_pp[pc1][pc2][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8];//平行移動
	}
}


//次元下げされた値をもとにしてgradJを構築する。
void weave_lowdim_to_gradj(dJValue& newgradJ, const lowerDimPP& lowdim) {

	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
			weave_eachPP(newgradJ, lowdim, bp1, bp2);
		}
	}
}

#endif//learn