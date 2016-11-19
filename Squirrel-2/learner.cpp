#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>
#include <vector>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
using namespace Eval;
using namespace std;


//教師手の指し手をmoves配列の一番先頭に持ってくるための関数
//moves配列の先頭ポインタ　num要素数　m教師手の指し手
//配列の中に教師手が含まれない場合falseを返す
bool swapmove(ExtMove* moves, int num, Move m) {

	ExtMove first = moves[0];

	for (int i = 0; i < num; i++) {

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

			PP[bp1][bp2]=PP[bp2][bp1]= int32_t(mt())%1000;

		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}


//パラメーターの更新のための関数
void Eval::renewal_PP(const Position & pos, const int32_t ** inc) {

	//そうだ list_fbだけじゃなくてlist_fwも用意しないとだめじゃん...

	auto list1 = pos.evallist();

	BonaPiece *list_fb = list1.bplist_fb, *list_fw=list1.bplist_fw;

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < i; j++) {
			PP[list_fb[i]][list_fb[j]] += inc[list_fb[i]][list_fb[j]] * FV_SCALE;
			PP[list_fw[i]][list_fw[j]] += inc[list_fw[i]][list_fw[j]] * FV_SCALE;
			//PP対称性を考えて
			PP[list_fb[j]][list_fb[i]] += inc[list_fb[j]][list_fb[i]] * FV_SCALE;
			PP[list_fw[j]][list_fw[i]] += inc[list_fw[j]][list_fw[i]] * FV_SCALE;
		}
	}
	//書き出した後読み込むことで値を更新する（ここでしないほうがいいかもしれない）
	/*write_PP();
	read_PP();*/
}


//学習用関数
void Eval::learner(Thread & th)
{
	//初期化
	const int numgames = 50;
	ifstream gamedata(gamedatabasefile);
	GameDataStream gamedatastream(gamedata);
	std::vector<Game> games;
	

	//棋譜を読み込む
	for (int i = 0; i < numgames; ++i) {
		Game game;
		if (gamedatastream.read_onegame(&game)) {
			games.push_back(game);
		}
	}

	//ーーーーーーーーーーーーーここまでちゃんと動いたことを確認済み



	Position pos;
	StateInfo si[256];
	ExtMove moves[600],*end;
	vector<MoveInfo> minfo_list;
	//Thread th;
	end = moves;

	for (int g = 0; g < numgames; g++) {

		auto thisgame = games[g];
		pos.set_hirate();
		for (int ply = 0; ply < thisgame.ply; ply++) {


			//この局面の差し手を生成する
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;


			//棋譜の差し手を指し手リストの一番最初に持ってくる。
			Move teacher_move = thisgame.moves[ply];
			if (!swapmove(moves, int(num_moves), teacher_move)) { break; }


			//差し手に対して探索を行う。探索をしたら探索したPVと指し手と探索した評価値をペアにして格納
			for (int move_i = 0; move_i < num_moves; num_moves++) {

				Move m = moves[move_i];
				if (pos.is_legal(m) == false) { continue; }
				pos.do_move(m, &si[ply]);
				th.set(pos);
				Value  score = th.think();
				if (abs(score) < Value_mate_in_maxply) {
					minfo_list.push_back(MoveInfo(m, th.pv, score));
				}
				else { pos.undo_move(); break; }
				pos.undo_move();
			}

			//このあたりで局面に対する誤差を計算しないといけない


			pos.do_move(teacher_move, &si[ply]);
		}// for gameply
	}//for numgames

	//損失から勾配を計算して特徴ベクトルの更新


}

