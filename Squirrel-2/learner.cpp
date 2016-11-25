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
//#define USE_PENALTY

double dJ[fe_end2][fe_end2];

//dJ/dvi��dJ�z��ɂ��ꂼ��i�[���Ă����B
void update_dJ(const Position pos, const double diff) {

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

//�R������d==0�̏ꍇ�̓����ʂ��ǂ�ǂ�}�C�i�X�ɑ傫���Ȃ��Ă��܂�(�C���ς�)�@�ƌ������o�Ă��Ȃ������ʂ�sparse�ɂ��Ă��������B
 int sign(const double d) {
	 return (d > 0) ? 1 : (d<0)?-1:0;
}

//���t��̎w�����moves�z��̈�Ԑ擪�Ɏ����Ă��邽�߂̊֐�
//moves�z��̐擪�|�C���^�@num�v�f���@m���t��̎w����
//�z��̒��ɋ��t�肪�܂܂�Ȃ��ꍇfalse��Ԃ�
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
	// �t�@�C���ւ̏����o���i�����p�X��usioption�ŕύX�ł���悤�ɂ���B�j
	FILE* fp = std::fopen(Options["eval"].str().c_str(), "wb");
	std::fwrite(&PP, sizeof(PP), 1, fp);
	std::fclose(fp);

}

void Eval::read_PP() {

	//�����p�X��usioption�ŕύX�ł���悤�ɂ���B
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


//PP�𗐐��ŏ���������
void Eval::initialize_PP()
{
	string YorN;
	cout << "do you really wanna initialize feature vector? [y/n]" << endl;
	cin >> YorN;

	if (YorN != "y") { cout << "I don't initialize." << endl; return; }
	memset(PP, 0, sizeof(PP));
	std::random_device rd;
	std::mt19937 mt(rd());
	//�������͑Ώ̐����l���Ȃ��点�˂΂Ȃ�Ȃ�(�Ƃ����������ŏ���������K�v���邩�H�H)
	for (BonaPiece bp1 = f_hand_pawn; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = f_hand_pawn; bp2 < bp1; bp2++) {

			int a = int32_t(mt()) % 100;
			PP[bp1][bp2]=PP[bp2][bp1]= a;

		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}


//�p�����[�^�[�̍X�V�̂��߂̊֐�
void renewal_PP() {

	std::random_device rd;
	std::mt19937 mt(rd());

	//h�͂Ȃ�ڂ��炢�̒l�ɐݒ肷��΂������H�H(bonanza�ł�0,1,2�̒l����闐���ɂ��Ă���炵���B�i���{�U)
	//�R���͊w�K�̐i�s�x�ɍ��킹�ĕω�������K�v������Ǝv���̂���...�B
	//�ŏ��͑�G�c�ɂ��񂾂񐳊m�ɓ������悤�ɕω�������B
	//bonanza��pharse 2��32��J��Ԃ��炵���̂ł��̉񐔂𑹎����������Ȃ��Ă������Ɍ��炵�Ă����΂����ƍl������B
	int h;

	//����Ȃ�ł����̂��H
	h =  std::abs(int8_t(mt())) % 3;

	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

#ifdef USE_PENALTY
			/*
			�y�i���e�B�������邱�ƂŒl��PP[i][j]����傫������邱�Ƃ͂Ȃ��Ȃ�
			�������ǂꂮ�炢�̑傫���̃y�i���e�B����p�ӂ���΂������̂�...

			*/
			if (PP[i][j]>0) { dJ[i][j] - (double)0.002; }
			else if (PP[i][j]<0) { dJ[i][j] + (double)0.002; }
#endif

			int inc = h*sign(dJ[i][j]);
			PP[i][j] += inc;
		}
	}
	//�����o������ǂݍ��ނ��ƂŒl���X�V����@������32��������o���������݂��s���͖̂��ʍŌ�ɂ܂Ƃ߂čs��
	//write_PP();
	//read_PP();
	
}


//�w�K�p�֐�
void Eval::learner()
{
	//������
	int readgames = 2000;
	const int numgames = 200;
	const int numiteration = 20;
	/*bonanza�ł�parse2���R�Q��J��Ԃ��炵����ł�����Q�l�ɂ���B
	�w�K�̑����̌��ۂ��i�ނɘA���num_parse2�̒l�����炵�Ă���
	*/
	const int num_parse2 = 32;
	ifstream gamedata(gamedatabasefile);
	GameDataStream gamedatastream(gamedata);
	std::vector<Game> games;
	
	int didmoves = 0;
	int diddepth = 0;

	//�ړI�֐��B�R�����������Ȃ��Ă���Ίw�K���i��ł���Ƃ݂Ȃ���B(�͂����I)
	double objective_function = 0;


	//������ǂݍ���
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
	filepath = "c:/book2/log/" + str + ".txt";
	ofstream ofs(filepath);
	if (ofs.fail()) { cout << "log fileError"; }

#endif


	Position pos;
	StateInfo si[256];
	ExtMove moves[600],*end;
	vector<MoveInfo> minfo_list;
	Thread th;
	end = moves;


	//�w�K���ɖ������[�v�Ɋׂ��Ă��܂����Ƃ��������B�ǂ��ŋN�������̂��H
	//�������[�v�Ɋׂ��Ă����킯�ł͂Ȃ����������T���ɂ��Ȃ�̎��Ԃ��₵�Ă��܂��Ă����B
	//�w�K���͐����T���̐[���𐧌�����H

	for (int iteration = 0; iteration < numiteration; iteration++) {

		//���Ԍv���̊J�n
		limit.starttime = now();

		//�����̃V���b�t��
		//random_shuffle���g���Ɠ������בւ������ɂȂ����̂�shuffle()���g���Ă݂�
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(games.begin(), games.end(),g);

		//iteration�̍ŏ��ɏ���������i����ȊO�ł͂��Ă͂����Ȃ��j
		memset(dJ, 0, sizeof(dJ));
		//�ړI�֐�
		objective_function = 0;


		for (int g = 0; g < numgames; g++) {
			diddepth = 0;
			didmoves = 0;
			

			auto thisgame = games[g];
			pos.set_hirate();
			for (int ply = 0; ply < thisgame.ply; ply++) {
				diddepth = ply;
				minfo_list.clear();

				const Color rootColor = pos.sidetomove();

				//���̋ǖʂ̍�����𐶐�����
				end = test_move_generation(pos, moves);
				ptrdiff_t num_moves = end - moves;


				//�����̍�������w���胊�X�g�̈�ԍŏ��Ɏ����Ă���B
				Move teacher_move = thisgame.moves[ply];
				//if (pos.is_legal(teacher_move) == false) { goto ERROR_OCCURED; }
				//���@�ă��X�g�ɋ��t�肪�����Ă��Ȃ���΂�������您�������Ȃ��Ă��܂��̂ł��̊������g���Ă̊w�T�͂����Œ��f

				//is_ok�ŗ����Ă����͍̂Ō�܂ŗ��p�ł����Ƃ������ƂȂ̂ł��肪����
				if(is_ok(teacher_move)==false){ cout << "is not ok" << endl; goto ERROR_OCCURED; }

				if (!swapmove(moves, int(num_moves), teacher_move)) { 
					cout << "cant swap" << endl;
//					cout << pos << endl;
//					check_move(teacher_move); 
//					//ASSERT(0);
#ifdef LOG
					ofs << " swap error position :" << pos.make_sfen() << " error move " << teacher_move << endl;
#endif
					
					goto ERROR_OCCURED;
				}
				if (pos.is_legal(teacher_move) == false) { cout << "teacher ilegal" << endl; goto ERROR_OCCURED; }

				//������ɑ΂��ĒT�����s���B�T����������T������PV�Ǝw����ƒT�������]���l���y�A�ɂ��Ċi�[
				//����͏����������Ĉ�ǖʍő�i15�肮�炢�ɂ��������������H�j
				//���ǖʂ�����̎w����̐��̐����͂��ׂ��ł͂Ȃ���������Ȃ�
				//bonanza�ł͑S���@��ɑ΂��Ă����Ȃ��Ă����̂�Squirrel�ł����Ȃ����ƂɌ��߂�
				//num_moves = std::min(int(num_moves), 15);
				//cout << "num_moves " << num_moves << endl;
				
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
				//�����ł��̋ǖʂ̎w����ɂ��T���I��

				{


					/*
					���t��ł͂Ȃ��w���Ăɂ��ǖʂɌ��������x�N�g���͉��l��Ⴍ�A
					���t��ɂ��ǖʂɌ��������x�N�g���͉��l�������A
					�����ɏo�Ă�������x�N�g���ɑ΂��Ă͉��l�𓮂����Ȃ��悤�ɂ������B

					���̂��߂�sum_diff�B
					*/
					double sum_diff = Value_Zero;
					if (minfo_list.size() == 0) { cout << "listsize==0" << endl; goto ERROR_OCCURED; }

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
						
						pp��bpp��wpp�ɂ킯����B
						pp�������������bpp��������wpp���グ��Ηǂ��B
						
						���Ղ̂Ƃ���bpp�𒆐S�ɂ��Č��Ă���
						���Ԃ̂Ƃ���wpp�𒆐S�ɂ��Ă݂Ă���B
						�R���Ő��������͂�
						
						*/
						double diffsig = dsigmoid(diff);
						diffsig = (rootColor == BLACK ? diffsig : -diffsig);
						
						sum_diff += diffsig;
						objective_function += sigmoid(diff);


						StateInfo si2[3];//�ő�ł��[���R
						int j = 0;
						//���t���PV�ŋǖʂ������߂�
						pos.do_move(minfo_list[i].move, &si[ply]);
						for (Move m : minfo_list[i].pv) {
							pos.do_move(m, &si2[j]);
							j++;
						}
						update_dJ(pos, -diffsig);//pv�̐�[�ileaf node�j�Ɉړ�����dJ���v�Z����̂�TD-leaf�Ƃ������ƁH�H
						//�ǖʂ�߂�
						for (int jj = 0; jj < j; jj++) {
							pos.undo_move();
						}
						pos.undo_move();
					}//end of (���t��ȊO�̎w���Ăɑ΂���)
					
					
					StateInfo si2[3];//�ő�ł��[���R
					int j = 0;
					//���t��ɑ΂���dJ/dvi���X�V
					if (pos.is_legal(minfo_list[0].move) == false) { ASSERT(0); }
					pos.do_move(minfo_list[0].move, &si[ply]);
					//pv�̎w���肪�񍇖@��ɂȂ��Ă��鎖������H�H
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

				}//���z�v�Z
				
				//���̋ǖʂ�
				pos.do_move(teacher_move, &si[ply]);
			}// for gameply

		ERROR_OCCURED:;
			cout << "game number: " << g << "/ maxgamenum: " << numgames <<" didmoves " << didmoves << " diddepth " << diddepth<<endl;

		}//for numgames

		//dJ�͑S�w����ƑS�����ɑ΂��đ����グ��ꂽ�̂ł�������bonanza�Ō���parse2
		
		//num_parse2��p�����[�^�[���X�V����B�R����-64����+64�͈͓̔��Ńp�����[�^�[���������ƂɂȂ�B
		//bonanza�ł͂���32��̊Ԃ�dJ���������ɂ���Ăǂ�ǂ�[���ɋ߂Â����Ă���B
		for (int i = 0; i < num_parse2; i++) {
			renewal_PP();
		}
		//�����o���ǂݍ��݂������ōs���Ēl�̍X�V
		write_PP();
		read_PP();

		//�����œ��v����\���ł���΂����񂾂����...
		std::cout << "iteration" << iteration<<"/maxiteration :"<<numiteration << " objfunc" << objective_function << std::endl;
#ifdef LOG
		ofs << " iteration " << iteration << " objfunc" << objective_function << " elasped " << (now() - limit.starttime + 1) / (1000 * 60) << " minitus" << endl;
#endif

	}//for num iteration
	

	ofs.close();
}

