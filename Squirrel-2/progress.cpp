#define _CRT_SECURE_NO_WARNINGS 1


#include "progress.h"
#include "misc.h"
#include "game_database.h"

#include <random>
#include <iostream>
#include <fstream>
#include <omp.h>


namespace Progress {

	ALIGNED(32) int32_t prog_KP[SQ_NUM][Eval::fe_end];

	void Progress::initialize_KP()
	{
		std::random_device rd;
		std::mt19937 mt(rd());

		for (Square ksq = SQ_ZERO; ksq < SQ_NUM; ksq++) {

			for (Eval::BonaPiece bp = Eval::BONA_PIECE_ZERO; bp < Eval::fe_end; bp++) {

				prog_KP[ksq][bp] = int32_t(mt() % 10);

			}
		}
		write_KP();
	}

	void Progress::read_KP()
	{
		FILE* fp = std::fopen("c:/book2/prog/progKP.bin", "rb");
		if (fp == NULL) { cout << "cannot read progKP.bin" << endl; ASSERT(0); }

		std::fread(&prog_KP, sizeof(prog_KP), 1, fp);
		std::fclose(fp);
	}

	void Progress::write_KP()
	{
		FILE* fp = std::fopen("c:/book2/prog/progKP.bin", "wb");
		if (fp == NULL) { cout << "cannot write progKP.bin" << endl; ASSERT(0); }
		std::fwrite(&prog_KP, sizeof(prog_KP), 1, fp);
		std::fclose(fp);

	}

	double calc_prog(const Position & pos)
	{
		int32_t bkp = 0, wkp = 0;

		const Square bksq = pos.ksq(BLACK);
		const Square wksq = hihumin_eye(pos.ksq(WHITE));

		const Eval::BonaPiece *list_fb = pos.evallist().bplist_fb, *list_fw = pos.evallist().bplist_fw;

		for (int i = 0; i < 38; i++) {
			bkp += prog_KP[bksq][list_fb[i]];
			wkp += prog_KP[wksq][list_fw[i]];
		}

		pos.state()->bkp = bkp;
		pos.state()->wkp = wkp;

		//cout << bkp << " " << wkp<<endl;
		return sigmoid(double(bkp+wkp)*double(1.0 / paramscale));
	}

	/*
	kp差分計算。

	・kが動いた場合
	kのみが動いた場合
	片側KP全計算
	もう一つ駒が動いた場合
	片側KP全計算　sum+=KP[K][移動後] sum-=KP[K][移動前]

	・Pが動いた場合。
	動く前のKPを引いて動いた後のKPを足すだけ。
	
	
	*/
	double calc_diff_prog(const Position & pos)
	{
		auto now = pos.state();
		int32_t bkp = 0, wkp = 0;

		if (now->bkp != Value_error&&now->wkp != Value_error) {
			//すでに計算済みであった。
			bkp = now->bkp;
			wkp = now->wkp;
			goto CALC_PROG_END;
		}

		auto prev = now->previous;

		if (prev->bkp == Value_error || prev == nullptr) {
			//前回計算されていなかった場合は全計算するしかない。
			return calc_prog(pos);
		}

		
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

		const Piece movedpiece=now->DirtyPiece[0];
		//const Piece capturedpiece = now->DirtyPiece[1];

		/*
		kp差分計算。

		・kが動いた場合
		kのみが動いた場合
		片側KP全計算
		もう一つ駒が動いた場合
		片側KP全計算　sum+=KP[K][移動後] sum-=KP[K][移動前]

		・Pが動いた場合。
		動く前のKPを引いて動いた後のKPを足すだけ。
		*/

		//Kが動いた！

		const Square bksq = now->ksq_[BLACK];
		const Square wksq = hihumin_eye(now->ksq_[WHITE]);

		if (moveduniform1>=38) {
			ASSERT(movedpiece == B_KING || movedpiece == W_KING);
			const Color movedcolor = piece_color(movedpiece);

			//動いた駒は1つ
			if (moveduniform2 == Eval::Num_Uniform) {
				//王様は黒
				if (movedcolor == BLACK) {
					wkp = prev->wkp;
					for (int i = 0; i < 38; i++) {
						bkp += prog_KP[bksq][now_list_fb[i]];
					}
				}
				else {
					//王様は白
					bkp = prev->bkp;
					for (int i = 0; i < 38; i++) {
						wkp += prog_KP[wksq][now_list_fw[i]];
					}
				}

			}
			//動いた駒は2つ(玉によって捕獲された駒)
			else {
				
				//王様は黒
				if (movedcolor == BLACK) {
					wkp = prev->wkp;
					for (int i = 0; i < 38; i++) {
						bkp += prog_KP[now->ksq_[BLACK]][now_list_fb[i]];
					}
					wkp += prog_KP[wksq][newbp2_fw];
					wkp -= prog_KP[wksq][oldbp2_fw];
				}
				else {
					//王様は白
					bkp = prev->bkp;
					for (int i = 0; i < 38; i++) {
						wkp += prog_KP[hihumin_eye(now->ksq_[WHITE])][now_list_fw[i]];
					}
					bkp += prog_KP[bksq][newbp2_fb];
					bkp -= prog_KP[bksq][oldbp2_fb];
				}
			}
		}
		//玉以外の駒が動いた。
		else {


			bkp = prev->bkp;
			wkp = prev->wkp;

			//動いた駒は1つ
			if (moveduniform2 == Eval::Num_Uniform) {
				bkp -= prog_KP[bksq][oldbp1_fb];
				bkp += prog_KP[bksq][newbp1_fb];

				wkp -= prog_KP[wksq][oldbp1_fw];
				wkp += prog_KP[wksq][newbp1_fw];

			}
			else {
				bkp -= prog_KP[bksq][oldbp1_fb];
				bkp += prog_KP[bksq][newbp1_fb];

				wkp -= prog_KP[wksq][oldbp1_fw];
				wkp += prog_KP[wksq][newbp1_fw];

				bkp -= prog_KP[bksq][oldbp2_fb];
				bkp += prog_KP[bksq][newbp2_fb];

				wkp -= prog_KP[wksq][oldbp2_fw];
				wkp += prog_KP[wksq][newbp2_fw];


			}


		}




		pos.state()->bkp = bkp, pos.state()->wkp = wkp;

	CALC_PROG_END:

		//差分計算できているかテスト
		calc_prog(pos);

		if (bkp != pos.state()->bkp || wkp != pos.state()->wkp) {
			cout << bkp << " " << pos.state()->bkp << " " << wkp << " " << pos.state()->wkp << endl;
			ASSERT(0);
		}


		return sigmoid(double(bkp + wkp)*double(1.0 / paramscale));
	}

}
