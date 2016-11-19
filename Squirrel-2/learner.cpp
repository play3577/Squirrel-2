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

//dJ/dvi��dJ�z��ɂ��ꂼ��i�[���Ă����B
void update_dJ(const Position pos, const double diff) {

	const auto list1 = pos.evallist();

	const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < i; j++) {
			dJ[list_fb[i]][list_fb[j]] += diff;
			dJ[list_fw[i]][list_fw[j]] += diff;
			//PP�Ώ̐����l����
			dJ[list_fb[j]][list_fb[i]] += diff;
			dJ[list_fw[j]][list_fw[i]] += diff;
		}
	}
}


 int sign(const double d) {
	 return (d > 0) ? 1 : -1;
}


//���t��̎w�����moves�z��̈�Ԑ擪�Ɏ����Ă��邽�߂̊֐�
//moves�z��̐擪�|�C���^�@num�v�f���@m���t��̎w����
//�z��̒��ɋ��t�肪�܂܂�Ȃ��ꍇfalse��Ԃ�
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
	// �t�@�C���ւ̏����o���i�����p�X��usioption�ŕύX�ł���悤�ɂ���B�j
	FILE* fp = std::fopen("c:/book2/fv_PP.bin", "wb");
	std::fwrite(&PP, sizeof(PP), 1, fp);
	std::fclose(fp);

}

void Eval::read_PP() {

	//�����p�X��usioption�ŕύX�ł���悤�ɂ���B
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
	int h;

	//����Ȃ�ł����̂��H
	h =  std::abs(int8_t(mt())) % 2 + 1;

	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {
			PP[i][j] += h*sign(dJ[i][j]);
		}
	}
	//�����o������ǂݍ��ނ��ƂŒl���X�V����
	write_PP();
	read_PP();
}


//�w�K�p�֐�
void Eval::learner(Thread & th)
{
	//������
	const int numgames = 50;
	const int numiteration = 100;
	ifstream gamedata(gamedatabasefile);
	GameDataStream gamedatastream(gamedata);
	std::vector<Game> games;
	

	//������ǂݍ���
	for (int i = 0; i < numgames; ++i) {
		Game game;
		if (gamedatastream.read_onegame(&game)) {
			games.push_back(game);
		}
	}

	//�[�[�[�[�[�[�[�[�[�[�[�[�[�����܂ł����Ɠ��������Ƃ��m�F�ς�



	Position pos;
	StateInfo si[256];
	ExtMove moves[600],*end;
	vector<MoveInfo> minfo_list;
	//Thread th;
	end = moves;

	for (int iteration = 0; iteration < numiteration; iteration++) {
		//iteration�̍ŏ��ɏ���������i����ȊO�ł͂��Ă͂����Ȃ��j
		memset(dJ, 0, sizeof(dJ));

		for (int g = 0; g < numgames; g++) {

			auto thisgame = games[g];
			pos.set_hirate();
			for (int ply = 0; ply < thisgame.ply; ply++) {
				minfo_list.clear();

				//���̋ǖʂ̍�����𐶐�����
				end = test_move_generation(pos, moves);
				ptrdiff_t num_moves = end - moves;


				//�����̍�������w���胊�X�g�̈�ԍŏ��Ɏ����Ă���B
				Move teacher_move = thisgame.moves[ply];
				if (!swapmove(moves, int(num_moves), teacher_move)) { break; }


				//������ɑ΂��ĒT�����s���B�T����������T������PV�Ǝw����ƒT�������]���l���y�A�ɂ��Ċi�[
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
				//�����ł��̋ǖʂ̎w����ɂ��T���I��

				{


					/*
					���t��ł͂Ȃ��w���Ăɂ��ǖʂɌ��������x�N�g���͉��l��Ⴍ�A
					���t��ɂ��ǖʂɌ��������x�N�g���͉��l�������A
					�����ɏo�Ă�������x�N�g���ɑ΂��Ă͉��l�𓮂����Ȃ��悤�ɂ������B

					���̂��߂�sum_diff�B
					*/
					double sum_diff = Value_Zero;

					//���t��ȊO�̎w���Ăɑ΂���
					for (size_t i = 1; i < minfo_list.size(); i++) {

						//�]���l�Ƌ��t��̍��������B
						const Value diff = minfo_list[i].score - minfo_list[0].score;

						//���t��Ƃ��܂�ɂ����ꂽ���l�̎w���Ẳe�������Ȃ����邽�߂�dsigmoid�ɓ����̂��ƍl���Ă���B
						/*
						�R���s���[�^�[���������҂���˂��牤�u���O�̓������long distance library�̘b�����ł������̂��߂ɂȂ�Ȃ���,
						sfen�̂悤�ȊȒP������b�𕷂��Ă������̂��߂ɂ͂Ȃ�Ȃ��B

						�����ɂ��������x���̎w���肩��w�K����̂��ǂ��̂ł͂Ȃ����ƍl������Ƃ������Ƃ��Ǝv���Ă���B
						*/
						const double diffsig = dsigmoid(diff);
						sum_diff += diffsig;

						StateInfo si2[3];//�ő�ł��[���R
						int j = 0;
						for (Move m : minfo_list[i].pv) {
							pos.do_move(m, &si2[j]);
							j++;
						}
						update_dJ(pos, -diffsig);//pv�̐�[�ileaf node�j�Ɉړ�����dJ���v�Z����̂�TD-leaf�Ƃ������ƁH�H
						for (int jj = 0; jj < j; jj++) {
							pos.undo_move();
						}
					}//end of (���t��ȊO�̎w���Ăɑ΂���)

					StateInfo si2[3];//�ő�ł��[���R
					int j = 0;
					//���t��ɑ΂���dJ/dvi���X�V
					for (Move m : minfo_list[0].pv) {
						pos.do_move(m, &si2[j]);
						j++;
					}
					update_dJ(pos, sum_diff);
					for (int jj = 0; jj < j; jj++) {
						pos.undo_move();
					}


				}//���z�v�Z
				pos.do_move(teacher_move, &si[ply]);
			}// for gameply
		}//for numgames

		//dJ�͑S�w����ƑS�����ɑ΂��đ����グ��ꂽ�̂ł�������bonanza�Ō���parse2
		renewal_PP();


		//�����ŉ����w����̈�v���݂����ȏ���\���ł���΂����񂾂����...
		std::cout << "iteration" << iteration+1 << std::endl;


	}//for num iteration
	


}

