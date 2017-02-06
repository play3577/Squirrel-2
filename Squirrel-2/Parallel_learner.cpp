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
				//PP�Ώ̐����l����
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

	//����Ȃ�ł����̂��H
	h = std::abs(int(mt())) % 3;

	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
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

	//l���]�O, r���]��
	BonaPiece il, ir, jl, jr;

	//static bool check[fe_end2*fe_end2 / 2] = {false};
	//t2.micro�͑ς��Ă���邾�낤��....
	static bool check[fe_end2][fe_end2] = { false };
	memset(check, false, sizeof(check));
	//PP�̈�ڂ�index�ɂ���
	for (il = f_hand_pawn; il < fe_end2; il++) {

		//������̏ꍇ�͂��̂܂܂ł����B�i���E�Ώ̂ɂȂ�ďo���Ȃ�����j
		if (il < fe_hand_end) { ir = il; }
		else {
			//�Տ�̋�͍��E�𔽓]������B
			ir = Eval::sym_rightleft(il);
		}

		//PP�̓�ڂ�index�ɂ���
		//for (jl = f_hand_pawn; jl <= il; jl++) {
		for (jl = f_hand_pawn; jl <fe_end2; jl++) {

			//������̏ꍇ�͂��̂܂܂ł����B�i���E�Ώ̂ɂȂ�ďo���Ȃ�����j
			if (jl < fe_hand_end) { jr = jl; }
			else {
				//�Տ�̋�͍��E�𔽓]������B
				jr = Eval::sym_rightleft(jl);
			}

			if ((il == ir) && (jl == jr)) { continue; }//���]�����Ă�����Ȃ��Ȃ̂ŃR���͑Ώ̐����l����Ӗ��͂Ȃ��i�����T�؂̋�||�������j
			if (check[il][jl] == true) { ASSERT(check[ir][jr] == true); continue; }
			//if(check[il*(il+1)/2+jl]==true){ ASSERT(check[ir*(ir + 1) / 2 + jr] == true); continue; }
			//�����֌W�Ȃ̂łQ����dJ/dvi��p���邱�Ƃ��ł���I�I
			/*
			������(iljl,irjr)��(irjr,iljl)��dJ���Q�d�Ɍv�Z���Ă��܂��A�l�����������Ȃ��Ă��܂����Ƃ��N���肤��B
			�R�������Ƃ����Ėh���Ȃ���΂Ȃ�Ȃ��B�O�p�e�[�u���Ώ̐������Ă��܂���
			*/

			/*
			for (il = f_hand_pawn; il < fe_end2; il++) �Ȃ̂�
			���̂܂܂ł�
			dJ[il][jl] = dJ[ir][jr]= (dJ[il][jl] + dJ[ir][jr]);�������コ���
			dJ[ir][jr] = dJ[il][jl]= (dJ[il][jl] + dJ[ir][jr]);������dJ[][]�̒l�����������Ȃ��Ă��܂����Ƃ��N���肤��B

			�����h�����߂ɂ�sym_rightleft()��sq���Ֆʂ̉E���ł����return -1������Ηǂ���������Ȃ���,���E���]�����邾���̊֐��ɂ��̂悤�ȋ@�\����������̂͂���܂肵�����Ȃ�

			bool check[fe_end2][fe_end2]�ň�x�v�Z�����g�ݍ��킹���ǂ����m�F����悤�ɂ�����B
			���̕��@�ł�check���m�ۂ��邱�Ƃ��ł��Ȃ������B�istatic�ɂ��Ė����������B�j
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
�w�K���Ɉ�v�����v�Z�����Ă������Ȃ񂩃o�O����cpu�g�p�����O�ɂȂ��Ă��܂����Ƃ��p�������̂�
��v���v�Z�̊֐���p�ӂ��čŌ�Ɉ�v�����v�Z����B

�ǂ��Ƀo�O������H�H�H�H�H�H�H�H�H�H
*/
double concordance() {
	std::random_device rd;
	std::mt19937 t_mt(rd());
	//std::shuffle(testset.begin(), testset.end(), t_mt);//�V���b�t�������Ă邪���Ȃ��ق��������H�H

	int num_tests = 500;//500�����Ŋm�F����
	ASSERT(num_tests < testset.size());

	//�����Ő錾�������̂�����ǂ���ł����̂��낤���H
	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
	//vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;

	int64_t num_concordance_move = 0;
	int64_t num_all_move = 0;

	for (int g = 0; g < num_tests; g++) {


		memset(si, 0, sizeof(si));//������

		auto thisgame = testset[g];//�����w�K�p�̃f�[�^�Ƌ�ʂ��Ă������ق����������H�H
		pos.set_hirate();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {

			const Color rootColor = pos.sidetomove();

			//���̋ǖʂ̍�����𐶐�����
			memset(moves, 0, sizeof(moves));//������
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;


			//�����̍�����
			Move teacher_move = thisgame.moves[ply];
			//�����̍�����͍��@�肩�H
			if (is_ok(teacher_move) == false) { cout << "is not ok" << endl; goto ERROR_OCCURED; }
			if (!swapmove(moves, int(num_moves), teacher_move)) {
				cout << "cant swap" << endl;
				goto ERROR_OCCURED;
			}
			if (pos.is_legal(teacher_move) == false||pos.pseudo_legal(teacher_move)==false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }
			//�T�����s����pv[0]��teachermove���m�F����B
			th.set(pos);
			Value  score = th.think();
			if (th.pv[0] == teacher_move) { num_concordance_move++; }

			pos.do_move(teacher_move, &si[ply]);
			num_all_move++;
		}
	ERROR_OCCURED:;
	}

	//cout <<"��v�� "<< double(num_concordance_move * 100 / num_all_move)<<" %"<<endl;
	return double((double)num_concordance_move * 100 / (double)num_all_move);
}










//�ړI�֐��B�R�����������Ȃ��Ă���Ίw�K���i��ł���Ƃ݂Ȃ���B(�͂����I)
/*
�w�K���i�ނɂ�ċ��t��̉��l���ق��̍���������傫���Ȃ�̂ŖړI�֐��͑傫���Ȃ��Ă����B
���̒l���قڕς��Ȃ��Ă������Ȃ邱�Ƃ͂���̂ł��̒l�͑S���Q�l�ɂȂ�Ȃ�
*/
double objective_function = 0;

int maxthreadnum;
int numgames;
int num_parse2;
void Eval::parallel_learner() {

	//������
	int readgames = 80000;
	int numtestset = 2500;
	numgames = 2000;
	int numiteration = 1000;
	maxthreadnum = omp_get_max_threads();

	cout << "numgames:>>";
	cin >> numgames;
	cout << "numiteration?>>";
	cin >> numiteration;

	/*bonanza�ł�parse2���R�Q��J��Ԃ��炵����ł�����Q�l�ɂ���B
	�w�K�̑����̌��ۂ��i�ނɘA���num_parse2�̒l�����炵�Ă���
	*/
	num_parse2 = 32;
	ifstream gamedata(fg2800_2ch);
	//ifstream gamedata(nichkihu);
	GameDataStream gamedatastream(gamedata);
	

	


	//������ǂݍ���
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

//#define Test_icchiritu

#ifdef Test_icchiritu
	concordance();
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
	//si��moves,position�͕��񂳂���֐����Ő�������

	for (size_t i = 0; i < maxthreadnum; i++) {
		//Parse2Data piyo;
		//parse2Datas.push_back(piyo);
		parse2Datas.emplace_back();
	}

	for (int iteration = 0; iteration < numiteration; iteration++) {
		//���Ԍv���̊J�n
		cout << "iteration" << iteration << endl;
		limit.starttime = now();
		//�ߋ��̏����N���A����
		objective_function = 0;
		//maxmoves = 0;
		//huicchi_moves = 0;
		for (auto& datas : parse2Datas) {
			datas.clear();
		}
		//�w�K
		learnphase1();
		learnphase2();


		std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << std::endl;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
		//ofs << "��v�� " << ((maxmoves- huicchi_moves) * 100 / (maxmoves + 1)) << endl;
		//cout << "��v�� " << ((maxmoves - huicchi_moves) * 100 / (maxmoves + 1)) <<" %"<< endl;
		cout << "calcurate ��v��" << endl;
		/*double icchiritu = concordance();
		ofs << "��v�� " <<icchiritu << " %" << endl;
		cout << "��v�� " << icchiritu <<" %"<< endl;*/
#endif

	}



}


//http://kaworu.jpn.org/cpp/std::mutex
//�A�N�Z�X�����̖h�~
//�����p���邱�ƂŊw�K�����ɐi�߂邱�Ƃ��ł���(from Apery)
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
	vector<std::thread> threads(maxthreadnum - 1);//maxthreadnum-1����std::thread�����Ă�B
	index_ = 0;
	//����w�K�J�n
	for (int i = 0; i < maxthreadnum - 1; ++i) {
		threads[i] = std::thread([i]{learnphase1body(i); });
	}
	learnphase1body(maxthreadnum-1);

	for (auto& th : threads) { th.join(); }

}


/*
lockingindexincrement�͂ǂ���������H�H
*/
void learnphase1body(int number) {

	//�����Ő錾�������̂�����ǂ���ł����̂��낤���H
	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;


	int didmoves = 0;
	int diddepth = 0;
	//============================================================
	//����lockingindexincrement�ɂ���K�v������I�I
	//=============================================================
	for (int g = lock_index_inclement(); g < numgames; g=lock_index_inclement()){
		didmoves = 0;
		auto thisgame = games[g];
		pos.set_hirate();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {
			diddepth = ply;
			minfo_list.clear();

			const Color rootColor = pos.sidetomove();

			//���̋ǖʂ̍�����𐶐�����
			memset(moves, 0, sizeof(moves));//������
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;


			//�����̍�������w���胊�X�g�̈�ԍŏ��Ɏ����Ă���B
			Move teacher_move = thisgame.moves[ply];
			//if (pos.is_legal(teacher_move) == false) { goto ERROR_OCCURED; }
			//���@�ă��X�g�ɋ��t�肪�����Ă��Ȃ���΂�������您�������Ȃ��Ă��܂��̂ł��̊������g���Ă̊w�T�͂����Œ��f

			//is_ok�ŗ����Ă����͍̂Ō�܂ŗ��p�ł����Ƃ������ƂȂ̂ł��肪����
			if (is_ok(teacher_move) == false) { cout << "is not ok" << endl; goto ERROR_OCCURED; }

			if (!swapmove(moves, int(num_moves), teacher_move)) {
				cout << "cant swap" << endl;
				
				goto ERROR_OCCURED;
			}
			if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }

			//������ɑ΂��ĒT�����s���B�T����������T������PV�Ǝw����ƒT�������]���l���y�A�ɂ��Ċi�[
			//����͏����������Ĉ�ǖʍő�i15�肮�炢�ɂ��������������H�j
			//���ǖʂ�����̎w����̐��̐����͂��ׂ��ł͂Ȃ���������Ȃ�
			//bonanza�ł͑S���@��ɑ΂��Ă����Ȃ��Ă����̂�Squirrel�ł����Ȃ����ƂɌ��߂�
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
			//�����ł��̋ǖʂ̎w����ɂ��T���I��
			
			//��������sum_gradJ�̍X�V
			{


				/*
				���t��ł͂Ȃ��w���Ăɂ��ǖʂɌ��������x�N�g���͉��l��Ⴍ�A
				���t��ɂ��ǖʂɌ��������x�N�g���͉��l�������A
				�����ɏo�Ă�������x�N�g���ɑ΂��Ă͉��l�𓮂����Ȃ��悤�ɂ������B

				���̂��߂�sum_diff�B
				�ǂ̍�������w���Ă��܂���邱�Ƃ����������ꍇ��listsize==0�ɂȂ�I
				*/
				double sum_diff = Value_Zero;
				if (minfo_list.size() == 0) {
					cout << "listsize==0" << endl;
					/*cout << pos << endl;
					check_move(teacher_move);
					cout << "aa" << endl;*/
					goto ERROR_OCCURED;
				}

				//���t��ȊO�̎w���Ăɑ΂���
				for (int i = 1; i < minfo_list.size(); i++) {

					//�]���l�Ƌ��t��̍��������B
					const Value diff = minfo_list[i].score - minfo_list[0].score;

					/*
					http://kifuwarabe.warabenture.com/2016/11/23/%E3%81%A9%E3%81%86%E3%81%B6%E3%81%A4%E3%81%97%E3%82%87%E3%81%86%E3%81%8E%E3%82%92%E3%83%97%E3%83%AD%E3%82%B0%E3%83%A9%E3%82%82%E3%81%86%E3%81%9C%E2%98%86%EF%BC%88%EF%BC%92%EF%BC%94%EF%BC%89%E3%80%80/
					�V�O���C�h�֐��ɒl������̂͋��t��̉��l�Ɠ������炢�̉��l�̎w����ɑ΂��Ċw�K�����A
					��������X�Ɉ����A���t����X�ɗǂ��w�K�����Ȃ��悤�ɂ��A
					�l���傫�����ꂽ�炻�̃p�����[�^�[�����������肵�Ȃ��悤�ɂ��A���������邽�߂ł���B

					���������t��������Ȃ荂���ǂ��肾�ƃR���s���[�^�[���딻�f���Ă���w���肪�V�O���C�h�֐��̔����ɓ����Ă��Ă��܂����ꍇ����ɑ΂���o�ė���l���������Ȃ��Ă��܂��A
					�l���������Ȃ��̂ł͂Ȃ����H
					����͗ǂ��Ȃ��C������̂�����ǎ��ۂǂ��Ȃ񂾂낤���H
					*/


					//=======================================================================================================================
					//bonanza�Ȃǂł͎�Ԃ̐F�ɂ����diffsig�̕�����ς��Ă��邪����Ȃ��Ƃ���K�v����̂��H�H�Ȃ��H�H
					//=======================================================================================================================

					/*
					���̎w����̉��l�����t������傫���������߁A���l�����������ꍇ���l����B
					���Ԃ̏ꍇ�͍ŏI�I��update_dJ�ɓ���̂�-dsig
					���Ԃ̏ꍇ�͍ŏI�I��update_dJ�ɓ���̂�dsig�ɂȂ�B

					�ڂ������
					https://twitter.com/daruma3940/status/801638488130994177
					*/
					double diffsig = dsigmoid(diff);
					diffsig = (rootColor == BLACK ? diffsig : -diffsig);

					sum_diff += diffsig;
					objective_function += sigmoid(diff);


					StateInfo si2[MAX_DEPTH];//�Î~�T���̐�܂Ői�߂邱�Ƃɂ���
					int j = 0;
					//���t���PV�ŋǖʂ������߂�
					//if (pos.is_legal(minfo_list[i].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[i].move, &si[ply]);

					for (Move m : minfo_list[i].pv) {
						if (j >= MAX_DEPTH) { break; }//mate����������������Maxdepth�𒴂���H
						if (pos.is_legal(m) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
						pos.do_move(m, &si2[j]);
						j++;
					}
					parse2Datas[number].gradJ.update_dJ(pos, -diffsig);//pv�̐�[�ileaf node�j�Ɉړ�����dJ���v�Z����̂�TD-leaf�Ƃ������ƁH�H
											 //�ǖʂ�߂�
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}//end of (���t��ȊO�̎w���Ăɑ΂���)


				StateInfo si2[MAX_DEPTH];//�Î~�T���̐�܂Ői�߂邱�Ƃɂ���
				int j = 0;
				//���t��ɑ΂���dJ/dvi���X�V
				if (pos.is_legal(minfo_list[0].move) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				if (pos.state()->lastmove == minfo_list[0].move) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				pos.do_move(minfo_list[0].move, &si[ply]);
				//pv�̎w���肪�񍇖@��ɂȂ��Ă��鎖������H�H
				for (Move m : minfo_list[0].pv) {
					if (j >= MAX_DEPTH) { break; }
					if (pos.is_legal(m) == false) { cout << games[g].black_P << " " << games[g].white_P;  ASSERT(0); }
					if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					pos.do_move(m, &si2[j]);
					j++;
				}
				parse2Datas[number].gradJ.update_dJ(pos, sum_diff);//���[�̋ǖʂɓ_��������
				for (int jj = 0; jj < j; jj++) {
					pos.undo_move();
				}
				pos.undo_move();

			}//���z�v�Z

			 //���̋ǖʂ�
			pos.do_move(teacher_move, &si[ply]);
		
		}
	ERROR_OCCURED:;
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}

}

void learnphase2() {
	sum_parse2Datas.clear();
	//����������
	for (auto& parse2 : parse2Datas) {
		sum_parse2Datas.gradJ.add(parse2.gradJ);
	}

	//���E�Ώ̐����l����B
	param_sym_leftright(sum_parse2Datas.gradJ);

	//num_parse2��p�����[�^�[���X�V����B�R����-64����+64�͈͓̔��Ńp�����[�^�[���������ƂɂȂ�B
	//bonanza�ł͂���32��̊Ԃ�dJ���������ɂ���Ăǂ�ǂ�[���ɋ߂Â����Ă���B
	for (int i = 0; i < num_parse2; i++) {
		renewal_PP(sum_parse2Datas.gradJ);
	}

	//�����o���ǂݍ��݂������ōs���Ēl�̍X�V
	write_PP();
	read_PP();


	
}

#endif//learn
