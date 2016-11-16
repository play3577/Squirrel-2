#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>
#include "position.h"

using namespace Eval;
using namespace std;

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

			PP[bp1][bp2]=PP[bp2][bp1]= int32_t(mt());

		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}
//
////�p�����[�^�[�̍X�V�̂��߂̊֐�
//void Eval::add_PP(const Position & pos, const int32_t ** inc) {
//
//	//������ list_fb��������Ȃ���list_fw���p�ӂ��Ȃ��Ƃ��߂����...
//
//	auto list1 = pos.evallist();
//
//	BonaPiece *list_fb = list1.bplist_fb, *list_fw;
//
//	for (int i = 0; i < 40; i++) {
//		for (int j = 0; j < i; j++) {
//			PP[list_fb[i]][list_fb[j]] += inc[list_fb[i]][list_fb[j]] * FV_SCALE;
//			PP[list_fw[i]][list_fw[j]] += inc[list_fw[i]][list_fw[j]] * FV_SCALE;
//			//PP�Ώ̐����l����
//			PP[list_fb[j]][list_fb[i]] += inc[list_fb[j]][list_fb[i]] * FV_SCALE;
//			PP[list_fw[j]][list_fw[i]] += inc[list_fw[j]][list_fw[i]] * FV_SCALE;
//		}
//	}
//	//�����o������ǂݍ��ނ��ƂŒl���X�V����i�����ł��Ȃ��ق���������������Ȃ��j
//	/*write_PP();
//	read_PP();*/
//}
