#define _CRT_SECURE_NO_WARNINGS

#include "evaluate.h"
#include "usi.h"
#include "position.h"
#include <random>
#ifdef EVAL_KPP

namespace Eval {

	int16_t kpp[81][fe_end][fe_end];
	int32_t kkp[81][81][fe_end];
	int32_t kk[81][81];

	void read_FV() {
		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "rb");
		if (fp == NULL) { cout << "can't read KPP" << endl;  ASSERT(0); }
		std::fread(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "rb");
		if (fp == NULL) { cout << "can't read KKP" << endl; ASSERT(0); }
		std::fread(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KK"].str().c_str(), "rb");
		if (fp == NULL) { cout << "can't read KK" << endl; ASSERT(0); }
		std::fread(&kk, sizeof(kk), 1, fp);
		std::fclose(fp);

	}

	void write_FV() {

		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "wb");
		std::fwrite(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "wb");
		std::fwrite(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KK"].str().c_str(), "wb");
		std::fwrite(&kk, sizeof(kk), 1, fp);
		std::fclose(fp);
	}
	void init_kpp() {
		std::random_device rd;
		std::mt19937 mt(rd());

		int a;
		FOR_KK(k1, k2) {
			a = int32_t(mt()) % 100;
			kk[k1][k2] =kk[k2][k1]=   a;
			for (int j = 0; j < fe_end; j++) {
				a = int32_t(mt()) % 100;
				kkp[k1][k2][j]=kkp[k2][k1][j] = a;
			}
		}
		FOR_KPP(k, i, j) {
			a = int16_t(mt()) % 32;
			kpp[k][i][j]= kpp[k][j][i] = a;
		}
		/*memset(kpp, 0, sizeof(kpp));
		memset(kkp, 0, sizeof(kkp));
		memset(kk, 0, sizeof(kk));*/


		Eval::write_FV();
	}

	Value eval(const Position& pos) {
		Value value;

		Value material = eval_material(pos);
		Value KPP_KKP_KK;

		StateInfo* now = pos.state();
		StateInfo* prev = now->previous;

		//全計算
		if (now->sumBKPP != Value_error) {
			ASSERT(now->sumWKPP != Value_error &&   now->sumKKP != Value_error);
			KPP_KKP_KK = Value((now->sumBKPP + now->sumWKPP + (now->sumKKP / FV_SCALE_KKP)) / FV_SCALE);
		}
		else if (prev != nullptr && prev->sumBKPP != Value_error) {
			ASSERT(prev->sumWKPP != Value_error &&   prev->sumKKP != Value_error);
			KPP_KKP_KK = eval_diff_KPP(pos);
		}
		else /*if (prev == nullptr || prev->sumBKPP == Value_error || prev->sumWKPP == Value_error || prev->sumKKP == Value_error)*/ {
			KPP_KKP_KK = eval_KPP(pos);
		}


		value = material + KPP_KKP_KK;

#ifdef USETMP
		return (pos.sidetomove() == BLACK) ? value + tempo : -value + tempo;
#else
		return (pos.sidetomove() == BLACK) ? value : -value;
#endif
	}

	Value eval_KPP(const Position& pos) {

		const Square bksq = pos.ksq(BLACK);
		const Square wksq = hihumin_eye(pos.ksq(WHITE));

		BonaPiece* list_fb = pos.evallist().bplist_fb;
		BonaPiece* list_fw = pos.evallist().bplist_fw;

		int i, j;

		BonaPiece k0, k1;
		int32_t bKPP, wKPP, KKP;

		KKP = kk[bksq][wksq];//KK
		bKPP = 0;
		wKPP = 0;

		for (i = 0; i < 38; i++) {

			k0 = list_fb[i];
			k1 = list_fw[i];
			KKP += kkp[bksq][wksq][k0];

			for (j = 0; j < i; j++) {
				bKPP += kpp[bksq][k0][list_fb[j]];
				wKPP -= kpp[wksq][k1][list_fw[j]];
			}
		}

		pos.state()->sumBKPP = bKPP;
		pos.state()->sumWKPP = wKPP;
		pos.state()->sumKKP = KKP;

		return Value((bKPP + wKPP + (KKP / FV_SCALE_KKP)) / FV_SCALE);
	}

	/*
	-------------玉が移動した場合

	黒盤の玉が移動したのであれば
	bkpp kkpを再計算する、
	もし捕獲された駒があればwkppを差分計算

	黒盤の玉が移動したのであれば
	wkpp kkpを再計算する、
	もし捕獲された駒があればbkppを差分計算

	-------------玉以外の駒が移動した場合



	*/
	Value eval_diff_KPP(const Position& pos) {

		auto now = pos.state();
		auto prev = now->previous;

		int KKP, bKPP, wKPP;

		//ここから差分計算開始！！

		KKP = prev->sumKKP;
		bKPP = prev->sumBKPP;
		wKPP = prev->sumWKPP;

		const Square bksq = pos.ksq(BLACK);
		const Square  wksq = hihumin_eye(pos.ksq(WHITE));


		int i, j;

		const Eval::BonaPiece oldbp1_fb = now->dirtybonap_fb[0];
		const Eval::BonaPiece oldbp2_fb = now->dirtybonap_fb[1];
		const Eval::BonaPiece oldbp1_fw = now->dirtybonap_fw[0];
		const Eval::BonaPiece oldbp2_fw = now->dirtybonap_fw[1];
		//swapする可能性があるためここはconstにしない
		Eval::UniformNumber moveduniform1 = now->dirtyuniform[0];//動いた駒
		Eval::UniformNumber moveduniform2 = now->dirtyuniform[1];//捕獲された駒


		const auto list = pos.evallist();

		const Eval::BonaPiece* now_list_fb = list.bplist_fb;
		const Eval::BonaPiece* now_list_fw = list.bplist_fw;

		const Eval::BonaPiece newbp1_fb = list.bplist_fb[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fb = list.bplist_fb[now->dirtyuniform[1]];
		const Eval::BonaPiece newbp1_fw = list.bplist_fw[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fw = list.bplist_fw[now->dirtyuniform[1]];

		const Piece movedpiece = now->DirtyPiece[0];
		const Piece capturedpiece = now->DirtyPiece[1];

		//王が動いた
		if (moveduniform1 >= 38) {
			ASSERT(piece_type(movedpiece) == KING);

			//黒番の玉が動いたのでkkp bkpp全計算
			//動いた駒が2つであればwkppも差分計算する必要がある。
			if (movedpiece == B_KING) {


				bKPP = 0;

				KKP = kk[bksq][wksq];

				for (i = 0; i < 38; i++) {
					int bp1 = now_list_fb[i];

					KKP += kkp[bksq][wksq][bp1];

					for (j = 0; j < i; j++) {
						bKPP += kpp[bksq][bp1][now_list_fb[j]];
					}
				}

				if (moveduniform2 != Eval::Num_Uniform) {
					//wkppなので引くのが+足すのが-
					for (i = 0; i < moveduniform2; ++i) {
						wKPP += kpp[wksq][oldbp2_fw][now_list_fw[i]];
						wKPP -= kpp[wksq][newbp2_fw][now_list_fw[i]];
					}
					for (++i; i < 38; ++i)
					{
						wKPP += kpp[wksq][oldbp2_fw][now_list_fw[i]];
						wKPP -= kpp[wksq][newbp2_fw][now_list_fw[i]];
					}

				}

			}
			//動いたのが白番の玉
			else {
				ASSERT(movedpiece == W_KING);
				wKPP = 0;
				KKP = kk[bksq][wksq];

				//白側とKKPを全計算 
				for (i = 0; i < 38; ++i) {
					int bp1_fb = now_list_fb[i];//oldbp1_fbはKKPで使う
					int bp1_fw = now_list_fw[i];
					KKP += kkp[bksq][wksq][bp1_fb];
					for (j = 0; j < i; j++) {
						wKPP -= kpp[wksq][bp1_fw][now_list_fw[j]];
					}
				}

				//動いた駒が2つあればbkppも差分計算
				if (moveduniform2 != Eval::Num_Uniform) {
					//ASSERT(is_ok(capturedpiece));
					for (i = 0; i < moveduniform2; ++i) {
						bKPP -= kpp[bksq][oldbp2_fb][now_list_fb[i]];
						bKPP += kpp[bksq][newbp2_fb][now_list_fb[i]];
					}
					for (++i; i < 38; ++i)
					{
						bKPP -= kpp[bksq][oldbp2_fb][now_list_fb[i]];
						bKPP += kpp[bksq][newbp2_fb][now_list_fb[i]];
					}

				}
			}
		}
		//-------------------------------玉以外の駒が動いた
		else {


#define ADD_BWKPP(W0,W1,W2,W3) { \
          bKPP -= Eval::kpp[bksq][W0][now_list_fb[i]]; \
          wKPP += Eval::kpp[wksq][W1][now_list_fw[i]]; \
          bKPP += Eval::kpp[bksq][W2][now_list_fb[i]]; \
          wKPP -= Eval::kpp[wksq][W3][now_list_fw[i]]; \
}
			//動いた駒が1つ
			if (moveduniform2 == Eval::Num_Uniform) {

				//kkp差分
				KKP -= kkp[bksq][wksq][oldbp1_fb];
				KKP += kkp[bksq][wksq][newbp1_fb];

				//kpp差分 
				for (i = 0; i < moveduniform1; ++i) {
					ADD_BWKPP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
				}
				for (++i; i < 38; ++i) {
					ADD_BWKPP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
				}
			}
			//2つ駒が動いた。この場合のKPP差分がめんどくさい
			else {

				//kpp差分計算の都合上moveduniform1を先に持ってくる
				if (moveduniform1 > moveduniform2) { swap(moveduniform1, moveduniform2); }

				
			
				//kkp差分
				KKP -= kkp[bksq][wksq][oldbp1_fb];
				KKP += kkp[bksq][wksq][newbp1_fb];
				KKP -= kkp[bksq][wksq][oldbp2_fb];
				KKP += kkp[bksq][wksq][newbp2_fb];

				//kpp差分
				for (i = 0; i < moveduniform1; ++i)
				{
					ADD_BWKPP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
					ADD_BWKPP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
				}
				for (++i; i < moveduniform2; ++i)
				{
					ADD_BWKPP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
					ADD_BWKPP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
				}
				for (++i; i < 38; ++i)
				{
					ADD_BWKPP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
					ADD_BWKPP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
				}

				//足しすぎ、引きすぎをここで補正する。
				bKPP -= Eval::kpp[bksq][oldbp1_fb][oldbp2_fb];
				wKPP += Eval::kpp[wksq][oldbp1_fw][oldbp2_fw];
				bKPP += Eval::kpp[bksq][newbp1_fb][newbp2_fb];
				wKPP -= Eval::kpp[wksq][newbp1_fw][newbp2_fw];


			}

			//差分のチェック
#ifdef DIFFTEST
			{
				eval_KPP(pos);
				if (KKP != pos.state()->sumKKP || bKPP != pos.state()->sumBKPP || wKPP != pos.state()->sumWKPP) {

					cout << pos << endl;
					cout << "KKP " << KKP << ":" << pos.state()->sumKKP << " bkpp " << bKPP << ":" << pos.state()->sumBKPP << " wkpp " << wKPP << ":" << pos.state()->sumWKPP << endl;
					cout << oldbp1_fb << endl;
					cout << oldbp1_fw << endl;
					cout << oldbp2_fb << endl;
					cout << oldbp2_fw << endl;
					cout << newbp1_fb << endl;
					cout << newbp1_fw << endl;
					cout << newbp2_fb << endl;
					cout << newbp2_fw << endl;
					cout << movedpiece << endl;
					cout << capturedpiece << endl;
					cout << moveduniform1 << endl;
					cout << moveduniform2 << endl;
					cout << bksq << " " << wksq;
					ASSERT(0);
				}
			}
#endif



			now->sumKKP = KKP;
			now->sumBKPP = bKPP;
			now->sumWKPP = wKPP;

		}//差分計算

	DIFFEND:;
		return (Value)((bKPP + wKPP + (KKP / FV_SCALE_KKP)) / FV_SCALE);

	}


#if defined(EVAL_KPP)

	//差分計算のための対称性を持たせる
	void Eval::param_sym_ij() {

		bool check_KPP[81][fe_end][fe_end] = { false };
		bool check_KKP[81][81][fe_end] = { false };
		bool check_KK[81][81] = { false };

		FOR_KPP(ksq, bp1, bp2) {
			if (check_KPP[ksq][bp1][bp2] == false) {
				check_KPP[ksq][bp1][bp2] = check_KPP[ksq][bp2][bp1] = true;
				int16_t a = kpp[ksq][bp1][bp2], b = kpp[ksq][bp2][bp1];
				kpp[ksq][bp1][bp2] = kpp[ksq][bp2][bp1] = (a + b) / 2;
			}
		}


		FOR_KK(ksq1, ksq2) {
			if (check_KK[ksq1][ksq2] == false) {
				check_KK[ksq1][ksq2] = check_KK[ksq2][ksq1] = true;
				int32_t a = kk[ksq1][ksq2], b = kk[ksq2][ksq1];
				kk[ksq1][ksq2] = kk[ksq2][ksq1] = (a + b) / 2;

			}


			for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end; bp1++) {
				if (check_KKP[ksq1][ksq2][bp1] == false) {
					check_KKP[ksq1][ksq2][bp1] = check_KKP[ksq2][ksq1][bp1] = true;
					int32_t a = kkp[ksq1][ksq2][bp1], b = kkp[ksq2][ksq1][bp1];
					kkp[ksq1][ksq2][bp1] = kkp[ksq2][ksq1][bp1] = (a + b) / 2;
				}
			}
		}
	}
#endif
}

#endif