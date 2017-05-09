#pragma once

#include "evaluate.h"
#include <fstream>
#include "Thread.h"
#include "evaluate.h"

using namespace Eval;
namespace Eval {

	void write_PP();

	void initialize_PP();


	//�p�����[�^�[�̍X�V�̂��߂̊֐�
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


#define JIGENSAGE//ok�����Ȃ��Ă�

#define LR//ok�����Ȃ��Ă�


#if defined(EVAL_PP)

struct lowerDimPP
{
	double absolute_pp[fe_end2][fe_end2];//���PP�@
	double relative_pp[PC_ALL][PC_ALL][17][17];//PP���s�ړ��ix,y���W�Œ�̊�^��傫������relativePP�̂ق��͊�^�����������ׂ����H�j
	double relative_ypp[PC_ALL][PC_ALL][17][17][Rank_Num];//y���W�Œ�PP���s�ړ��@rank�ɂ�i��rank���i�[�����
	double relative_xpp[PC_ALL][PC_ALL][17][17][File_Num];//x���W�Œ�
														  //double absolute_p[fe_end2];//���P ����͂��Ԃ�_��

														  //�ǂꂾ���������l�ł��l���t���Ă����PP�𓮂����̂Ńy�i���e�B���d�v�ɂȂ��Ă��邩....
	double absolute_pe[fe_end2][ColorALL][SQ_NUM][SQ_NUM];//[bonap][�����̌����ƂȂ�����̐F][�����̂���}�X][�����̌����ƂȂ��̈ʒu]  ppe���ƂȂ�̎��������ɂ��Ȃ�Ȃ��̂�pe�@��̈ʒu���ǉ�����

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
				//PP�Ώ̐����l����
				absolute_PP[list_fb[j]][list_fb[i]] += diff;
				absolute_PP[list_fw[j]][list_fw[i]] -= diff;
#ifdef LR

				/*
				���E�Ώ̐����l����@������Ɖ�����....
				*/
				if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//�����Ƃ��Տ�
					absolute_PP[sym_rightleft(list_fb[i])][sym_rightleft(list_fb[j])] += diff;
					absolute_PP[sym_rightleft(list_fw[i])][sym_rightleft(list_fw[j])] -= diff;

					absolute_PP[sym_rightleft(list_fb[j])][sym_rightleft(list_fb[i])] += diff;
					absolute_PP[sym_rightleft(list_fw[j])][sym_rightleft(list_fw[i])] -= diff;
				}
				else if (bp2sq(list_fb[i]) != Error_SQ&&bp2sq(list_fb[j]) == Error_SQ) {
					//i���Տ�
					absolute_PP[sym_rightleft(list_fb[i])][(list_fb[j])] += diff;
					absolute_PP[sym_rightleft(list_fw[i])][(list_fw[j])] -= diff;

					absolute_PP[(list_fb[j])][sym_rightleft(list_fb[i])] += diff;
					absolute_PP[(list_fw[j])][sym_rightleft(list_fw[i])] -= diff;
				}
				else if (bp2sq(list_fb[i]) == Error_SQ&&bp2sq(list_fb[j]) != Error_SQ) {
					//j���Տ�
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
	

	void clear() {
		memset(this, 0, sizeof(*this));
	}
};

struct  dJValue
{
	


	void clear() { memset(this, 0, sizeof(*this)); }


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
