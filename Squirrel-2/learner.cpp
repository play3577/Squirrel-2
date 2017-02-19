#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>
#include <vector>
#include <time.h>       /* time_t, struct tm, time, localtime */
#include <fstream>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
#include "usi.h"
using namespace Eval;
using namespace std;

#define LOG



//コレだとd==0の場合の特徴量がどんどんマイナスに大きくなってしまう。と言うか出てこない特徴量はどんどんsparseにしていきたい。
//私の最終目標は棋譜データの足りない分をスパースモデリング理論で補う学習方法の考案にあるのだ！
int sign(const double d) {
	return (d > 0) ? 1 : (d<0) ? -1 : 0;
#if 0
	//return  (d > 0) ? 1: -1; //これにするとかなり弱くなってしまった。1000棋譜でこれを使おうとするのはだめだな....
#endif
}

//教師手の指し手をmoves配列の一番先頭に持ってくるための関数
//moves配列の先頭ポインタ　num要素数　m教師手の指し手
//配列の中に教師手が含まれない場合falseを返す
bool swapmove(ExtMove* moves, const int num, const Move m) {

	ExtMove first = moves[0];

	for (int i = 0; i < num + 1; i++) {

		if (moves[i].move == m) {

			moves[i] = first.move;
			moves[0].move = m;

			return true;
		}
	}

	return false;
}

void Eval::write_PP()
{
	// ファイルへの書き出し（ここパスをusioptionで変更できるようにする。）
	FILE* fp = std::fopen(Options["eval"].str().c_str(), "wb");
	std::fwrite(&PP, sizeof(PP), 1, fp);
	std::fclose(fp);

}

void Eval::read_PP() {

	//ここパスをusioptionで変更できるようにする。
	FILE* fp = std::fopen(Options["eval"].str().c_str(), "rb");
	if (fp != NULL) {
		std::fread(&PP, sizeof(PP), 1, fp);
	}
	else {
		cout << "error reading PP!!!" << endl;
	}
	std::fclose(fp);

	return;
}



//PPを乱数で初期化する
void Eval::initialize_PP()
{
	string YorN;
	cout << "do you really wanna initialize feature vector? [y/n]" << endl;
	cin >> YorN;

	if (YorN != "y") { cout << "I don't initialize." << endl; return; }
	memset(PP, 0, sizeof(PP));
	//std::random_device rd;
	//std::mt19937 mt(rd());
	////初期化は対称性を考えながらせねばならない(というか乱数で初期化する必要あるか？？)
	//for (BonaPiece bp1 = f_hand_pawn; bp1 < fe_end2; bp1++) {
	//	for (BonaPiece bp2 = f_hand_pawn; bp2 < bp1; bp2++) {

	//		int a = int32_t(mt()) % 100;
	//		PP[bp1][bp2]=PP[bp2][bp1]= a;

	//	}
	//}
	write_PP();
	cout << "initialize param PP!" << endl;
}


#if 0
double dJ[fe_end2][fe_end2];


void param_sym_leftright();


//dJ/dviをdJ配列にそれぞれ格納していく。
void update_dJ(const Position pos, const double diff) {

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

#define USE_PENALTY
//パラメーターの更新のための関数
void renewal_PP() {

	std::random_device rd;
	std::mt19937 mt(rd());

	//hはなんぼぐらいの値に設定すればええんや？？(bonanzaでは0,1,2の値を取る乱数にしているらしい。進歩本６)
	//コレは学習の進行度に合わせて変化させる必要があると思うのだが...。
	//最初は大雑把にだんだん正確に動かすように変化させる。
	//bonanzaはpharse 2を32回繰り返すらしいのでその回数を損失が小さくなっていく毎に減らしていけばいいと考えられる。
	int h;

	//こんなんでいいのか？
	h =  std::abs(int(mt())) % 3;

	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

#ifdef USE_PENALTY
			/*
			ペナルティ項をつけることで値がPP[i][j]から大きく離れることはなくなる
			しかしどれぐらいの大きさのペナルティ項を用意すればいいものか...

			Aperyに倣う
			*/
			/*if (PP[i][j]>0) { dJ[i][j] -= double(0.2/double(FV_SCALE)); }
			else if (PP[i][j]<0) { dJ[i][j] += double(0.2/double(FV_SCALE)); }
			*/
#endif
			int inc = h*sign(dJ[i][j]);
			PP[i][j] += inc;
		}
	}
	//書き出した後読み込むことで値を更新する　ここで32回も書き出し書き込みを行うのは無駄最後にまとめて行う
	
	
}


//学習用関数
//学習の並列化もさせたいが、t2.microは1coreしかなかったので....
/*
学習部で同じ差し手を繰り返してしまうバグは教師データが原因でもなかったし仕掛けたassertにもかからなかった！！
iterationが増えると起こる？？

*/
void Eval::learner()
{
	//初期化
	int readgames = 10000;
	const int numgames = 1500;//debug用
	const int numiteration = 1000;
	/*bonanzaではparse2を３２回繰り返すらしいんでそれを参考にする。
	学習の損失の現象が進むに連れてnum_parse2の値を減らしていく
	*/
	const int num_parse2 = 32;
	ifstream gamedata(gamedatabasefile);
	GameDataStream gamedatastream(gamedata);
	std::vector<Game> games;
	
	int didmoves = 0;
	int diddepth = 0;

	//目的関数。コレが小さくなってくれば学習が進んでいるとみなせる。(はずっ！)
	/*
	学習が進むにつれて教師手の価値がほかの差し手よりも大きくなるので目的関数は大きくなっていく。
	この値がほぼ変わらなくても強くなることはあるのでこの値は全く参考にならない
	*/
	double objective_function = 0;


	//棋譜を読み込む
	for (int i = 0; i < readgames; ++i) {
		Game game;

		if (gamedatastream.read_onegame(&game)
			&&game.ply>=60
			&&game.result!=draw)
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
	///home/daruma/fvPP/

#if defined(_MSC_VER)
	filepath = "c:/book2/log/" + str + ".txt";
#endif
#if defined(__unix__) 
	//filepath = "/home/daruma/fvPP/" + str + ".txt";
	filepath = "/home/daruma/fvPP/aa.txt";
#endif
	
	ofstream ofs(filepath);
	if (ofs.fail()) { cout << "log fileError"; }

#endif


	Position pos;
	StateInfo si[256];
	ExtMove moves[600],*end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;


	

	for (int iteration = 0; iteration < numiteration; iteration++) {

		//時間計測の開始
		limit.starttime = now();

		//棋譜のシャッフル
		//random_shuffleを使うと同じ並べ替えられ方になったのでshuffle()を使ってみる
		std::random_device rd;
		std::mt19937 g_mt(rd());
		std::shuffle(games.begin(), games.end(),g_mt);

		//iterationの最初に初期化する（それ以外ではしてはいけない）
		memset(dJ, 0, sizeof(dJ));
		//目的関数
		objective_function = 0;


		for (int g = 0; g < numgames; g++) {
			diddepth = 0;
			didmoves = 0;
			

			auto thisgame = games[g];
			pos.set_hirate();
			for (int ply = 0; ply < (thisgame.moves.size()-1); ply++) {
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
//					cout << pos << endl;
//					check_move(teacher_move); 
//					//ASSERT(0);
#ifdef LOG
					//ofs << " swap error position :" << pos.make_sfen() << " error move " << teacher_move << endl;
#endif
					
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
					th.cleartable();
					th.set(pos);
					Value  score = th.think();
					if (abs(score) < Value_mate_in_maxply) {
						minfo_list.push_back(MoveInfo(m, th.pv, score));
					}
					else { pos.undo_move(); break; }
					pos.undo_move();
				}
				//ここでこの局面の指し手による探索終了

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
						update_dJ(pos, -diffsig);//pvの先端（leaf node）に移動してdJを計算するのがTD-leafということ？？
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
					update_dJ(pos, sum_diff);
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();

				}//勾配計算
				
				//次の局面へ
				pos.do_move(teacher_move, &si[ply]);
			}// for gameply

		ERROR_OCCURED:;
			cout << "game number: " << g << "/ maxgamenum: " << numgames <<" didmoves " << didmoves << " diddepth " << diddepth<<endl;

		}//for numgames

		//dJは全指し手と全棋譜に対して足し上げられたのでここからbonanzaで言うparse2
		
		

		//num_parse2回パラメーターを更新する。コレで-64から+64の範囲内でパラメーターが動くことになる。
		//bonanzaではこの32回の間にdJが罰金項によってどんどんゼロに近づけられている。
		for (int i = 0; i < num_parse2; i++) {
			renewal_PP();
		}
		//書き出し読み込みをここで行って値の更新

		write_PP();
		read_PP();

		//ここで統計情報を表示できればいいんだけれど...
		std::cout << "iteration" << iteration<<"/maxiteration :"<<numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min"<< std::endl;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
#endif

	}//for num iteration
	

	ofs.close();
}


//左右の対称性を考えてdJの値を調整する。
//左右の対称性を考えると弱くなってしまうことも起こりうるそうだが実際どうなんだろうか？
//http://yaneuraou.yaneu.com/2014/12/23/kpp%E3%81%A7%E3%81%AF%E5%B7%A6%E5%8F%B3%E3%81%AE%E5%AF%BE%E7%A7%B0%E6%80%A7%E3%82%92%E8%80%83%E6%85%AE%E3%81%99%E3%82%8B%E3%81%A8%E5%BC%B1%E3%81%8F%E3%81%AA%E3%82%8B%EF%BC%81%EF%BC%9F/
/*
私は今のところ左右の対称性を考えて学習を進めたい。
左右対称にしてしまい、正しく局面を評価できないデメリットは多少あるかもしれないが、
学習されていない特徴の数を減らすことのほうが大事だと考えるからである。
*/
//PPの三角対称性を妨げてしまっていた！！！！！
void param_sym_leftright() {

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

			dJ[il][jl] = dJ[ir][jr] = (dJ[il][jl] + dJ[ir][jr]);
			//check[il*(il + 1) / 2 + jl] = check[ir*(ir + 1) / 2 + jr] = true;
			check[il][jl] = check[ir][jr] = true;

		}
	}
}
#endif