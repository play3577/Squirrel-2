#define _CRT_SECURE_NO_WARNINGS

#include "position.h"
#include "evaluate.h"
#include "usi.h"
#ifdef EVAL_PPT
namespace Eval {

	int32_t PP[fe_end2][fe_end2];//手番ボーナスを与えないほうは今までのPPを使いまわす。
	int32_t PPT[fe_end2][fe_end2];//手番ボーナス


	Value eval_PPT(const Position& pos) {

		int32_t bPP = 0, wPP = 0, bPPt = 0, wPPt = 0;
		BonaPiece *list_fb = pos.evallist().bplist_fb, *list_fw = pos.evallist().bplist_fw;

		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
			
#if defined(_MSC_VER)
				bPP += PP[list_fb[i]][list_fb[j]];
				wPP -= PP[list_fw[i]][list_fw[j]];
				bPPt += PPT[list_fb[i]][list_fb[j]];
				wPPt += PPT[list_fw[i]][list_fw[j]];
#endif
#if defined(__GNUC__)
				bPP += PP[pos.evallist().bplist_fb[i]][pos.evallist().bplist_fb[j]];
				wPP -= PP[pos.evallist().bplist_fw[i]][pos.evallist().bplist_fw[j]];
#endif


			}
		}
		pos.state()->bpp = (bPP);
		pos.state()->wpp = wPP;
		pos.state()->bppt = bPPt;
		pos.state()->wppt = wPPt;

		int pp = (bPP + wPP) / FV_SCALE, bonus = (bPPt + wPPt) / FV_SCALE;

		return Value(((pos.sidetomove() == BLACK) ? pp : -pp) + bonus);
	}

	void read_FV() {
		FILE* fp = std::fopen(Options["PP"].str().c_str(), "rb");
		if (fp != NULL) {
			std::fread(&PP, sizeof(PP), 1, fp);
		}
		else {
			cout << "error reading PP!!!" << endl;
		}
		std::fclose(fp);

		fp = std::fopen(Options["PPT"].str().c_str(), "rb");
		if (fp != NULL) {
			std::fread(&PPT, sizeof(PPT), 1, fp);
		}
		else {
			cout << "error reading PPT!!!" << endl;
		}
		std::fclose(fp);

		return;
	}

	void write_FV() {
		FILE* fp = std::fopen(Options["PP"].str().c_str(), "wb");
		std::fwrite(&PP, sizeof(PP), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["PPT"].str().c_str(), "wb");
		std::fwrite(&PPT, sizeof(PPT), 1, fp);
		std::fclose(fp);
	}

	Value eval_diff_PPT(const Position& pos) {

		const StateInfo *now = pos.state();
		const StateInfo *prev = pos.state()->previous;
		const auto list = pos.evallist();
		int32_t bPP, wPP,bPPt,wPPt;
		bPP = prev->bpp;
		wPP = prev->wpp;
		bPPt = prev->bppt;
		wPPt = prev->wppt;

		ASSERT(bPP != Value_error&&wPP != Value_error);

		const Eval::BonaPiece oldbp1_fb = now->dirtybonap_fb[0];
		const Eval::BonaPiece oldbp2_fb = now->dirtybonap_fb[1];
		const Eval::BonaPiece oldbp1_fw = now->dirtybonap_fw[0];
		const Eval::BonaPiece oldbp2_fw = now->dirtybonap_fw[1];
		//swapする可能性があるためここはconstにしない
		Eval::UniformNumber moveduniform1 = now->dirtyuniform[0];
		Eval::UniformNumber moveduniform2 = now->dirtyuniform[1];




		const Eval::BonaPiece* now_list_fb = list.bplist_fb;
		const Eval::BonaPiece* now_list_fw = list.bplist_fw;

		const Eval::BonaPiece newbp1_fb = list.bplist_fb[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fb = list.bplist_fb[now->dirtyuniform[1]];
		const Eval::BonaPiece newbp1_fw = list.bplist_fw[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fw = list.bplist_fw[now->dirtyuniform[1]];

#define ADD_PP(oldbp,oldwp,newbp,newwp){ \
		bPP -= PP[oldbp][now_list_fb[i]];\
		bPP += PP[newbp][now_list_fb[i]];\
		wPP += PP[oldwp][now_list_fw[i]];\
		wPP -= PP[newwp][now_list_fw[i]];\
											\
		bPPt -= PPT[oldbp][now_list_fb[i]]; \
		bPPt += PPT[newbp][now_list_fb[i]]; \
		wPPt -= PPT[oldwp][now_list_fw[i]]; \
		wPPt += PPT[newwp][now_list_fw[i]]; \
}

		int i;


		if (moveduniform2 == Num_Uniform) {
			//駒が①つ動いた

			//このようにして②つに分けることでPP[old1][new1]を引いてしまうのを防ぎ,PP[new][new]を足してしまうのを防ぐ。(賢い...さすがはやねうら王...)
			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}

		}
		else {
			//駒が②つ動いた

			//pawn1<=dirty<dirty2<fe_end2　にしておく
			if (moveduniform1 > moveduniform2) { std::swap(moveduniform1, moveduniform2); }

			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//コレで[old1][old2]回避
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < moveduniform2; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//コレで[old1][old2] [new1][new2]回避
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			//ここで回避してしまった[old1][old2] [new1][new2]を補正する。
			bPP -= PP[oldbp1_fb][oldbp2_fb];
			bPP += PP[newbp1_fb][newbp2_fb];
			wPP += PP[oldbp1_fw][oldbp2_fw];
			wPP -= PP[newbp1_fw][newbp2_fw];

			bPPt -= PPT[oldbp1_fb][oldbp2_fb];
			bPPt += PPT[newbp1_fb][newbp2_fb];
			wPPt -= PPT[oldbp1_fw][oldbp2_fw];
			wPPt += PPT[newbp1_fw][newbp2_fw];
		}





		pos.state()->bpp = bPP;
		pos.state()->wpp = wPP;
		pos.state()->bppt = bPPt;
		pos.state()->wppt = wPPt;
#ifdef DIFFTEST


		
		eval_PPT(pos);
		//if (Value((bPP + wPP) / FV_SCALE) != eval_PP(pos)) {
		if (bPP != pos.state()->bpp || wPP != pos.state()->wpp||bPPt!=pos.state()->bppt||wPPt!=pos.state()->wppt) {

			cout << pos << endl;
			/*cout << "oldlist" << endl;
			for (int i = 0; i < Num_Uniform; i++) {
			cout <<"fb:"<< old_list_fb[i];
			cout << "fw:" << old_list_fw[i] << endl;
			}*/
			cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PPT(pos) << endl;
			cout << "bpp " << bPP << " " << pos.state()->bpp << endl;
			cout << "wpp " << wPP << " " << pos.state()->wpp << endl;
			UNREACHABLE;
		}
#endif
		//ASSERT(((bPP + wPP+bPPt+wPPt) / FV_SCALE) < INT16_MAX);
		int pp = (bPP + wPP)/FV_SCALE, bonus = (bPPt + wPPt)/FV_SCALE;

		return Value(((pos.sidetomove()==BLACK)?pp:-pp)+bonus);

	}
	
	Value eval(const Position &pos) {

		Value pp, value;
		Value material = (pos.sidetomove() == BLACK)?pos.state()->material:-pos.state()->material;

		if (pos.state()->bpp != Value_error) {
			ASSERT(pos.state()->wpp != Value_error);
			ASSERT(pos.state()->wppt != Value_error);
			ASSERT(pos.state()->bppt != Value_error);
			//pp = Value((pos.state()->bpp + pos.state()->wpp+ pos.state()->bppt + pos.state()->wppt) / FV_SCALE);
			pp = (Value)(((pos.sidetomove() == BLACK) ? pos.state()->bpp + pos.state()->wpp : -(pos.state()->bpp + pos.state()->wpp)) + pos.state()->bppt + pos.state()->wppt) / FV_SCALE;
#ifdef DIFFTEST
			int32_t bPP, wPP, bPPt, wPPt;
			bPP = pos.state()->bpp, bPPt = pos.state()->bppt, wPP = pos.state()->wpp, wPPt = pos.state()->wppt;

			eval_PPT(pos);

			if (bPP != pos.state()->bpp || wPP != pos.state()->wpp || bPPt != pos.state()->bppt || wPPt != pos.state()->wppt) {

				cout << pos << endl;
				cout << " diff " << pp << " evalfull " << eval_PPT(pos) << endl;

				cout << "bpp " << bPP << " " << pos.state()->bpp << endl;;
				cout << "wpp " << wPP << " " << pos.state()->wpp << endl;;
				UNREACHABLE;
			}

#endif
		}
		else if (pos.state()->previous != nullptr&&pos.state()->previous->bpp != Value_error) {
			//差分計算可能
			ASSERT(pos.state()->previous->wpp != Value_error);
			ASSERT(pos.state()->previous->wppt != Value_error);
			ASSERT(pos.state()->previous->bppt != Value_error);
			pp = eval_diff_PPT(pos);

		}
		else {
			pp = eval_PPT(pos);
		}

		value = material + pp;

		return value;
		//return (pos.sidetomove() == BLACK) ? value : -value;
	}


	void param_sym_ij() {

		bool check[fe_end2][fe_end2] = { false };
		memset(check, false, sizeof(check));

		for (BonaPiece i = BONA_PIECE_ZERO; i < fe_end2; i++) {
			for (BonaPiece j = BONA_PIECE_ZERO; j < fe_end2; j++) {

				if (check[i][j] != true) {

					check[i][j] = check[j][i] = true;

					int32_t a = PP[i][j], b = PP[j][i];
					PP[i][j] = PP[j][i] = (a + b) / 2;
					int32_t c = PPT[i][j], d = PPT[j][i];
					PPT[i][j] = PPT[j][i] = (c + d) / 2;
				}
			}
		}
	}
}
#endif