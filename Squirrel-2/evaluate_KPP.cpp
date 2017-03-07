#define _CRT_SECURE_NO_WARNINGS

#include "evaluate.h"
#include "usi.h"
#include "position.h"
#ifdef EVAL_KPP

namespace Eval {

	int16_t kpp[82][fe_end][fe_end];
	int32_t kkp[82][82][fe_end + 1];




	void read_KPP() {
		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "rb");
		if (fp == NULL) { ASSERT(0); }
		std::fread(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "rb");
		if (fp == NULL) { ASSERT(0); }
		std::fread(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);


	}

	void write_KPP() {

		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "wb");
		std::fwrite(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "wb");
		std::fwrite(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);

	}

	Value eval_KPP(const Position& pos) {

		const Square bksq = pos.ksq(BLACK);
		const Square wksq = hihumin_eye(pos.ksq(WHITE));

		auto list_fb = pos.evallist().bplist_fb;
		auto list_fw = pos.evallist().bplist_fw;

		int i, j;

		BonaPiece k0, k1;
		int32_t bKPP, wKPP, KKP;

		KKP = kkp[bksq][wksq][fe_end];
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


		return Value((bKPP + wKPP + KKP / FV_SCALE_KKP) / FV_SCALE);


	}



	Value eval_diff_KPP(const Position& pos) {

		auto now = pos.state();

		int KKP, bKPP, wKPP;

		//すでに計算されていた
		if (now->sumBKPP != Value_error&&now->sumWKPP != Value_error&&now->sumKKP != Value_error) {

			KKP = now->sumKKP;
			bKPP = now->sumBKPP;
			wKPP = now->sumWKPP;
			goto DIFFEND;
		}

		auto prev = now->previous;
		//全計算しなければならない
		if(prev->sumBKPP == Value_error||prev->sumWKPP == Value_error||prev->sumKKP == Value_error) {
			return eval_KPP(pos);
		}
		
		//ここから差分計算開始！！
		{
			KKP = prev->sumKKP;
			bKPP = prev->sumBKPP;
			wKPP = prev->sumWKPP;
			int k0, k1, k2, k3;

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

					KKP = kkp[bksq][wksq][fe_end];

					for (i = 0; i < 38; i++) {
						k0 = now_list_fb[i];

						KKP += kkp[bksq][wksq][k0];

						for (j = 0; j < i; j++) {
							bKPP += kpp[bksq][k0][now_list_fb[j]];
						}
					}

					if (moveduniform2 != Eval::Num_Uniform) {
						ASSERT(is_ok(capturedpiece));
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
					KKP= kkp[bksq][wksq][fe_end];
					
					//白側とKKPを全計算 
					for (i = 0; i < 38; ++i) {
						k0 = now_list_fb[i];//k0はKKPで使う
						k1 = now_list_fw[i];
						KKP += kkp[bksq][wksq][k0];
						for (j = 0; j < i; j++) {
							wKPP -= kpp[wksq][k1][now_list_fw[j]];
						}
					}

					//動いた駒が2つあればbkppも差分計算
					if (moveduniform2 != Eval::Num_Uniform) {
						ASSERT(is_ok(capturedpiece));
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

					k0 = oldbp1_fb;
					k1 = oldbp1_fw;
					k2 = newbp1_fb;
					k3 = newbp1_fw;

					//kkp差分
					KKP -= kkp[bksq][wksq][k0];
					KKP += kkp[bksq][wksq][k2];

					//kpp差分 
					for (i = 0; i < moveduniform1; ++i) {
						ADD_BWKPP(k0, k1, k2, k3);
					}
					for (++i; i < 38; ++i) {
						ADD_BWKPP(k0, k1, k2, k3);
					}
				}
				//2つ駒が動いた。この場合のKPP差分がめんどくさい
				else {

					//kpp差分計算の都合上moveduniform1を先に持ってくる
					if (moveduniform1 > moveduniform2) { swap(moveduniform1, moveduniform2); }

					k0 = oldbp1_fb;
					k1 = oldbp1_fw;
					k2 = newbp1_fb;
					k3 = newbp1_fw;

					int m0, m1, m2, m3;
					m0 = oldbp2_fb;
					m1 = oldbp2_fw;
					m2 = newbp2_fb;
					m3 = newbp2_fw;

					//kkp差分
					KKP -= kkp[bksq][wksq][k0];
					KKP += kkp[bksq][wksq][k2];
					KKP -= kkp[bksq][wksq][m0];
					KKP += kkp[bksq][wksq][m2];

					//kpp差分
					for (i = 0; i < moveduniform1; ++i)
					{
						ADD_BWKPP(k0, k1, k2, k3);
						ADD_BWKPP(m0, m1, m2, m3);
					}
					for (++i; i < moveduniform2; ++i)
					{
						ADD_BWKPP(k0, k1, k2, k3);
						ADD_BWKPP(m0, m1, m2, m3);
					}
					for (++i; i < 38; ++i)
					{
						ADD_BWKPP(k0, k1, k2, k3);
						ADD_BWKPP(m0, m1, m2, m3);
					}

					//足しすぎ、引きすぎをここで補正する。
					bKPP -= Eval::kpp[bksq][k0][m0];
					wKPP += Eval::kpp[wksq][k1][m1];
					bKPP += Eval::kpp[bksq][k2][m2];
					wKPP -= Eval::kpp[wksq][k3][m3];

					
				}

				//差分のチェック
				/*
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
				*/
				{
					eval_KPP(pos);
					if (KKP != pos.state()->sumKKP || bKPP != pos.state()->sumBKPP || wKPP != pos.state()->sumWKPP) {

						cout << pos << endl;
						cout << "KKP " << KKP << ":" << pos.state()->sumKKP << " bkpp " << bKPP << ":" << pos.state()->sumBKPP << " wkpp " << wKPP << ":" << pos.state()->sumWKPP << endl;
						/*cout << oldbp1_fb << endl;
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
						cout << moveduniform2 << endl;*/
						cout << bksq << " " << wksq;
						ASSERT(0);
					}
				}


			}

			now->sumKKP = KKP;
			now->sumBKPP = bKPP;
			now->sumWKPP = wKPP;

		}//差分計算
		
	DIFFEND:;
		return (Value)((bKPP + wKPP + KKP / FV_SCALE_KKP) / FV_SCALE);

	}

	Value Eval::eval(const Position& pos) {

		Value material= pos.state()->material;
		
		Value value = material+eval_diff_KPP(pos);

		return (pos.sidetomove() == BLACK) ? value : -value;
	}

}

#endif