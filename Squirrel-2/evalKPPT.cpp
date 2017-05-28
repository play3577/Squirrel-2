

#include "evaluate.h"
#include "usi.h"
#include "position.h"
#include <fstream>
#include <iostream>

#ifdef EVAL_KPPT
namespace Eval {
	std::array<int16_t, 2> KPP[SQ_NUM][fe_end][fe_end];
	std::array<int32_t, 2> KKP[SQ_NUM][SQ_NUM][fe_end];
	std::array<int32_t, 2> KK[SQ_NUM][SQ_NUM];

	void read_FV() {

		std::ifstream ifsKK(Options["KK"].str().c_str(), std::ios::binary);
		if (ifsKK) ifsKK.read(reinterpret_cast<char*>(KK), sizeof(KK));
		else ASSERT(0);

		std::ifstream ifsKKP(Options["KKP"].str().c_str(), std::ios::binary);
		if (ifsKKP) ifsKKP.read(reinterpret_cast<char*>(KKP), sizeof(KKP));
		else ASSERT(0);


		std::ifstream ifsKPP(Options["KPP"].str().c_str(), std::ios::binary);
		if (ifsKPP) ifsKPP.read(reinterpret_cast<char*>(KPP), sizeof(KPP));
		else ASSERT(0);

		cout << "success read fv" << endl;
	};
	void write_FV() {

		std::ofstream ofsKK(Options["KK"].str().c_str(), std::ios::binary);
		if (ofsKK) ofsKK.write(reinterpret_cast<char*>(KK), sizeof(KK));
		else ASSERT(0);

		std::ofstream ofsKKP(Options["KKP"].str().c_str(), std::ios::binary);
		if (ofsKKP) ofsKKP.write(reinterpret_cast<char*>(KKP), sizeof(KKP));
		else ASSERT(0);


		std::ofstream ofsKPP(Options["KPP"].str().c_str(), std::ios::binary);
		if (ofsKPP) ofsKPP.write(reinterpret_cast<char*>(KPP), sizeof(KPP));
		else ASSERT(0);

	};

	//全計算
	/*
	Aperyではmateralもp[2][0]に格納していたがめんどくさいのでmaterialは別で足そうかな...
	*/
	Value eval_ALLKPPT(const Position & pos)
	{
		const Square bksq = pos.ksq(BLACK);
		const Square wksq = pos.ksq(WHITE);

		auto listfb = pos.evallist().bplist_fb;//bplist_fbの先頭
		auto listfw = pos.evallist().bplist_fw;

		const auto* ppbkpp = KPP[bksq];//bkppへのポインタへのポインタ
		const auto* ppwkpp = KPP[hihumin_eye(wksq)];
		
		int i, j;
		BonaPiece bp1fb, bp1fw, bp2fb, bp2fw;
		
		EvalSum sum;//ここに計算した情報を格納する
		//sumの初期化
		sum.p[0][0] = 0;
		sum.p[0][1] = 0;
		sum.p[1][0] = 0;
		sum.p[1][1] = 0;
		sum.p[2] = KK[bksq][wksq];
		//loop 開始を i = 1 からにして、i = 0 の分のKKPを先に足す。
		sum.p[2] += KKP[bksq][wksq][listfb[0]];
		for (i = 1; i < 38; ++i) {
			
			bp1fb = listfb[i];
			bp1fw = listfw[i];
			const auto* pbkpp = ppbkpp[bp1fb];
			const auto* pwkpp = ppwkpp[bp1fw];

			for (j = 0; j < i; ++j) {

				bp2fb = listfb[j];
				bp2fw = listfw[j];
				sum.p[0] += pbkpp[bp2fb];
				sum.p[1] += pwkpp[bp2fw];

			}
			sum.p[2] += KKP[bksq][wksq][bp1fb];

		}
		return Value();
	}

	Value eval(const Position & pos)
	{
		return Value();
	}

}
#endif