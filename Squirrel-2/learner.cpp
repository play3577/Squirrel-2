#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>
#include <vector>
#include "position.h"
#include "game_database.h"
#include "makemove.h"
using namespace Eval;
using namespace std;


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

			PP[bp1][bp2]=PP[bp2][bp1]= int32_t(mt())%1000;

		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}


//�p�����[�^�[�̍X�V�̂��߂̊֐�
void Eval::renewal_PP(const Position & pos, const int32_t ** inc) {

	//������ list_fb��������Ȃ���list_fw���p�ӂ��Ȃ��Ƃ��߂����...

	auto list1 = pos.evallist();

	BonaPiece *list_fb = list1.bplist_fb, *list_fw=list1.bplist_fw;

	for (int i = 0; i < 40; i++) {
		for (int j = 0; j < i; j++) {
			PP[list_fb[i]][list_fb[j]] += inc[list_fb[i]][list_fb[j]] * FV_SCALE;
			PP[list_fw[i]][list_fw[j]] += inc[list_fw[i]][list_fw[j]] * FV_SCALE;
			//PP�Ώ̐����l����
			PP[list_fb[j]][list_fb[i]] += inc[list_fb[j]][list_fb[i]] * FV_SCALE;
			PP[list_fw[j]][list_fw[i]] += inc[list_fw[j]][list_fw[i]] * FV_SCALE;
		}
	}
	//�����o������ǂݍ��ނ��ƂŒl���X�V����i�����ł��Ȃ��ق���������������Ȃ��j
	/*write_PP();
	read_PP();*/
}


//�w�K�p�֐�
void Eval::learner(Thread & th)
{
	//������
	const int numgames = 50;
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

	for (int g = 0; g < numgames; g++) {

		auto thisgame = games[g];
		pos.set_hirate();
		for (int ply = 0; ply < thisgame.ply; ply++) {


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

			//���̂�����ŋǖʂɑ΂���덷���v�Z���Ȃ��Ƃ����Ȃ�


			pos.do_move(teacher_move, &si[ply]);
		}// for gameply
	}//for numgames

	//����������z���v�Z���ē����x�N�g���̍X�V


}

