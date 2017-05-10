#define _CRT_SECURE_NO_WARNINGS




#include "learner.h"
#if defined(LEARN) 
#include <random>
#include <vector>
#include <time.h>       /* time_t, struct tm, time, localtime */
#include <fstream>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
#include "progress.h"
#include "usi.h"
#include <omp.h>
#include <thread>
#include <mutex>
#include <cstdio>
#include "Bitboard.h"
using namespace Eval;
using namespace std;

//#define test_learn

#define LOG




#define SOFTKIFU



/*
次元下げされた要素。
PP（p0,p1）=PP絶対(左右対称、手番対称)+PP相対(平行移動、手番対称)

次元下げとは例えばPPでは
一つ目のbonapieceをx,もう一つのbonapieceをyとしたときに
その(x,y)に対応する値を
(x,y)だけでなく(x-y)のように引数が減った次元の低い配列の値も用いることである。
相対位置のことを考えるとx-yというのがどういうことかわかりやすい。

まだ試していない次元下げ
p
ypp
xpp
KPE次元下げ 
効きを与えている駒が何であろうがその効きのあるsquareと駒のcolorが同じならばそこにも値を与える。
*/

lowerDimPP lowdimPP;




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

	//hはループの内部で変えたほうがいいか？
	h = std::abs(int(mt())) % 3;

	//こんなんでいいのか？
	//
#if defined(EVAL_PP)
	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			
			//それぞれの更新幅を乱数できめる
			//h = std::abs(int(mt())) % 3;

		
			//効きを含めた次元下げをしたらpenaltyは必須になると考えられるがどれぐらいの値を使うべきなのか.....
#if 0
			//bonanzaは4万局程に対してこの値なのでmin batchをつかうときはこれではだめ！！！！！
			/*if (PP[i][j]>0) { data.dJ[i][j] -= double(0.2 / double(FV_SCALE)); }
			else if (PP[i][j]<0) { data.dJ[i][j] += double(0.2 / double(FV_SCALE)); }*/
			//PE次元下げでおかしなところにも値がついてしまっている可能性があるため
			if (abs(data.absolute_PP[i][j])<double(1.0/(1<<8))) {
				continue;
			}
#endif
			int inc = h*sign(data.absolute_PP[i][j]);
			PP[i][j] += inc;


		}
	}
#elif defined(EVAL_KPP)
	for (Square ksq = SQ_ZERO; ksq <= Square(82); ksq++) {
		//KPP-----------------------------------------------------------
		for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end; bp1++) {
			for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end; bp2++) {
				kpp[ksq][bp1][bp2] += h*sign(data.absolute_KPP[ksq][bp1][bp2]);
			}
		}
		//KKP-----------------------------------------------------------
		for (Square ksq2 = SQ_ZERO; ksq2 <= Square(82); ksq2++) {
			for (BonaPiece bp3 = BONA_PIECE_ZERO; bp3 < fe_end + 1; bp3++) {
				kkp[ksq][ksq2][bp3] += h*sign(data.absolute_KKP[ksq][ksq2][bp3]);
			}
		}
	}
#endif
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


#define Test_icchiritu


/*
学習中に一致率を計算させていたがなんかバグってcpu使用率が０になってしまうことが頻発したので
一致率計算の関数を用意して最後に一致率を計算する。

学習用の寄付で一致率を図る。学習を進めると過学習を起こすはずであるのでそれで学習にバグがないかどうか確かめる

一致率測定中に差分計算にバグが出ることが発覚した。
どこが悪い？？

*/
double concordance() {
	/*std::random_device rd;
	std::mt19937 t_mt(rd());*/
	//std::shuffle(testset.begin(), testset.end(), t_mt);//シャッフルさせてるがしないほうがいい？？

	int num_tests = 500;//500棋譜で確認する

	//if (num_tests > testset.size()) { num_tests = testset.size(); }
	//ASSERT(num_tests <= testset.size());
	if (num_tests > games.size()) { num_tests = games.size(); }
	ASSERT(num_tests <= games.size());

	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
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
			si[ply].clear();
			
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
			if (pos.is_legal(teacher_move) == false||!pos.pseudo_legal(teacher_move)) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }
			//探索を行ってpv[0]がteachermoveか確認する。
			
			th.set(pos);
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			th.l_depth = 3;

			//差分計算でバグらないようにするため

			eval(pos);
			Value  score = th.think();
			if (th.pv[0] == teacher_move) { num_concordance_move++; }
			th.pv.clear();

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
double ocilation_error = 0.0;
int maxthreadnum;
int numgames;
int num_parse2;
void Eval::parallel_learner() {

	//初期化
#ifdef test_learn
	int readgames = 20;
	int numtestset = 19;
	maxthreadnum = 1;
#else
	int readgames = 70000;
	int numtestset = 500;
	maxthreadnum = omp_get_max_threads();
#endif
	
	numgames = 2000;
	int numiteration = 1000;
	

	

	cout << "numgames:>>";
	cin >> numgames;
	cout << "numiteration?>>";
	cin >> numiteration;
	/*bonanzaではparse2を３２回繰り返すらしいんでそれを参考にする。
	学習の損失の現象が進むに連れてnum_parse2の値を減らしていく
	*/
	num_parse2 = 32;
	
#ifdef  SOFTKIFU
	readgames = 35000;
	ifstream gamedata(gamedatabasefile);
 //  SOFTKIFU
#else
	readgames = 70000;
	//ifstream gamedata(fg2800_2ch);
	ifstream gamedata(nichkihu);//ソフト棋譜の水平線効果を学習させないために2ch棋譜のみを使用して学習することにする。(技巧やapery(sdt4を除く)もそれで学習していたはずなので大丈夫だと思う)
#endif
	GameDataStream gamedatastream(gamedata);
	
	cout << "readgames " << readgames << endl;
	


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
	/*for (int i = 0; i < numtestset; ++i) {
		Game game;

		if (gamedatastream.read_onegame(&game)
			&& game.ply >= 60
			&& game.result != draw)
		{
			testset.push_back(game);
		}
	}*/
	cout <<"teacher games "<< games.size() << endl;
	cout << "test games " << testset.size() << endl;
	cout << "read kihu OK!" << endl;



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
		ocilation_error = 0.0;
		//maxmoves = 0;
		//huicchi_moves = 0;
		for (auto& datas : parse2Datas) {
			datas.clear();
		}
		//学習
		learnphase1();
		learnphase2();


		std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min " << " ocillation error " << ocilation_error;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << " ocillation error " << ocilation_error;
		//ofs << "一致率 " << ((maxmoves- huicchi_moves) * 100 / (maxmoves + 1)) << endl;
		//cout << "一致率 " << ((maxmoves - huicchi_moves) * 100 / (maxmoves + 1)) <<" %"<< endl;
		//cout << "calcurate 一致率" << endl;
		double icchiritu = concordance();
		ofs << " 一致率 " <<icchiritu << " %" << endl;
		cout << " 一致率 " << icchiritu <<" %"<< endl;
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

		//iteration2で同じ局面に対してPVを作ってしまわないようにする。
		games[g].other_pv.clear();

		didmoves = 0;
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {
			diddepth = ply;
			minfo_list.clear();

#if 1
			if ((float(ply) / float(thisgame.moves.size())) > 0.8) {
				//cout <<"progress:"<< (float(ply) / float(thisgame.moves.size()))<<" ply:"<<ply<<" maxply:"<< thisgame.moves.size() <<  " progress over 90%" << endl;
				goto ERROR_OCCURED;
			}
			/*if (ply >(thisgame.moves.size() - 10)) {
				goto ERROR_OCCURED;
			}*/
#endif

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
				//cout << "cant swap" << endl;
				//check_move(teacher_move);
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
			Value record_score;

			for (int move_i = 0; move_i < num_moves; move_i++) {

				si[ply].clear();

				Move m = moves[move_i];
				if (pos.is_legal(m) == false) { continue; }
				if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				didmoves++;
				pos.do_move(m, &si[ply]);
				//差分計算のためにここでもevalを呼んで置いたほうがいいか？？？まあvalue_errorになっているのでこのままでもバグはないとは思うが　早くpharse1おわるか？？

				eval(pos);

				th.set(pos);
				/*---------------------------------------------------------------------------------------------------------
				ここではPVの作成だけを行ってscoreはPVで末端まで移動させてeval()呼んで、その値を使用したほうがいい？？？
				---------------------------------------------------------------------------------------------------------*/
				
				/*=================================
				探索のalpha betaは後で実装する
				==================================*/
				if (move_i == 0) {
					th.l_alpha =-Value_Infinite;
					th.l_beta = Value_Infinite;
				}
				else {
					th.l_alpha = record_score-(Value)256;
					th.l_beta = record_score +(Value)256;
				}

				th.l_depth = 3;

				//Aperyや技巧のように探索深さを乱数によって変更してみる。
				//大槻将棋のページでBonanzaが学習中の探索深さを1深くしたら強くなったということが書かれていたので乱数によって深くしてみる。
				//私の手元の実験データでは深くしても強くならなかったのだけれど...
				//深くすると時間がかかりすぎるので2,3でやってみたら弱くなった。
				/*std::random_device seed_gen;
				std::default_random_engine engine(seed_gen());
				std::uniform_int_distribution<int> dis(3, 4);
				th.l_depth = dis(engine);*/

				Value  score = th.think();
				if (move_i == 0) { record_score = score; 
				
					if (abs(record_score) > Value_mate_in_maxply) {
						/*cout << "teachermate " << ply << " " << thisgame.moves.size();
						cout << games[g].black_P << " " << games[g].white_P<<" "<< games[g].day<<endl;
						cout << pos << endl;
						cout << "record_score" << record_score << endl;

						cout <<"teachermove:" <<teacher_move << endl;
						for (Move m : th.pv) {
							check_move(m);
						}*/
						goto ERROR_OCCURED;
					}
				
				}
				
				//if (m==teacher_move) { teachervalue = score; }
				//teachermoveがalpha betaに入らなかった。（詰みの値を返した場合はどうしようか....そのまま学習に使うのはどうかと思う...）
				if ((th.l_alpha<score&&score<th.l_beta)||move_i==0) {
					minfo_list.push_back(MoveInfo(m, th.pv));
				}
				th.pv.clear();//pvのクリアを忘れていた！！！！！
				/*else { 
					pos.undo_move(); break; 
				}*/
				pos.undo_move();
				//if (score > teachervalue&&huicchi_firsttime == true) { huicchi_firsttime = false;  lock_huicchimoves_inclement(); }
			}
			ASSERT(minfo_list[0].move == teacher_move);
			games[g].other_pv.push_back(minfo_list);

			//次の局面へ
			pos.do_move(teacher_move, &si[ply]);

			//ここでこの局面の指し手による探索終了

			//ここからsum_gradJの更新
			//{



		}
	ERROR_OCCURED:;
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}


}

//この値の更新の方法だとパラメーターを移動させてる間にPVが変わってしまったときに対応できないので他の方法で値の更新をしたいのだが...
void learnphase2() {
	
	vector<std::thread> threads(maxthreadnum - 1);//maxthreadnum-1だけstd::threadをたてる。
	cout <<endl<< "parse2" << endl;
	//num_parse2回パラメーターを更新する。コレで-64から+64の範囲内でパラメーターが動くことになる。
	//bonanzaではこの32回の間にdJが罰金項によってどんどんゼロに近づけられている。
	//値の更新だけを32かい行っても意味がない！！！！！！
	//gradの作成から32回行わないといけない！！！
	for (int i = 0; i < num_parse2; i++) {

		sum_parse2Datas.clear();
		for (auto& parse2 : parse2Datas) {
			parse2.clear();
		}
		

		
		index_ = 0;
		//並列学習開始
		for (int i = 0; i < maxthreadnum - 1; ++i) {
			threads[i] = std::thread([i] {learnphase2body(i); });
		}
		learnphase2body(maxthreadnum - 1);


		//たしあげる
		for (auto& parse2 : parse2Datas) {
			sum_parse2Datas.gradJ.add(parse2.gradJ);
		}
		for (auto& th : threads) { th.join(); }

#ifdef JIGENSAGE
		lowdimPP.clear();
		lower__dimPP(lowdimPP, sum_parse2Datas.gradJ);
		sum_parse2Datas.clear();
		weave_lowdim_to_gradj(sum_parse2Datas.gradJ, lowdimPP);
#endif

		renewal_PP(sum_parse2Datas.gradJ);
	}

	//書き出し読み込みをここで行って値の更新
#ifndef test_learn
	Eval::param_sym_ij();
	write_FV();
	read_FV();
	
#endif

	
}

void learnphase2body(int number)
{

	//ここで宣言したいのだけれどこれでいいのだろうか？
	Position pos;
	StateInfo si[500];

	

	//ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	//end = moves;


	int didmoves = 0;
	int diddepth = 0;
	//============================================================
	//ここlockingindexincrementにする必要がある！！
	//=============================================================
	for (int g = lock_index_inclement(); g < numgames; g = lock_index_inclement()) {

		for (int i = 0; i <500; i++) si[i].clear();

		didmoves = 0;
		Move teacher_move;
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
		ASSERT(thisgame.other_pv.size() < thisgame.ply);
		for (int ply = 0; ply < thisgame.other_pv.size(); ply++) {
			diddepth = ply;
			minfo_list.clear();

			minfo_list = thisgame.other_pv[ply];
			

			const Color rootColor = pos.sidetomove();
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
				if (minfo_list.size() <= 1) {
					//cout << "listsize<=1" << endl;
					/*cout << pos << endl;
					check_move(teacher_move);
					cout << "aa" << endl;*/
					teacher_move = thisgame.moves[ply];
					//ASSERT(teacher_move == minfo_list[0].move);
					goto skip_calc;
					if (minfo_list.size() == 0) {
						goto ERROR_OCCURED;
					}
				}
				teacher_move = thisgame.moves[ply];
				ASSERT(teacher_move == minfo_list[0].move);
				Value teachervalue;
				Value deepscore[600] = { Value(0) };
				//ーーーーーーーーーーーーーーーーーー
				//教師手の評価値を求める。
				//ーーーーーーーーーーーーーーーーーー
				{
					StateInfo si2[64];
					int j = 0;

					if (pos.is_legal(minfo_list[0].move) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					if (pos.state()->lastmove == minfo_list[0].move) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					pos.do_move(minfo_list[0].move, &si[ply]);

					for (Move m : minfo_list[0].pv) {
						if (j >= 64) { break; }
						if (pos.is_legal(m) == false) { cout << pos; check_move(m);  ASSERT(0); }
						if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
						pos.do_move(m, &si2[j]);
						j++;
					}
					//evalPPはコマ割りを考えていなかったしvalueを反転させてなかった！！evalをつかうべきだった
					teachervalue = (rootColor == pos.sidetomove())? Eval::eval(pos) : -eval(pos);
					deepscore[0] = teachervalue;
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}
				//=-----------------------------------------
				//教師手以外の指してに対して
				//=-----------------------------------------
				for (int i = 1; i < minfo_list.size(); i++) {


					StateInfo si2[64];
					int j = 0;
					//教師手とPVで局面をすすめる
					if (pos.is_legal(minfo_list[i].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[i].move, &si[ply]);

					for (Move m : minfo_list[i].pv) {
						if (j >= 64) { break; }
						if (pos.is_legal(m) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
						pos.do_move(m, &si2[j]);
						j++;
					}
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
					//bonanzaなどでは手番の色によってdiffsigの符号を変えているがそんなことする必要あるのか？？なぜ？？←evalを思い出してみるべし
					//=======================================================================================================================

					/*
					この指し手の価値が教師手よりも大きかったため、価値を下げたい場合を考える。
					黒番の場合は最終的にupdate_dJに入るのは-dsig
					白番の場合は最終的にupdate_dJに入るのはdsigになる。
					詳しい解説
					https://twitter.com/daruma3940/status/801638488130994177
					*/
					//評価値と教師手の差分を取る。
					const Value score = (rootColor == pos.sidetomove()) ? Eval::eval(pos) : -eval(pos);
					deepscore[i] = score;
					const Value diff = score - teachervalue;
					double diffsig = dsigmoid(diff);
					diffsig = (rootColor == BLACK ? diffsig : -diffsig);

					sum_diff += diffsig;
					objective_function += sigmoid(diff);

					parse2Datas[number].gradJ.update_dJ(pos, -diffsig);//pvの先端（leaf node）に移動してdJを計算するのがTD-leafということ？？
																	   //局面を戻す
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}//end of (教師手以外の指してに対して)

				//教師手を進めた局面でのgradJの更新
				{
					StateInfo si2[64];//最大でも深さ３

					for (int i = 0; i < 64; i++) si2[i].clear();

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
				}

			}//勾配計算
		skip_calc:;
			 //次の局面へ
			pos.do_move(teacher_move, &si[ply]);

		}
	ERROR_OCCURED:;
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}


}


//------------------------------------PP次元sゲ------------------------------------------------------------------------------------------------------------
#ifdef EVAL_PP
void lowdim_each_PP(lowerDimPP & lowdim, const dJValue& gradJ, const BonaPiece bp1, const BonaPiece bp2) {
	if (bp1 == bp2) { return; }//一致する場所はevaluateで見ないのでgradJも0になっている。これは無視していい。

							   //iとjの前後によって違う場所を参照してしまうのを防ぐ。
	BonaPiece i = std::max(bp1, bp2), j = std::min(bp1, bp2);

	const double grad = gradJ.absolute_PP[bp1][bp2];



	//相対PPは盤上の駒に対してのみ行う
	if (bp2sq(i) != Error_SQ&&bp2sq(j) != Error_SQ) {
		Piece pci = (bp2piece.bp_to_piece(bpwithoutsq(i)));//iの駒は先手の駒に変換させられているのでptでいい
		Piece pcj = bp2piece.bp_to_piece(bpwithoutsq(j));//jにいるのが味方の駒か相手の駒かは重要になってくるので含めなければならない。
		Square sq1 = bp2sq(i), sq2 = bp2sq(j);//bp1,bp2の駒の位置
		lowdim.relative_pp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8] += grad;


		lowdim.relative_ypp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8][sqtorank(sq1)] += grad;
		lowdim.relative_xpp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8][sqtofile(sq1)] += grad;

	}
	//ここから効きを含めた次元下げ
	//まずはiの効きのある場所
	/*
	ここで効きのあるマスにgradをまき散らしておいてweaveで拾い集める
	これによる寄与は小さくしておく
	*/
	if (f_pawn <= i) {
		const Piece pci =  (bp2piece.bp_to_piece(bpwithoutsq(i)));
		const Piece pti = piece_type(pci);
		const Square sqi = bp2sq(i);
		const Color ci = piece_color(pci);
		Bitboard ebb=effectBB(SquareBB256[sqi],pti,ci,sqi);//occupiedがおかしいことになっているのでやはりposを与えなければならないか??...厳しい..
		while (ebb.isNot()) {
			const Square esq = ebb.pop();

			lowdim.absolute_pe[j][ci][esq][sqi] += grad*double(1.0 / (1 << (3+distance_table[sqi][esq])));
		}
	}
	if (f_pawn <= j) {
		const Piece pcj = (bp2piece.bp_to_piece(bpwithoutsq(j)));
		const Piece ptj = piece_type(pcj);
		const Square sqj = bp2sq(j);
		const Color cj = piece_color(pcj);
		Bitboard ebb = effectBB(SquareBB256[sqj], ptj, cj, sqj);//occupiedがおかしいことになっているのでやはりposを与えなければならないか??...厳しい..
		while (ebb.isNot()) {
			const Square esq = ebb.pop();

			lowdim.absolute_pe[i][cj][esq][sqj] += grad*double(1.0 / (1 << (3 + distance_table[esq][sqj])));
		}
	}

	//絶対PP
	lowdim.absolute_pp[i][j] += grad;
	//絶対P
	/*lowdim.absolute_p[i]+= gradJ.dJ[bp1][bp2];
	lowdim.absolute_p[j]+=gradJ.dJ[bp1][bp2];*/
}

void weave_eachPP(dJValue& newgradJ, const lowerDimPP& lowdim, const BonaPiece bp1, const BonaPiece bp2) {

	if (bp1 == bp2) { return; }//一致する場所はevaluateで見ないのでgradJも0になっている。これは無視していい。

	//iとjの前後によって違う場所を参照してしまうのを防ぐ。
	BonaPiece i = std::max(bp1, bp2), j = std::min(bp1, bp2);


	//相対PP(平行移動)
	if (bp2sq(i) != Error_SQ&&bp2sq(j) != Error_SQ) {
		Piece pci = (bp2piece.bp_to_piece(bpwithoutsq(i)));//iの駒は先手の駒に変換させられているのでptでいい
		Piece pcj = bp2piece.bp_to_piece(bpwithoutsq(j));//jにいるのが味方の駒か相手の駒かは重要になってくるので含めなければならない。
		Square sq1 = bp2sq(i), sq2 = bp2sq(j);//bp1,bp2の駒の位置
		newgradJ.absolute_PP[bp1][bp2] += lowdim.relative_pp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8];

		newgradJ.absolute_PP[bp1][bp2] += lowdim.relative_ypp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8][sqtorank(sq1)];
		newgradJ.absolute_PP[bp1][bp2] += lowdim.relative_xpp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8][sqtofile(sq1)];

	}

	/*
	ばらまいたgradをかき集めてくる
	*/
#if 1
	if (f_pawn <= i) {
		const Piece pci = (bp2piece.bp_to_piece(bpwithoutsq(i)));
		const Piece pti = piece_type(pci);
		const Square sqi = bp2sq(i);
		const Color ci = piece_color(pci);
		Bitboard ebb = effectBB(SquareBB256[sqi], pti, ci, sqi);
		while (ebb.isNot()) {
			const Square effsq = ebb.pop();

			newgradJ.absolute_PP[bp1][bp2]+=lowdim.absolute_pe[j][ci][effsq][sqi];
		}
	}
	if (f_pawn <= j) {
		const Piece pcj = (bp2piece.bp_to_piece(bpwithoutsq(j)));
		const Piece ptj = piece_type(pcj);
		const Square sqj = bp2sq(j);
		const Color cj = piece_color(pcj);
		Bitboard ebb = effectBB(SquareBB256[sqj], ptj, cj, sqj);
		while (ebb.isNot()) {
			const Square effsq = ebb.pop();

			newgradJ.absolute_PP[bp1][bp2] += lowdim.absolute_pe[i][cj][effsq][sqj];
		}
	}
#endif

	//絶対PP
	newgradJ.absolute_PP[bp1][bp2] += lowdim.absolute_pp[i][j];
	//絶対P
	//newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[i];
	//newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[j];
}



void lower__dimPP(lowerDimPP & lowdim,const dJValue& gradJ)
{
	

	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
			lowdim_each_PP(lowdim, gradJ, bp1, bp2);
		}
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
#endif
//-------------------------------------------------------------------------------------------------------------------------
#endif//learn