#pragma once

#include "evaluate.h"
#include <fstream>
#include "Thread.h"
#include "evaluate.h"

using namespace Eval;
namespace Eval {

	void write_FV();

	void initialize_PP();


	//パラメーターの更新のための関数
	//void renewal_PP(const Position& pos, const double **dJ);

	void learner();

	void parallel_learner();

	struct MoveInfo {

		Move move;
		vector<Move> pv;

		MoveInfo(const Move m, const vector<Move> pv_) {
			move = m; pv = pv_; 
		}

	};

	


	
}


//#define JIGENSAGE//ok強くなってた

//#define LR//ok強くなってた


#if defined(EVAL_PP)

struct lowerDimPP
{
	double absolute_pp[fe_end2][fe_end2];//絶対PP　
	double relative_pp[PC_ALL][PC_ALL][17][17];//PP平行移動（x,y座標固定の寄与を大きくしてrelativePPのほうは寄与を小さくすべきか？）
	double relative_ypp[PC_ALL][PC_ALL][17][17][Rank_Num];//y座標固定PP平行移動　rankにはiのrankが格納される
	double relative_xpp[PC_ALL][PC_ALL][17][17][File_Num];//x座標固定
														  //double absolute_p[fe_end2];//絶対P これはたぶんダメ

														  //どれだけ小さい値でも値が付いていればPPを動かすのでペナルティが重要になってくるか....
	double absolute_pe[fe_end2][ColorALL][SQ_NUM][SQ_NUM];//[bonap][効きの原因となった駒の色][効きのあるマス][効きの原因となる駒の位置]  ppeだとなんの次元下げにもならないのでpe　駒の位置も追加する

	void clear() {
		memset(this, 0, sizeof(*this));
	}
};





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
				//PP対称性を考えて
				absolute_PP[list_fb[j]][list_fb[i]] += diff;
				absolute_PP[list_fw[j]][list_fw[i]] -= diff;
#ifdef LR

				/*
				左右対称性を考える　ちょっと怪しい....
				*/
				if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//両方とも盤上
					absolute_PP[sym_rightleft(list_fb[i])][sym_rightleft(list_fb[j])] += diff;
					absolute_PP[sym_rightleft(list_fw[i])][sym_rightleft(list_fw[j])] -= diff;

					absolute_PP[sym_rightleft(list_fb[j])][sym_rightleft(list_fb[i])] += diff;
					absolute_PP[sym_rightleft(list_fw[j])][sym_rightleft(list_fw[i])] -= diff;
				}
				else if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) == Error_SQ) {
					//iが盤上
					absolute_PP[sym_rightleft(list_fb[i])][(list_fb[j])] += diff;
					absolute_PP[sym_rightleft(list_fw[i])][(list_fw[j])] -= diff;

					absolute_PP[(list_fb[j])][sym_rightleft(list_fb[i])] += diff;
					absolute_PP[(list_fw[j])][sym_rightleft(list_fw[i])] -= diff;
				}
				else if (bp2sq(list_fb[i]) == Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//jが盤上
					absolute_PP[(list_fb[i])][sym_rightleft(list_fb[j])] += diff;
					absolute_PP[(list_fw[i])][sym_rightleft(list_fw[j])] -= diff;

					absolute_PP[sym_rightleft(list_fb[j])][(list_fb[i])] += diff;
					absolute_PP[sym_rightleft(list_fw[j])][(list_fw[i])] -= diff;
				}

#endif
			}
		}
	}

	void clear() { memset(this, 0, sizeof(*this)); }


	void add(dJValue& data) {

		for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end2; bp1++) {
			for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end2; bp2++) {
				absolute_PP[bp1][bp2] += data.absolute_PP[bp1][bp2];
			}
		}
	}
};

#elif defined(EVAL_KPP)

struct lowerDimPP
{
	float absolute_KPP[82][fe_end][fe_end];
	float absolute_KKP[82][82][fe_end + 1];

	float relative_KPP[82][PC_ALL][PC_ALL][17][17];//相対KPP	相対的になっているのはPPだけKPの方も相対的にできる
	float relative_KKP[82][82][PC_ALL][17];//相対kkp　		相対的になっているのはKPだけKKの方も相対的にできる やっぱ３駒関係はめんどくさいなぁ


	void clear() {
		memset(this, 0, sizeof(*this));
	}
};

struct  dJValue
{
	//エラーはこのKPP KKPを　利用可能にしたら起こることが分かった

	float  absolute_KPP[82][fe_end][fe_end];
	float  absolute_KKP[82][82][fe_end+1];
	int a;

	void clear() { memset(this, 0, sizeof(*this)); }

	void add(dJValue& data) {
		for (Square ksq = SQ_ZERO; ksq <= Square(82); ksq++) {
			//KPP-----------------------------------------------------------
			for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end; bp1++) {
				for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end; bp2++) {
					absolute_KPP[ksq][bp1][bp2] += data.absolute_KPP[ksq][bp1][bp2];
				}
			}
			//KKP-----------------------------------------------------------
			for (Square ksq2 = SQ_ZERO; ksq2 <= Square(82); ksq2++) {
				for (BonaPiece bp3 = BONA_PIECE_ZERO; bp3 < fe_end + 1; bp3++) {
					absolute_KKP[ksq][ksq2][bp3] += data.absolute_KKP[ksq][ksq2][bp3];
				}
			}
		}
	}

	void update_dJ(const Position& pos, const double diff) {

		const auto list1 = pos.evallist();

		const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
		const Square bksq = pos.ksq(BLACK), wksq = pos.ksq(WHITE);

		int i, j;
		BonaPiece bp1_fb, bp1_fw, bp2_fb, bp2_fw;

		//------------------------------------------左右対称とpp対称は後で持たせる
		for (i = 0; i < 38; i++) {

			bp1_fb = list_fb[i];
			bp1_fw = list_fw[i];
			absolute_KKP[bksq][wksq][bp1_fb] += diff;

			for (j = 0; j < i; j++) {
				bp2_fb = list_fb[j];
				bp2_fw = list_fw[j];
				absolute_KPP[bksq][bp1_fb][bp2_fb] += diff;
				absolute_KPP[wksq][bp1_fw][bp2_fw] -= diff;
			}
		}

	}

};

#endif


int sign(const double d);

bool swapmove(ExtMove* moves, const int num, const Move m);

void learnphase1();
void learnphase1body(int number);

void learnphase2();
void learnphase2body(int number);

void renewal_PP(dJValue &data);


void lower__dimPP(lowerDimPP& lowdim, const dJValue& gradJ);
void weave_lowdim_to_gradj(dJValue& newgradJ, const lowerDimPP& lowdim);
