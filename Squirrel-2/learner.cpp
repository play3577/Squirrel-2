#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>
#include <vector>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
using namespace Eval;
using namespace std;



double dJ[fe_end2][fe_end2];

//dJ/dviをdJ配列にそれぞれ格納していく。
void update_dJ(const Position pos, const double diff) {

	const auto list1 = pos.evallist();

	const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < i; j++) {
			dJ[list_fb[i]][list_fb[j]] += diff;
			dJ[list_fw[i]][list_fw[j]] += diff;
			//PP対称性を考えて
			dJ[list_fb[j]][list_fb[i]] += diff;
			dJ[list_fw[j]][list_fw[i]] += diff;
		}
	}
}

//コレだとd==0の場合の特徴量がどんどんマイナスに大きくなってしまう(修正済み)　と言うか出てこない特徴量はsparseにしていきたい。
 int sign(const double d) {
	 return (d > 0) ? 1 : (d<0)?-1:0;
}

//教師手の指し手をmoves配列の一番先頭に持ってくるための関数
//moves配列の先頭ポインタ　num要素数　m教師手の指し手
//配列の中に教師手が含まれない場合falseを返す
bool swapmove(ExtMove* moves,const int num,const Move m) {

	ExtMove first = moves[0];

	for (int i = 0; i < num+1; i++) {

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
	FILE* fp = std::fopen("c:/book2/fv_PP.bin", "wb");
	std::fwrite(&PP, sizeof(PP), 1, fp);
	std::fclose(fp);

}

void Eval::read_PP() {

	//ここパスをusioptionで変更できるようにする。
	FILE* fp = std::fopen("c:/book2/fv_PP.bin", "rb");
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
	std::random_device rd;
	std::mt19937 mt(rd());
	//初期化は対称性を考えながらせねばならない(というか乱数で初期化する必要あるか？？)
	for (BonaPiece bp1 = f_hand_pawn; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = f_hand_pawn; bp2 < bp1; bp2++) {

			int a = int32_t(mt()) % 100;
			PP[bp1][bp2]=PP[bp2][bp1]= a;

		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}


//パラメーターの更新のための関数
void renewal_PP() {

	std::random_device rd;
	std::mt19937 mt(rd());
	//hはなんぼぐらいの値に設定すればええんや？？(bonanzaでは0,1,2の値を取る乱数にしているらしい。進歩本６)
	int h;

	//こんなんでいいのか？
	h =  std::abs(int8_t(mt())) % 2 + 1;

	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {
			int inc = h*sign(dJ[i][j]);
			PP[i][j] += inc;
		}
	}
	//書き出した後読み込むことで値を更新する
	write_PP();
	read_PP();
}


//学習用関数
void Eval::learner(Thread & th)
{
	//初期化
	const int numgames = 200;
	const int numiteration = 20;
	ifstream gamedata(gamedatabasefile);
	GameDataStream gamedatastream(gamedata);
	std::vector<Game> games;
	
	int didmoves = 0;
	int diddepth = 0;

	//棋譜を読み込む
	for (int i = 0; i < numgames; ++i) {
		Game game;
		if (gamedatastream.read_onegame(&game)) {
			games.push_back(game);
		}
	}

	cout << "read kihu OK!" << endl;

	//ーーーーーーーーーーーーーここまでちゃんと動いたことを確認済み



	Position pos;
	StateInfo si[256];
	ExtMove moves[600],*end;
	vector<MoveInfo> minfo_list;
	//Thread th;
	end = moves;

	//何故か教師データで用いるdepthが浅い？？

	for (int iteration = 0; iteration < numiteration; iteration++) {
		//iterationの最初に初期化する（それ以外ではしてはいけない）
		memset(dJ, 0, sizeof(dJ));
		
		for (int g = 0; g < numgames; g++) {
			diddepth = 0;
			didmoves = 0;
			

			auto thisgame = games[g];
			pos.set_hirate();
			for (int ply = 0; ply < thisgame.ply; ply++) {
				diddepth = ply;
				minfo_list.clear();

				//この局面の差し手を生成する
				end = test_move_generation(pos, moves);
				ptrdiff_t num_moves = end - moves;


				//棋譜の差し手を指し手リストの一番最初に持ってくる。
				Move teacher_move = thisgame.moves[ply];
				//if (pos.is_legal(teacher_move) == false) { goto ERROR_OCCURED; }
				//合法てリストに教師手が入っていなければここから先おかしくなってしまうのでこの棋譜を使っての学週はここで中断

				//is_okで落ちてくれるのは最後まで利用できたということなのでありがたい
				if(is_ok(teacher_move)==false){ cout << "is not ok" << endl; goto ERROR_OCCURED; }
				//ほとんどcantswapで教師手が否定されてしまう！！
				//殆どの指してがk r bの指して！！指して生成がおかしい！！


				//たまに駒打ちがおかしかったりすることがあるのでoccbitboardの更新部がおかしいのか？

				/*
				とりあえずsfenの逆関数を作り、エラーとなった局面を吐かせて、それらの局面で正しくまずはトビ機器を生成できているかどうか確認する。
				もしこの段階では飛びコマの移動をちゃんと生成できているということであればそれはoccに問題があるということである。
				*/

				if (!swapmove(moves, int(num_moves), teacher_move)) { cout << "cant swap" << endl; cout << pos.sidetomove(); check_move(teacher_move); goto ERROR_OCCURED; }
				if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }

				//差し手に対して探索を行う。探索をしたら探索したPVと指し手と探索した評価値をペアにして格納
				for (int move_i = 0; move_i < num_moves; move_i++) {

					Move m = moves[move_i];
					if (pos.is_legal(m) == false) { continue; }
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

				{


					/*
					教師手ではない指してによる局面に現れる特徴ベクトルは価値を低く、
					教師手による局面に現れる特徴ベクトルは価値を高く、
					両方に出てくる特徴ベクトルに対しては価値を動かさないようにしたい。

					そのためのsum_diff。
					*/
					double sum_diff = Value_Zero;
					if (minfo_list.size() == 0) { cout << "listsize==0" << endl; goto ERROR_OCCURED; }

					//教師手以外の指してに対して
					for (size_t i = 1; i < minfo_list.size(); i++) {

						//評価値と教師手の差分を取る。
						const Value diff = minfo_list[i].score - minfo_list[0].score;

						//教師手とあまりにも離れた価値の指しての影響を少なくするためにdsigmoidに入れるのだと考えている。
						/*
						コンピューター将棋中級者がやねうら王ブログの難しすぎるlong distance libraryの話をよんでも自分のためにならないし,
						sfenのような簡単すぎる話を聞いても自分のためにはならない。

						自分にあったレベルの指し手から学習するのが良いのではないかと考えられるということだと思っている。
						*/
						const double diffsig = dsigmoid(diff);
						sum_diff += diffsig;

						StateInfo si2[3];//最大でも深さ３
						int j = 0;
						//教師手とPVで局面をすすめる
						pos.do_move(minfo_list[i].move, &si[ply]);
						for (Move m : minfo_list[i].pv) {
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
					if (pos.is_legal(minfo_list[0].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[0].move, &si[ply]);
					//pvの指し手が非合法手になっている事がある？？
					for (Move m : minfo_list[0].pv) {
						if (pos.is_legal(m) == false) { ASSERT(0); }
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
		renewal_PP();


		//ここで指し手の一致率みたいな情報を表示できればいいんだけれど...
		std::cout << "iteration" << iteration<<"/maxiteration :"<<numiteration << std::endl;


	}//for num iteration
	


}

