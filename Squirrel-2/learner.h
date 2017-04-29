#pragma once

#include "evaluate.h"
#include <fstream>
#include "Thread.h"
#include "evaluate.h"

using namespace Eval;
namespace Eval {

	void write_PP();

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

#ifndef EVAL_PROG
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

#else
struct lowerDimPP
{
	double absolute_pp[fe_end2][fe_end2];//絶対PP　
	double relative_pp[PC_ALL][PC_ALL][17][17];//相対PP
											   //double absolute_p[fe_end2];//絶対P 
	double absolute_ppF[fe_end2][fe_end2];//絶対PP　   終盤用
	double relative_ppF[PC_ALL][PC_ALL][17][17];//相対PP	終盤用

	void clear() {
		memset(this, 0, sizeof(*this));
	}
};
#endif

#define JIGENSAGE//ok強くなってた

#define LR//ok強くなってた



//struct  Parse2data;
#ifndef EVAL_PROG
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
#else
struct  dJValue
{
	double absolute_PP[fe_end2][fe_end2];
	double absolute_PPF[fe_end2][fe_end2];

	void update_dJ(const Position& pos, const double diff) {

		double progress = Progress::calc_prog(pos);

		const double diffo = (1.0 - progress)*diff;
		const double difff = progress*diff;

		const auto list1 = pos.evallist();

		const BonaPiece *list_fb = list1.bplist_fb, *list_fw = list1.bplist_fw;
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				absolute_PP[list_fb[i]][list_fb[j]] += diffo;
				absolute_PP[list_fw[i]][list_fw[j]] -= diffo;
				absolute_PPF[list_fb[i]][list_fb[j]] += difff;
				absolute_PPF[list_fw[i]][list_fw[j]] -= difff;
				//PP対称性を考えて
				absolute_PP[list_fb[j]][list_fb[i]] += diffo;
				absolute_PP[list_fw[j]][list_fw[i]] -= diffo;
				absolute_PPF[list_fb[j]][list_fb[i]] += difff;
				absolute_PPF[list_fw[j]][list_fw[i]] -= difff;



#ifdef LR

				/*
				左右対称性を考える　ちょっと怪しい....
				*/
				if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//両方とも盤上
					absolute_PP[sym_rightleft(list_fb[i])][sym_rightleft(list_fb[j])] += diffo;
					absolute_PP[sym_rightleft(list_fw[i])][sym_rightleft(list_fw[j])] -= diffo;

					absolute_PP[sym_rightleft(list_fb[j])][sym_rightleft(list_fb[i])] += diffo;
					absolute_PP[sym_rightleft(list_fw[j])][sym_rightleft(list_fw[i])] -= diffo;

					absolute_PPF[sym_rightleft(list_fb[i])][sym_rightleft(list_fb[j])] += difff;
					absolute_PPF[sym_rightleft(list_fw[i])][sym_rightleft(list_fw[j])] -= difff;

					absolute_PPF[sym_rightleft(list_fb[j])][sym_rightleft(list_fb[i])] += difff;
					absolute_PPF[sym_rightleft(list_fw[j])][sym_rightleft(list_fw[i])] -= difff;



				}
				else if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) == Error_SQ) {
					//iが盤上
					absolute_PP[sym_rightleft(list_fb[i])][(list_fb[j])] += diffo;
					absolute_PP[sym_rightleft(list_fw[i])][(list_fw[j])] -= diffo;

					absolute_PP[(list_fb[j])][sym_rightleft(list_fb[i])] += diffo;
					absolute_PP[(list_fw[j])][sym_rightleft(list_fw[i])] -= diffo;


					absolute_PPF[sym_rightleft(list_fb[i])][(list_fb[j])] += difff;
					absolute_PPF[sym_rightleft(list_fw[i])][(list_fw[j])] -= difff;

					absolute_PPF[(list_fb[j])][sym_rightleft(list_fb[i])] += difff;
					absolute_PPF[(list_fw[j])][sym_rightleft(list_fw[i])] -= difff;

				}
				else if (bp2sq(list_fb[i]) == Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//jが盤上
					absolute_PP[(list_fb[i])][sym_rightleft(list_fb[j])] += diffo;
					absolute_PP[(list_fw[i])][sym_rightleft(list_fw[j])] -= diffo;

					absolute_PP[sym_rightleft(list_fb[j])][(list_fb[i])] += diffo;
					absolute_PP[sym_rightleft(list_fw[j])][(list_fw[i])] -= diffo;


					absolute_PPF[(list_fb[i])][sym_rightleft(list_fb[j])] += difff;
					absolute_PPF[(list_fw[i])][sym_rightleft(list_fw[j])] -= difff;

					absolute_PPF[sym_rightleft(list_fb[j])][(list_fb[i])] += difff;
					absolute_PPF[sym_rightleft(list_fw[j])][(list_fw[i])] -= difff;

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
				absolute_PPF[bp1][bp2] += data.absolute_PPF[bp1][bp2];
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
