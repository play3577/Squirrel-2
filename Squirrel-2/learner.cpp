#define _CRT_SECURE_NO_WARNINGS

#include "learner.h"
#include <random>

using namespace Eval;
using namespace std;

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

	std::random_device rd;
	std::mt19937 mt(rd());

	for (BonaPiece bp1 = f_hand_pawn; bp1 < fe_end2; bp1++) {
		for (BonaPiece bp2 = f_hand_pawn; bp2 < fe_end2; bp2++) {
			PP[bp1][bp2]= int32_t(mt());
		}
	}
	write_PP();
	cout << "initialize param PP!" << endl;
}
