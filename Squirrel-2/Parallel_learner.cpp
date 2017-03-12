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


#define SOFTKIFU


#define JIGENSAGE

//#define LOWDIM2

//struct  Parse2data;

struct  dJValue
{
	double absolute_PP[fe_end2][fe_end2];

	

	void update_dJ(const Position& pos, const double diff) {
		const auto list1 = pos.evallist();

		const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				absolute_PP[list_fb[i]][list_fb[j]] += diff;
				absolute_PP[list_fw[i]][list_fw[j]] -= diff;
				//PP�Ώ̐����l����
				absolute_PP[list_fb[j]][list_fb[i]] += diff;
				absolute_PP[list_fw[j]][list_fw[i]] -= diff;
			}
		}
	}

	void clear() { memset(this, 0, sizeof(*this));}


	void add(dJValue& data) {

		for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
			for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
				absolute_PP[bp1][bp2] += data.absolute_PP[bp1][bp2];
			}
		}
	}


#ifdef  LOWDIM2
	double relative_pp[PT_ALL][PC_ALL][17][17]; //[17][17]�͕��s�ړ��̑��Έʒu

	void lowdim_eachPP(const BonaPiece bp1, const BonaPiece bp2, const double diff) {

		double diff_ = diff;

		BonaPiece i = std::max(bp1, bp2), j = std::min(bp1, bp2);

		Piece pci = bp2piece.bp_to_piece(bpwithoutsq(i));
		ASSERT(pci != NO_PIECE);
		//���̋�����ԂƂ��Ĉ������߂�diff�𔽓]������
		if (piece_color(pci) == WHITE) {
			diff_ = -diff_;
			i = inversebonapiece(i);
			j = inversebonapiece(j);
		}
		ASSERT(piece_color(bp2piece.bp_to_piece(bpwithoutsq(i))) == BLACK);

		//����PP
		if (bp2sq(i) != Error_SQ&&bp2sq(j) != Error_SQ) {
			Piece pti = piece_type(bp2piece.bp_to_piece(bpwithoutsq(i)));//i�̋�͐��̋�ɕϊ��������Ă���̂�pt�ł���
			Piece pcj = bp2piece.bp_to_piece(bpwithoutsq(j));//j�ɂ���̂������̋����̋�͏d�v�ɂȂ��Ă���̂Ŋ܂߂Ȃ���΂Ȃ�Ȃ��B
			Square sq1 = bp2sq(i), sq2 = bp2sq(j);//bp1,bp2�̋�̈ʒu
			relative_pp[pti][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8] += diff_;
		}
		absolute_PP[i][j] += diff_;


	}

	void update_lowdim(const Position& pos, const double diff) {

		const auto list1 = pos.evallist();

		const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				lowdim_eachPP(list_fb[i], list_fb[j], diff);
				lowdim_eachPP(list_fw[i], list_fw[j], -diff);
			}
		}
	}

	void add_lowdim() {




	}



#endif //  LOWDIM2

	

};



/*
�����������ꂽ�v�f�B
PP�ip0,p1�j=PP���(���E�Ώ́A��ԑΏ�)+PP����(���s�ړ��A��ԑΏ�)�{P��΁i���E�Ώ́A��ԑΏہj
*/
struct lowerDimPP
{
	double absolute_pp[fe_end2][fe_end2];//���PP�@
	double relative_pp[PC_ALL][PC_ALL][17][17];//����PP �Z�I�ł�y���W�Œ葊��PP�����������ǂ悭�킩���̂ŏȂ�
	//double absolute_p[fe_end2];//���P 
	void clear() {

		memset(absolute_pp, 0, sizeof(absolute_pp));
		memset(relative_pp, 0, sizeof(relative_pp));

	}
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

	//����Ȃ�ł����̂��H
	h = std::abs(int(mt())) % 3;

	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			//bonanza��4���ǒ��ɑ΂��Ă��̒l�Ȃ̂�min batch�������Ƃ��͂���ł͂��߁I�I�I�I�I
#if 0
			/*if (PP[i][j]>0) { data.dJ[i][j] -= double(0.2 / double(FV_SCALE)); }
			else if (PP[i][j]<0) { data.dJ[i][j] += double(0.2 / double(FV_SCALE)); }*/
#endif
			int inc = h*sign(data.absolute_PP[i][j]);
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

			data.absolute_PP[il][jl] = data.absolute_PP[ir][jr] = (data.absolute_PP[il][jl] + data.absolute_PP[ir][jr]);
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


�w�K�p�̊�t�ň�v����}��B�w�K��i�߂�Ɖߊw�K���N�����͂��ł���̂ł���Ŋw�K�Ƀo�O���Ȃ����ǂ����m���߂�

*/
double concordance() {
	/*std::random_device rd;
	std::mt19937 t_mt(rd());*/
	//std::shuffle(testset.begin(), testset.end(), t_mt);//�V���b�t�������Ă邪���Ȃ��ق��������H�H

	int num_tests = 500;//500�����Ŋm�F����
	//if (num_tests > testset.size()) { num_tests = testset.size(); }
	//ASSERT(num_tests <= testset.size());
	if (num_tests > games.size()) { num_tests = games.size(); }
	ASSERT(num_tests <= games.size());

	//�����Ő錾�������̂�����ǂ���ł����̂��낤���H
	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;

	int64_t num_concordance_move = 0;
	int64_t num_all_move = 0;

	for (int g = 0; g < num_tests; g++) {

		//auto thisgame = testset[g];//�����w�K�p�̃f�[�^�Ƌ�ʂ��Ă������ق����������H�H
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {

#if 0
			/*if ((float(ply) / float(thisgame.moves.size())) > 0.9) {
				cout << "progress:" << (float(ply) / float(thisgame.moves.size())) << " ply:" << ply << " maxply:" << thisgame.moves.size() << " progress over 90%" << endl;
				goto ERROR_OCCURED;
			}*/
			//��������ELO2800�ȏ�̃\�t�g�ł���΍Ō��20��͉���̘A�����낤...
			if (ply > (thisgame.moves.size() - 20)) {
				goto ERROR_OCCURED;
			}
#endif

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
			if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }
			//�T�����s����pv[0]��teachermove���m�F����B
			
			th.set(pos);
			th.l_alpha = Value_Mated;
			th.l_beta = Value_Mate;
			
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
	/*bonanza�ł�parse2���R�Q��J��Ԃ��炵����ł�����Q�l�ɂ���B
	�w�K�̑����̌��ۂ��i�ނɘA���num_parse2�̒l�����炵�Ă���
	*/
	num_parse2 = 32;
	
#ifdef  SOFTKIFU
	readgames = 35000;
	ifstream gamedata(gamedatabasefile);
 //  SOFTKIFU
#else
	readgames = 70000;
	//ifstream gamedata(fg2800_2ch);
	ifstream gamedata(nichkihu);//�\�t�g�����̐��������ʂ��w�K�����Ȃ����߂�2ch�����݂̂��g�p���Ċw�K���邱�Ƃɂ���B(�Z�I��apery(sdt4������)������Ŋw�K���Ă����͂��Ȃ̂ő��v���Ǝv��)
#endif
	GameDataStream gamedatastream(gamedata);
	
	cout << "readgames " << readgames << endl;
	


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


		std::cout << "iteration" << iteration << "/maxiteration :" << numiteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min ";
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " min" << endl;
		//ofs << "��v�� " << ((maxmoves- huicchi_moves) * 100 / (maxmoves + 1)) << endl;
		//cout << "��v�� " << ((maxmoves - huicchi_moves) * 100 / (maxmoves + 1)) <<" %"<< endl;
		//cout << "calcurate ��v��" << endl;
		double icchiritu = concordance();
		ofs << "��v�� " <<icchiritu << " %" << endl;
		cout << "��v�� " << icchiritu <<" %"<< endl;
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
	for (int g = lock_index_inclement(); g < numgames; g = lock_index_inclement()) {

		//iteration2�œ����ǖʂɑ΂���PV������Ă��܂�Ȃ��悤�ɂ���B
		games[g].other_pv.clear();

		didmoves = 0;
		auto thisgame = games[g];
		pos.set_hirate();
		th.cleartable();
		for (int ply = 0; ply < (thisgame.moves.size() - 1); ply++) {
			diddepth = ply;
			minfo_list.clear();

#if 0
			if ((float(ply) / float(thisgame.moves.size())) > 0.9) {
				//cout <<"progress:"<< (float(ply) / float(thisgame.moves.size()))<<" ply:"<<ply<<" maxply:"<< thisgame.moves.size() <<  " progress over 90%" << endl;
				goto ERROR_OCCURED;
			}
			/*if (ply >(thisgame.moves.size() - 10)) {
				goto ERROR_OCCURED;
			}*/
#endif

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
				//cout << "cant swap" << endl;
				//check_move(teacher_move);
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
			Value record_score;

			for (int move_i = 0; move_i < num_moves; move_i++) {

				si[ply].clear();

				Move m = moves[move_i];
				if (pos.is_legal(m) == false) { continue; }
				if (pos.state()->lastmove == m) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
				didmoves++;
				pos.do_move(m, &si[ply]);
				
				th.set(pos);
				/*---------------------------------------------------------------------------------------------------------
				�����ł�PV�̍쐬�������s����score��PV�Ŗ��[�܂ňړ�������eval()�Ă�ŁA���̒l���g�p�����ق��������H�H�H
				---------------------------------------------------------------------------------------------------------*/
				
				/*=================================
				�T����alpha beta�͌�Ŏ�������
				==================================*/
				if (move_i == 0) {
					th.l_alpha = Value_Mated;
					th.l_beta = Value_Mate;
				}
				else {
					th.l_alpha = record_score-(Value)256;
					th.l_beta = record_score +(Value)256;
				}

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
				//teachermove��alpha beta�ɓ���Ȃ������B�i�l�݂̒l��Ԃ����ꍇ�͂ǂ����悤��....���̂܂܊w�K�Ɏg���̂͂ǂ����Ǝv��...�j
				if ((th.l_alpha<score&&score<th.l_beta)||move_i==0) {
					minfo_list.push_back(MoveInfo(m, th.pv));
				}
				th.pv.clear();//pv�̃N���A��Y��Ă����I�I�I�I�I
				/*else { 
					pos.undo_move(); break; 
				}*/
				pos.undo_move();
				//if (score > teachervalue&&huicchi_firsttime == true) { huicchi_firsttime = false;  lock_huicchimoves_inclement(); }
			}
			ASSERT(minfo_list[0].move == teacher_move);
			games[g].other_pv.push_back(minfo_list);

			//���̋ǖʂ�
			pos.do_move(teacher_move, &si[ply]);

			//�����ł��̋ǖʂ̎w����ɂ��T���I��

			//��������sum_gradJ�̍X�V
			//{



		}
	ERROR_OCCURED:;
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}


}

void learnphase2() {
	
	vector<std::thread> threads(maxthreadnum - 1);//maxthreadnum-1����std::thread�����Ă�B
	cout <<endl<< "parse2" << endl;
	//num_parse2��p�����[�^�[���X�V����B�R����-64����+64�͈͓̔��Ńp�����[�^�[���������ƂɂȂ�B
	//bonanza�ł͂���32��̊Ԃ�dJ���������ɂ���Ăǂ�ǂ�[���ɋ߂Â����Ă���B
	//�l�̍X�V������32�����s���Ă��Ӗ����Ȃ��I�I�I�I�I�I
	//grad�̍쐬����32��s��Ȃ��Ƃ����Ȃ��I�I�I
	for (int i = 0; i < num_parse2; i++) {

		sum_parse2Datas.clear();
		for (auto& parse2 : parse2Datas) {
			parse2.clear();
		}
		

		
		index_ = 0;
		//����w�K�J�n
		for (int i = 0; i < maxthreadnum - 1; ++i) {
			threads[i] = std::thread([i] {learnphase2body(i); });
		}
		learnphase2body(maxthreadnum - 1);


		//����������
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

	//�����o���ǂݍ��݂������ōs���Ēl�̍X�V
#ifndef test_learn
	Eval::param_sym_ij();
	write_PP();
	read_PP();
	
#endif

	
}

void learnphase2body(int number)
{

	//�����Ő錾�������̂�����ǂ���ł����̂��낤���H
	Position pos;
	StateInfo si[500];

	

	//ExtMove moves[600], *end;
	vector<MoveInfo> minfo_list;
	Thread th;
	//end = moves;


	int didmoves = 0;
	int diddepth = 0;
	//============================================================
	//����lockingindexincrement�ɂ���K�v������I�I
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

				//�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[
				//���t��̕]���l�����߂�B
				//�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[
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
					//evalPP�̓R�}������l���Ă��Ȃ�������value�𔽓]�����ĂȂ������I�Ieval�������ׂ�������
					teachervalue = (rootColor == pos.sidetomove())? Eval::eval(pos) : -eval(pos);
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}
				//=-----------------------------------------
				//���t��ȊO�̎w���Ăɑ΂���
				//=-----------------------------------------
				for (int i = 1; i < minfo_list.size(); i++) {


					StateInfo si2[64];
					int j = 0;
					//���t���PV�ŋǖʂ������߂�
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
					//�]���l�Ƌ��t��̍��������B
					const Value score = (rootColor == pos.sidetomove()) ? Eval::eval(pos) : -eval(pos);
					const Value diff = score - teachervalue;
					double diffsig = dsigmoid(diff);
					diffsig = (rootColor == BLACK ? diffsig : -diffsig);

					sum_diff += diffsig;
					objective_function += sigmoid(diff);

					parse2Datas[number].gradJ.update_dJ(pos, -diffsig);//pv�̐�[�ileaf node�j�Ɉړ�����dJ���v�Z����̂�TD-leaf�Ƃ������ƁH�H
																	   //�ǖʂ�߂�
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}
					pos.undo_move();
				}//end of (���t��ȊO�̎w���Ăɑ΂���)

				//���t���i�߂��ǖʂł�gradJ�̍X�V
				{
					StateInfo si2[64];//�ő�ł��[���R

					for (int i = 0; i < 64; i++) si2[i].clear();

					int j = 0;
					//���t��ɑ΂���dJ/dvi���X�V
					if (pos.is_legal(minfo_list[0].move) == false) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					if (pos.state()->lastmove == minfo_list[0].move) { cout << games[g].black_P << " " << games[g].white_P; ASSERT(0); }
					pos.do_move(minfo_list[0].move, &si[ply]);
					//pv�̎w���肪�񍇖@��ɂȂ��Ă��鎖������H�H
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
			}//���z�v�Z
		skip_calc:;
			 //���̋ǖʂ�
			pos.do_move(teacher_move, &si[ply]);

		}
	ERROR_OCCURED:;
		//cout << "game number: " << g << "/ maxgamenum: " << numgames << " didmoves " << didmoves << " diddepth " << diddepth << endl;
		//lock_maxmoves_inclement(diddepth);
	}


}

//gradJ�͊��ɍ��E�Ώ̐�����������Ă�����̂Ƃ���B
//(bp1,bp2)�v�f�ɂ��Ď����������s��
/*
���������ɂ��đS�R�킩��Ȃ��̂Œ����ŃR�[�h�����Ă�i���ꂪ��Ԃ����Ȃ��C������j
�i�܂����������̗��O���炢�����Ɨ������Ă������ق��������̂ł͂Ȃ����H�j
�܂����ǂ�Ȃ��̂������ŏ����Ă݂��ق����ق��̐l�̏������R�[�h�������Ƃ���������̂������̂ł܂���񏑂��Ă݂�B
*/
//
//void each_PP(lowerDimPP & lowdim, const dJValue& gradJ,const BonaPiece bp1,const BonaPiece bp2) {
//	if (bp1 == bp2) { return; }//��v����ꏊ��evaluate�Ō��Ȃ��̂�gradJ��0�ɂȂ��Ă���B����͖������Ă����B
//	/*Piece pc;
//	Square sq;*/
//	//���PP
//	lowdim.absolute_pp[bp1][bp2]=gradJ.dJ[bp1][bp2];
//
//	//���P
//	lowdim.absolute_p[bp1] += gradJ.dJ[bp1][bp2];
//	lowdim.absolute_p[bp2] += gradJ.dJ[bp1][bp2];
//	
//	//����PP�i�Տ�̋���j
//	if (bp1 >= fe_hand_end&&bp2 >= fe_hand_end) {
//		Piece pc1 = bp2piece.bp_to_piece(bpwithoutsq(bp1)), pc2 = bp2piece.bp_to_piece(bpwithoutsq(bp2));//bp1,bp2�̋��
//		Square sq1=bp2sq(bp1), sq2=bp2sq(bp2);//bp1,bp2�̋�̈ʒu
//		lowdim.relative_pp[pc1][pc2][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8]+= gradJ.dJ[bp1][bp2];
//	}
//}
//
//
////���̊֐��Ŏ����������ꂽ�l��gradJ�ɐD�荞��ł����B�iweave�F�D�荞�ށj
//void weave_eachPP(dJValue& newgradJ, const lowerDimPP& lowdim, const BonaPiece bp1, const BonaPiece bp2) {
//	if (bp1 == bp2) { return; }//��v����ꏊ��evaluate�Ō��Ȃ��̂�gradJ��0�ł����B����͖������Ă����B
//	//���PP
//	newgradJ.dJ[bp1][bp2] = lowdim.absolute_pp[bp1][bp2];
//
//	//���P
//	newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[bp1];
//	newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[bp2];
//
//	//����PP�i�Տ�̋���j
//	if (bp1 >= fe_hand_end&&bp2 >= fe_hand_end) {
//		Piece pc1=bp2piece.bp_to_piece(bpwithoutsq(bp1)), pc2 = bp2piece.bp_to_piece(bpwithoutsq(bp2));//bp1,bp2�̋��
//		Square sq1=bp2sq(bp1), sq2=bp2sq(bp2);//bp1,bp2�̋�̈ʒu
//		newgradJ.dJ[bp1][bp2]+=lowdim.relative_pp[pc1][pc2][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8];//���s�ړ�
//	}
//}


void each_PP(lowerDimPP & lowdim, const dJValue& gradJ, const BonaPiece bp1, const BonaPiece bp2) {
	if (bp1 == bp2) { return; }//��v����ꏊ��evaluate�Ō��Ȃ��̂�gradJ��0�ɂȂ��Ă���B����͖������Ă����B

	//i��j�̑O��ɂ���ĈႤ�ꏊ���Q�Ƃ��Ă��܂��̂�h���B
	BonaPiece i=std::max(bp1,bp2), j=std::min(bp1,bp2);

	double grad = gradJ.absolute_PP[bp1][bp2];


	//��i�͐��̋�ɒu��������B�i���̋���̋�ǂ���̏ꍇ������index�̑g�Ƃ��Ď�舵���ׁB��ԑΏ̂ɂ�鎟�������B�j
	//���̋�ɒu�������邽�߁A�������ꂪ���̋�ł���΁A�����͔��]�����Ȃ���΂Ȃ�Ȃ��B
	/*Piece pci = bp2piece.bp_to_piece(bpwithoutsq(i));
	ASSERT(pci != NO_PIECE);
	if (piece_color(pci) == WHITE) {
		grad = -grad;
		i = inversebonapiece(i);
		j = inversebonapiece(j);
	}
	ASSERT(piece_color(bp2piece.bp_to_piece(bpwithoutsq(i))) == BLACK);
*/
	//����PP
	if (bp2sq(i) != Error_SQ&&bp2sq(j) != Error_SQ) {
		Piece pci = (bp2piece.bp_to_piece(bpwithoutsq(i)));//i�̋�͐��̋�ɕϊ��������Ă���̂�pt�ł���
		Piece pcj = bp2piece.bp_to_piece(bpwithoutsq(j));//j�ɂ���̂������̋����̋�͏d�v�ɂȂ��Ă���̂Ŋ܂߂Ȃ���΂Ȃ�Ȃ��B
		Square sq1 = bp2sq(i), sq2 = bp2sq(j);//bp1,bp2�̋�̈ʒu
		lowdim.relative_pp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8] += grad;
	}


	//���PP
	lowdim.absolute_pp[i][j] += grad;
	//���P
	/*lowdim.absolute_p[i]+= gradJ.dJ[bp1][bp2];
	lowdim.absolute_p[j]+=gradJ.dJ[bp1][bp2];*/
}

void weave_eachPP(dJValue& newgradJ, const lowerDimPP& lowdim, const BonaPiece bp1, const BonaPiece bp2) {

	if (bp1 == bp2) { return; }//��v����ꏊ��evaluate�Ō��Ȃ��̂�gradJ��0�ɂȂ��Ă���B����͖������Ă����B

	int sign = 1;
							   //i��j�̑O��ɂ���ĈႤ�ꏊ���Q�Ƃ��Ă��܂��̂�h���B
	BonaPiece i = std::max(bp1, bp2), j = std::min(bp1, bp2);

	//��i�͐��̋�ɒu��������B�i���̋���̋�ǂ���̏ꍇ������index�̑g�Ƃ��Ď�舵���ׁB��ԑΏ̂ɂ�鎟�������B�j
	/*Piece pci = bp2piece.bp_to_piece(bpwithoutsq(i));
	ASSERT(pci != NO_PIECE);
	if (piece_color(pci) == WHITE) {
		i = inversebonapiece(i);
		j = inversebonapiece(j);
	}
	ASSERT(piece_color(bp2piece.bp_to_piece(bpwithoutsq(i))) == BLACK);*/

	//����PP(���s�ړ�)
	if (bp2sq(i) != Error_SQ&&bp2sq(j) != Error_SQ) {
		Piece pci = (bp2piece.bp_to_piece(bpwithoutsq(i)));//i�̋�͐��̋�ɕϊ��������Ă���̂�pt�ł���
		Piece pcj = bp2piece.bp_to_piece(bpwithoutsq(j));//j�ɂ���̂������̋����̋�͏d�v�ɂȂ��Ă���̂Ŋ܂߂Ȃ���΂Ȃ�Ȃ��B
		Square sq1 = bp2sq(i), sq2 = bp2sq(j);//bp1,bp2�̋�̈ʒu
		newgradJ.absolute_PP[bp1][bp2] += lowdim.relative_pp[pci][pcj][sqtofile(sq1) - sqtofile(sq2) + 8][sqtorank(sq1) - sqtorank(sq2) + 8];
	}
	//���PP
	newgradJ.absolute_PP[bp1][bp2] += lowdim.absolute_pp[i][j];
	//���P
	//newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[i];
	//newgradJ.dJ[bp1][bp2] += lowdim.absolute_p[j];
}



void lower__dimPP(lowerDimPP & lowdim,const dJValue& gradJ)
{
	

	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
			each_PP(lowdim, gradJ, bp1, bp2);
		}
	}
}

//�����������ꂽ�l�����Ƃɂ���gradJ���\�z����B
void weave_lowdim_to_gradj(dJValue& newgradJ, const lowerDimPP& lowdim) {

	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
			weave_eachPP(newgradJ, lowdim, bp1, bp2);
		}
	}
}

#endif//learn