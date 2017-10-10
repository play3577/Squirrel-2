

#include "evaluate.h"
#include "position.h"
#include "eval_hash.h"
#include <utility>
/*
bonanza ������evah_hash�Ƃ������̂�������
�m����wpp,bpp�����������̂Ɋi�[����Ε]���͑����Ȃ�Ǝv����
key������Ă��܂��ƔߎS�Ȍ��ʂ��N����B�ǂ������hash�����Ȃ��悤�ɂł���̂��낤���H
*/

namespace Eval {

	Bp2Piece bp2piece;

	//�O�p�e�[�u���ɂ��ׂ���....??
#ifdef EVAL_PP
	ALIGNED(32) int32_t PP[fe_end2][fe_end2];
#endif




	int16_t piece_value[PC_ALL] = {
		Value_Zero,

		PawnValue,
		LanceValue,
		KnightValue,
		SilverValue,
		BishopValue,
		RookValue,
		GoldValue,
		KingValue,
		ProPawnValue,
		ProLanceValue,
		ProKnightValue,
		ProSilverValue,
		UnicornValue,
		DragonValue,
		0,0,

		-PawnValue,
		-LanceValue,
		-KnightValue,
		-SilverValue,
		-BishopValue,
		-RookValue,
		-GoldValue,
		-KingValue,
		-ProPawnValue,
		-ProLanceValue,
		-ProKnightValue,
		-ProSilverValue,
		-UnicornValue,
		-DragonValue,
	};

	int16_t capture_value[PC_ALL] = {
		Value_Zero,

		2*PawnValue,
		2*LanceValue,
		2*KnightValue,
		2*SilverValue,
		2*BishopValue,
		2*RookValue,
		2*GoldValue,
		2*KingValue,
		ProPawnValue+PawnValue,
		ProLanceValue+LanceValue,
		ProKnightValue+KnightValue,
		ProSilverValue+SilverValue,
		UnicornValue+BishopValue,
		DragonValue+RookValue,
		0,0,
		2 * PawnValue,
		2 * LanceValue,
		2 * KnightValue,
		2 * SilverValue,
		2 * BishopValue,
		2 * RookValue,
		2 * GoldValue,
		2 * KingValue,
		ProPawnValue + PawnValue,
		ProLanceValue + LanceValue,
		ProKnightValue + KnightValue,
		ProSilverValue + SilverValue,
		UnicornValue + BishopValue,
		DragonValue + RookValue,
	};

	int16_t diff_promote[GOLD] = {
		0,
		ProPawnValue-PawnValue,
		ProLanceValue-LanceValue,
		ProKnightValue-KnightValue,
		ProSilverValue-SilverValue,
		UnicornValue-BishopValue,
		DragonValue-RookValue,
	};
	
	


	

	//���Ԃ��猩��bonapiece��Ԃ��B
	BonaPiece bonapiece(const Square sq, const Piece pc)
	{
		switch (pc)
		{
		case B_PAWN:
			return BonaPiece(f_pawn + sq);
			break;
		case B_LANCE:
			return BonaPiece(f_lance+sq);
			break;
		case B_KNIGHT:
			return BonaPiece(f_knight+sq);
			break;
		case B_SILVER:
			return BonaPiece(f_silver+sq);
			break;
		case B_BISHOP:
			return BonaPiece(f_bishop+sq);
			break;
		case B_ROOK:
			return BonaPiece(f_rook+sq);
			break;
		case B_GOLD:
			return BonaPiece(f_gold+sq);
			break;
		case B_KING:
			return BonaPiece(f_king+sq);
			break;
		case B_PRO_PAWN:
		case B_PRO_LANCE:
		case B_PRO_NIGHT:
		case B_PRO_SILVER:
			return BonaPiece(f_gold+sq);
			break;
		case B_UNICORN:
			return BonaPiece(f_unicorn+sq);
			break;
		case B_DRAGON:
			return BonaPiece(f_dragon+sq);
			break;
		case W_PAWN:
			return BonaPiece(e_pawn+sq);
			break;
		case W_LANCE:
			return BonaPiece(e_lance+sq);
			break;
		case W_KNIGHT:
			return BonaPiece(e_knight+sq);
			break;
		case W_SILVER:
			return BonaPiece(e_silver+sq);
			break;
		case W_BISHOP:
			return BonaPiece(e_bishop+sq);
			break;
		case W_ROOK:
			return BonaPiece(e_rook+sq);
			break;
		case W_GOLD:
			return BonaPiece(e_gold+sq);
			break;
		case W_KING:
			return BonaPiece(e_king+sq);
			break;
		case W_PRO_PAWN:
		case W_PRO_LANCE:
		case W_PRO_NIGHT:
		case W_PRO_SILVER:
			return BonaPiece(e_gold+sq);
			break;
		case W_UNICORN:
			return BonaPiece(e_unicorn+sq);
			break;
		case W_DRAGON:
			return BonaPiece(e_dragon+sq);
			break;
		default:
			UNREACHABLE;
			return BONA_PIECE_ZERO;
			break;
		}
	}

	BonaPiece bonapiece(const Color c, const Piece pt, const int num)
	{
		Piece pc = add_color(pt, c);

		switch (pc)
		{
		case B_PAWN:
			return BonaPiece(f_hand_pawn + (num-1));
			break;
		case B_LANCE:
			return BonaPiece(f_hand_lance + (num-1));
			break;
		case B_KNIGHT:
			return BonaPiece(f_hand_knight + (num-1));
			break;
		case B_SILVER:
			return BonaPiece(f_hand_silver + (num-1));
			break;
		case B_BISHOP:
			return BonaPiece(f_hand_bishop + (num-1));
			break;
		case B_ROOK:
			return BonaPiece(f_hand_rook + (num-1));
			break;
		case B_GOLD:
			return BonaPiece(f_hand_gold + (num-1));
			break;

		case W_PAWN:
			return BonaPiece(e_hand_pawn + (num-1));
			break;
		case W_LANCE:
			return BonaPiece(e_hand_lance + (num-1));
			break;
		case W_KNIGHT:
			return BonaPiece(e_hand_knight + (num-1));
			break;
		case W_SILVER:
			return BonaPiece(e_hand_silver + (num-1));
			break;
		case W_BISHOP:
			return BonaPiece(e_hand_bishop + (num-1));
			break;
		case W_ROOK:
			return BonaPiece(e_hand_rook + (num-1));
			break;
		case W_GOLD:
			return BonaPiece(e_hand_gold + (num-1));
			break;
		default:
			UNREACHABLE;
			return BONA_PIECE_ZERO;
			break;
		}
	}

#ifdef EVAL_EFFECT
	//����ɂ��ʎ��ӂ̌���
	ALIGNED(32) int32_t KE_FROMENEMY[SQ_NUM][1<<8];

	//���Ԃ̋���Ԃ̋ʂɍU�ߍ���ł���index���擾����
	int make_effectindex_FROMBLACK(const Position& pos) {
		//ASSERT(pos.state()->KEindex_fromblack == (1 << 9));//���v�Z�̃t���O

		int index=0;

		const int d[8] = { -10,-9,-8,-1,1,8,9,10 };
		const Color us = BLACK;
		const Color enemy = opposite(us);
		const Square eksq = pos.ksq(enemy);

		for (int i = 0; i < 8; i++) {
			Square around_ksq = eksq + d[i];
			if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {
				
				if (pos.attackers_to(us, around_ksq, pos.ret_occ_256()).isNot()) {
					index += 1 << i;
				}
			}
		}
		return index;
	}

	int make_effectindex_FROMWHITE(const Position& pos) {
		//ASSERT(pos.state()->KEindex_fromwhite == (1 << 9));//���v�Z�̃t���O

		int index = 0;
		const int d[8] = { 10,9,8,1,-1,-8,-9,-10 };//BLACK�̎��Ɠ����l���g����悤�ɔ��ʂ��猩�č��ʂ̕��тƓ����ɂ���

		const Color us = WHITE;
		const Color enemy = opposite(us);
		const Square eksq = pos.ksq(enemy);

		for (int i = 0; i < 8; i++) {
			Square around_ksq = eksq + d[i];
			if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {

				if (pos.attackers_to(us, around_ksq, pos.ret_occ_256()).isNot()) {
					index += 1 << i;
				}
			}
		}
		return index;
	}


	Value eval_effect(const Position& pos) {

		int effect=Value_Zero;
		const Square Bksq = pos.ksq(BLACK), Wksq = hihumin_eye(pos.ksq(WHITE));
		const Color stm = pos.sidetomove();
		int indexfromblack, indexfromwhite;

		//index�̍쐬�ǂ��炩��mate1ply�Ōv�Z�ł���̂ł���͏Ȃ� ��Ŏ�������
		indexfromblack = make_effectindex_FROMBLACK(pos);
		indexfromwhite = make_effectindex_FROMWHITE(pos);
		if (stm == BLACK) {
			effect += KE_FROMENEMY[Wksq][indexfromblack];//�U�ߍ���ł镪
			effect -= KE_FROMENEMY[Bksq][indexfromwhite];//�U�ߍ��܂�Ă镪
		}
		else {
			effect -= KE_FROMENEMY[Wksq][indexfromblack];
			effect += KE_FROMENEMY[Bksq][indexfromwhite];
		}
		return (Value)effect/FV_SCALE;
	}

#endif



//#define DIFFTEST
	Value eval_material(const Position & pos)
	{
		int16_t v = 0;

		//�Տ�
		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
			v += piece_value[pos.piece_on(sq)];
		}

		//���
		for (Color c = BLACK; c <= WHITE; c++) {


			const auto hands = pos.hand(c);

			for (Piece pt = PAWN; pt <= GOLD; pt++) {
				v += (c == BLACK ? 1:-1)*num_pt(hands, pt)*piece_value[pt];
			}

		}
		return Value(v);
	}
#ifdef EVAL_PP
	Value eval(const Position & pos)
	{
		Value pp,value,effect=Value(0);
		Value material= pos.state()->material;

#ifndef EVAL_NONDIFF

#ifdef EVALHASH
		bool found=false;
		EHASH_Entry* tte;
		tte=EHASH.probe(pos.key(), found);
		if (found) {
			pos.state()->bpp = tte->bpp();
			pos.state()->wpp = tte->wpp();
			
			int bpp = pos.state()->bpp;
			int wpp = pos.state()->wpp;
#ifdef DIFFTEST
			
			eval_PP(pos);
			if (/*pp != eval_PP(pos)*/
				bpp != pos.state()->bpp ||
				wpp != pos.state()->wpp) {
				cout << " diff " << Value((bpp + wpp) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;;
				cout << "bpp " << bpp << " " << pos.state()->bpp << endl;;
				cout << "wpp " << wpp << " " << pos.state()->wpp << endl;;
			}
#endif
			ASSERT(((bpp + wpp) / FV_SCALE) < INT16_MAX);
			//����material��tempo�����̖Y��Ă�I�I�I�I�I
			pp=Value((bpp + wpp) / FV_SCALE);
			goto FIND_HASH;
		}
#endif


		//�v�Z�ς�
		if (pos.state()->bpp != Value_error) {
			ASSERT(pos.state()->wpp != Value_error);
			pp=Value((pos.state()->bpp + pos.state()->wpp) / FV_SCALE);

#ifdef DIFFTEST
		


			//�v�Z�ς݂̒l�����������m�F
			int bPP = pos.state()->bpp, wPP = pos.state()->wpp;
#ifdef EVAL_PROG
			int bPPf = pos.state()->bppf, wPPf = pos.state()->wppf;
#endif // EVAL_PROG

			

			eval_PP(pos);

			if (/*pp != eval_PP(pos)*/
				bPP!= pos.state()->bpp||
				wPP!= pos.state()->wpp
#ifdef EVAL_PROG
				||bPPf!= pos.state()->bppf||
				wPPf!= pos.state()->wppf
#endif
				) {
				cout << " diff " << pp << " evalfull " << eval_PP(pos) << endl;

				cout << pos << endl;
					/*cout << "oldlist" << endl;
					for (int i = 0; i < Num_Uniform; i++) {
					cout <<"fb:"<< old_list_fb[i];
					cout << "fw:" << old_list_fw[i] << endl;
					}*/
				cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;;
				cout << "bpp " << bPP << " " << pos.state()->bpp << endl;;
				cout << "wpp " << wPP << " " << pos.state()->wpp << endl;;
#ifdef EVAL_PROG
				cout << "bpp " << bPPf << " " << pos.state()->bppf << endl;;
				cout << "wpp " << wPPf << " " << pos.state()->wppf << endl;;
#endif
				UNREACHABLE;;
			}
#endif
		}
		else if (pos.state()->previous!=nullptr&&pos.state()->previous->bpp != Value_error) {
			//�����v�Z�\
			ASSERT(pos.state()->previous->wpp != Value_error);
			pp = eval_diff_PP(pos);

		}
		else {
			//�S�v�Z�I
#ifdef HAVE_AVX2
			pp = eval_allPP_AVX2(pos);
#else
			pp = eval_PP(pos);
#endif
		}
#else
		pp = eval_PP(pos);
		//ASSERT(material == eval_material(pos));
#endif
		/*if (pp != eval_PP(pos)) {
			cout << " diff " << pp << " evalfull " << eval_PP(pos) << endl;
			UNREACHABLE;
		}*/
		//pp = eval_PP(pos);
#ifdef EVALHASH
		tte->save(pos.key(), pos.state()->bpp, pos.state()->wpp);
#endif

	FIND_HASH:

#ifdef EVAL_EFFECT
		//���莞�͌v�Z���Ȃ��@�����ق����������H
		if (!pos.is_incheck()) {
			effect = eval_effect(pos);
		}


		value = material + pp + effect;
#else
		value = material + pp;

#endif

		//�R���͊m�F�ς�
		//Value  full = eval_material(pos);
		////��̍����v�Z���ł��Ă��邩�`�F�b�N
		//if (value != full) {
		//	cout << pos << endl;
		//	ASSERT(0);
		//}

		//ASSERT(material == eval_material(pos));

#ifdef USETMP
		return (pos.sidetomove() == BLACK) ? value+tempo: -value+tempo;
#else
		return (pos.sidetomove() == BLACK) ? value : -value ;
#endif
		
	}


	//�����v�Z
	Value eval_diff_PP(const Position & pos)
	{
		//cout << "evaldiff" << endl;
		const StateInfo *now = pos.state();
		const StateInfo *prev = pos.state()->previous;
		const auto list = pos.evallist();
		int32_t bPP, wPP;
		bPP = prev->bpp;
		wPP = prev->wpp;

		ASSERT(bPP != Value_error&&wPP != Value_error);

	
		//dirtybonaP���[���ɂȂ��Ă���I
		//dirtyuniform���[���ɂȂ��Ă�I
		//dirty uniform�����������̂�do_move�ł��������Ȃ��ĂȂ����m�F����B
		const Eval::BonaPiece oldbp1_fb = now->dirtybonap_fb[0];
		const Eval::BonaPiece oldbp2_fb = now->dirtybonap_fb[1];
		const Eval::BonaPiece oldbp1_fw = now->dirtybonap_fw[0];
		const Eval::BonaPiece oldbp2_fw = now->dirtybonap_fw[1];
		//swap����\�������邽�߂�����const�ɂ��Ȃ�
		Eval::UniformNumber moveduniform1 = now->dirtyuniform[0];
		Eval::UniformNumber moveduniform2 = now->dirtyuniform[1];
		
		


		const Eval::BonaPiece* now_list_fb = list.bplist_fb;
		const Eval::BonaPiece* now_list_fw = list.bplist_fw;

		const Eval::BonaPiece newbp1_fb = list.bplist_fb[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fb = list.bplist_fb[now->dirtyuniform[1]];
		const Eval::BonaPiece newbp1_fw = list.bplist_fw[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fw = list.bplist_fw[now->dirtyuniform[1]];

		//����if 0�̕����̕��@�͂悭�Ȃ��Ǝv���Ă�����
		//��C�Ɍv�Z���đ������������������ォ��␳����Ƃ������j��AVX2���g���Čv�Z�������ꍇ�ɂ͌����Ă���̂�������Ȃ���...
#if 0
		Eval::BonaPiece old_list_fb[40];
		Eval::BonaPiece old_list_fw[40];
		//========================
		//oldlist�̍쐬
		//========================
		//�����̃R�s�[�Ɏ��Ԃ������肻���Ȃ񂾂�Ȃ�(memcopy�ŃR�s�[���邩)
		memcpy(old_list_fb, now_list_fb, sizeof(old_list_fb));
		memcpy(old_list_fw, now_list_fw, sizeof(old_list_fw));
		old_list_fb[moveduniform1] = oldbp1_fb;
		old_list_fw[moveduniform1] = oldbp1_fw;
		if (moveduniform2 != Num_Uniform) {
			old_list_fb[moveduniform2] = oldbp2_fb;
			old_list_fw[moveduniform2] = oldbp2_fw;
		}
		//assert�Ɉ������������ǖʂł�list�͂����Ɛ����ł��Ă����ifw���m�F���ĂȂ������I�I�j�ifw����肭�s���Ă����j
		//======================================
		//�A���S���Y���l�������K�v�����邩....�H������l���Ă��R�������v�����Ȃ��B
		//======================================
		/*

		ABCD
		ABCD
		BA CA CB DA DB DC �@
		ABED
		ABED
		BA EA EB DA DB DE�@�A
		�@-(CA CB CD CC)+(EA EB ED EE)=�A
		AEFD
		AEFD
		EA FA FE DA DE DF �B
		�@-(BA BC BD BB  CA CB CC CD)+BC+(EA EF ED EE FA FE FD FF)-EF=�B

		����ς�R���ł��܂������Ă���͂��Ȃ̂�...
		*/
		//����������
		//---------------------���������̎��������v�Z�~�X���Ă�H
		if (moveduniform2 == Num_Uniform) {
			ASSERT(is_ok(oldbp1_fb));
			ASSERT(is_ok(oldbp1_fw));
			ASSERT(is_ok(newbp1_fb));
			ASSERT(is_ok(newbp1_fw));
			ASSERT(is_ok(moveduniform1));
			//�����v�Z�̎��s
			//for (int i = 0; i < 40; i++) {
			//	//dirty�ɂȂ��Ă��܂�����bPP�������
			//	bPP -= PP[oldbp1_fb][old_list_fb[i]];//������pp[old1][old1]������������B
			//	wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old1]�𑫂�������B
			//	//�V����bPP�̂Ԃ�𑫂�
			//	bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]�𑫂�������
			//	wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]������������B
			//}
			//�Q�ɕ����邱�Ƃ�pp[i][i]�����Ԃ�Ȃ��悤�ɂ���
			for (int i = 0; i < moveduniform1; i++) {
				//dirty�ɂȂ��Ă��܂�����bPP�������
				bPP -= PP[oldbp1_fb][old_list_fb[i]];//������pp[old1][old1]������������B
				wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old1]�𑫂�������B
													 //�V����bPP�̂Ԃ�𑫂�
				bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]�𑫂�������
				wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]������������B
			}
			for (int i = moveduniform1 + 1; i < 40; i++) {
				//dirty�ɂȂ��Ă��܂�����bPP�������
				bPP -= PP[oldbp1_fb][old_list_fb[i]];//������pp[old1][old1]������������B
				wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old1]�𑫂�������B
													 //�V����bPP�̂Ԃ�𑫂�
				bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]�𑫂�������
				wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]������������B
			}
			/*
			�����ň��������A����������␳���Ȃ���΂Ȃ�Ȃ���PP[i][i]=0�ł���͂��Ȃ̂ł��̏ꍇ�␳�͕K�v�Ȃ�
			*/
			//�܂��ꉞ�m�F���Ă����B
			ASSERT(PP[oldbp1_fb][oldbp1_fb] == 0);
			ASSERT(PP[oldbp1_fw][oldbp1_fw] == 0);
			ASSERT(PP[newbp1_fb][newbp1_fb] == 0);
			ASSERT(PP[newbp1_fw][newbp1_fw] == 0);
		}
		else {
			ASSERT(is_ok(oldbp1_fb));
			ASSERT(is_ok(oldbp1_fw));
			ASSERT(is_ok(newbp1_fb));
			ASSERT(is_ok(newbp1_fw));
			ASSERT(is_ok(oldbp2_fb));
			ASSERT(is_ok(oldbp2_fw));
			ASSERT(is_ok(newbp2_fb));
			ASSERT(is_ok(newbp2_fw));
			ASSERT(is_ok(moveduniform1));
			ASSERT(is_ok(moveduniform2));
			//��������Q��
			//for (int i = 0; i < 40; i++) {
			//	//dirty�ɂȂ��Ă��܂�����������
			//	bPP -= PP[oldbp1_fb][old_list_fb[i]];//������pp[old1][old1]�@pp[old1][old2]�������Ă���
			//	bPP -= PP[oldbp2_fb][old_list_fb[i]];//������pp[old2][old2]�@pp[old2][old1]������������B
			//	wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old1]�@pp[old1][old2]�𑫂��Ă���B
			//	wPP += PP[oldbp2_fw][old_list_fw[i]];//������pp[old2][old2]�@pp[old2][old1]�𑫂�������B
			//	//�V�������𑫂�
			//	bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]  pp[new1][new2]�𑫂��Ă�
			//	bPP += PP[newbp2_fb][now_list_fb[i]];//������pp[new2][new2]�@pp[new2][new1]�𑫂�������
			//	wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]  pp[new1][new2]�������Ă�
			//	wPP -= PP[newbp2_fw][now_list_fw[i]];//������pp[new2][new2]  pp[new2][new1]����������
			//}
			////�����߂���␳����
			//bPP += PP[oldbp1_fb][oldbp2_fb];
			//wPP -= PP[oldbp1_fw][oldbp2_fw];
			////�����߂���␳����
			//bPP -= PP[newbp1_fb][newbp2_fb];
			//wPP += PP[newbp1_fw][newbp2_fw];
			for (int i = 0; i < moveduniform1; i++) {
				//dirty�ɂȂ��Ă��܂�����������
				bPP -= PP[oldbp1_fb][old_list_fb[i]];//�@pp[old1][old2]�������Ă���
				bPP -= PP[oldbp2_fb][old_list_fb[i]];//
				wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old2]�𑫂��Ă���B
				wPP += PP[oldbp2_fw][old_list_fw[i]];//
													 //�V�������𑫂�
				bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]  pp[new1][new2]�𑫂��Ă�
				bPP += PP[newbp2_fb][now_list_fb[i]];//
				wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]  pp[new1][new2]�������Ă�
				wPP -= PP[newbp2_fw][now_list_fw[i]];//
			}
			for (int i = moveduniform1 + 1; i < 40; i++) {
				//dirty�ɂȂ��Ă��܂�����������
				bPP -= PP[oldbp1_fb][old_list_fb[i]];//������pp[old1][old1]�@pp[old1][old2]�������Ă���
				bPP -= PP[oldbp2_fb][old_list_fb[i]];//
				wPP += PP[oldbp1_fw][old_list_fw[i]];//������pp[old1][old1]�@pp[old1][old2]�𑫂��Ă���B
				wPP += PP[oldbp2_fw][old_list_fw[i]];//
													 //�V�������𑫂�
				bPP += PP[newbp1_fb][now_list_fb[i]];//������pp[new1][new1]  pp[new1][new2]�𑫂��Ă�
				bPP += PP[newbp2_fb][now_list_fb[i]];//
				wPP -= PP[newbp1_fw][now_list_fw[i]];//������pp[new1][new1]  pp[new1][new2]�������Ă�
				wPP -= PP[newbp2_fw][now_list_fw[i]];//
			}
			//�Ώ̐�������͂��B
			ASSERT(PP[oldbp1_fb][oldbp2_fb] == PP[oldbp2_fb][oldbp1_fb]);
			ASSERT(PP[oldbp1_fw][oldbp2_fw] == PP[oldbp2_fw][oldbp1_fw]);
			ASSERT(PP[newbp1_fb][newbp2_fb] == PP[newbp2_fb][newbp1_fb]);
			ASSERT(PP[newbp1_fw][newbp2_fw] == PP[newbp2_fw][newbp1_fw]);
			//�܂��ꉞ�m�F���Ă����B
			ASSERT(PP[oldbp1_fb][oldbp1_fb] == 0);
			ASSERT(PP[oldbp1_fw][oldbp1_fw] == 0);
			ASSERT(PP[newbp1_fb][newbp1_fb] == 0);
			ASSERT(PP[newbp1_fw][newbp1_fw] == 0);
			ASSERT(PP[oldbp2_fb][oldbp2_fb] == 0);
			ASSERT(PP[oldbp2_fw][oldbp2_fw] == 0);
			ASSERT(PP[newbp2_fb][newbp2_fb] == 0);
			ASSERT(PP[newbp2_fw][newbp2_fw] == 0);
		}//2�������
#endif
#if 1
#define ADD_PP(oldbp,oldwp,newbp,newwp){ \
		bPP -= PP[oldbp][now_list_fb[i]];\
		bPP += PP[newbp][now_list_fb[i]];\
		wPP += PP[oldwp][now_list_fw[i]];\
		wPP -= PP[newwp][now_list_fw[i]];\
		}

		int i;

		
		if (moveduniform2 == Num_Uniform) {
			//��@������

			//���̂悤�ɂ��ćA�ɕ����邱�Ƃ�PP[old1][new1]�������Ă��܂��̂�h��,PP[new][new]�𑫂��Ă��܂��̂�h���B(����...�������͂�˂��牤...)
			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
			}

		}
		else {
			//��A������

			//pawn1<=dirty<dirty2<fe_end2�@�ɂ��Ă���
			if (moveduniform1 > moveduniform2) { std::swap(moveduniform1, moveduniform2); }

			for (i = 0; i < moveduniform1; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//�R����[old1][old2]���
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < moveduniform2; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);//�R����[old1][old2] [new1][new2]���
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			for (++i; i < 40; ++i) {
				ADD_PP(oldbp1_fb, oldbp1_fw, newbp1_fb, newbp1_fw);
				ADD_PP(oldbp2_fb, oldbp2_fw, newbp2_fb, newbp2_fw);
			}
			//�����ŉ�����Ă��܂���[old1][old2] [new1][new2]��␳����B
			bPP -= PP[oldbp1_fb][oldbp2_fb];
			bPP += PP[newbp1_fb][newbp2_fb];
			wPP += PP[oldbp1_fw][oldbp2_fw];
			wPP -= PP[newbp1_fw][newbp2_fw];

		}


#endif




		pos.state()->bpp = bPP;
		pos.state()->wpp = wPP;
#ifdef DIFFTEST


		/*
		diff�̒l�͑S�v�Z�ƈقȂ��Ă��܂��o�O�����������B
		����� PP[i][j]==PP[j][i]�łȂ����ߔ������Ă��܂����o�O�ł���ƍl������B
		dj[i][j]=dJ[j][i]�ł͂��邽�߁A�l�Ƃ��Ă͋߂��l�ɂȂ��Ă���̂ł����܂ő傫�ȃG���[�ł͂Ȃ��B
		�����
		PP[i][j]=PP[j][i]=(PP[i][j]+PP[j][i])/2�Ƃ��邱�ƂőΉ����悤����....
		*/



		//nullmove���ɍ����v�Z�����������Ȃ�I�I�I
		eval_PP(pos);
		//if (Value((bPP + wPP) / FV_SCALE) != eval_PP(pos)) {
		if (bPP!=pos.state()->bpp||wPP!=pos.state()->wpp) {

			cout << pos << endl;
			/*cout << "oldlist" << endl;
			for (int i = 0; i < Num_Uniform; i++) {
				cout <<"fb:"<< old_list_fb[i];
				cout << "fw:" << old_list_fw[i] << endl;
			}*/
			cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;
			cout << "bpp " << bPP << " " << pos.state()->bpp << endl;
			cout << "wpp " << wPP << " " << pos.state()->wpp << endl;
			UNREACHABLE;
		}
#endif
		ASSERT(((bPP + wPP) / FV_SCALE) < INT16_MAX);
		return Value((bPP + wPP) / FV_SCALE);

	}

	
	Value eval_PP(const Position & pos)
	{
		int32_t bPP=0, wPP=0;
		BonaPiece *list_fb=pos.evallist().bplist_fb, *list_fw=pos.evallist().bplist_fw;

		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				//list�̓��e�̏��Ԃ͂ǂ��ł������Ǝv���̂������...

				//���̕��@�ł͌��Ă͂����Ȃ������������Ă��܂��Ă���̂ŃZ�O�t�H���N����I�@
#if defined(_MSC_VER)
				bPP += PP[list_fb[i]][list_fb[j]];
				wPP -= PP[list_fw[i]][list_fw[j]];
#endif
#if defined(__GNUC__)
				bPP += PP[pos.evallist().bplist_fb[i]][pos.evallist().bplist_fb[j]];
				wPP -= PP[pos.evallist().bplist_fw[i]][pos.evallist().bplist_fw[j]];
#endif
				

			}
		}
		pos.state()->bpp = (bPP);
		pos.state()->wpp = wPP;
		//�]���l�z���g��16bit�Ŏ��܂邩�ǂ����m�F
		ASSERT(bPP != Value_error&&wPP != Value_error);
		ASSERT(abs(bPP + wPP) / FV_SCALE < INT16_MAX);

		return Value((bPP+wPP) / FV_SCALE);
	}


	/*
	AVX2��p�����]���֐��v�Z�������B

	nodchip����̖{���Q�l�ɓǂݐi�߂Ă����B

	PP��list[40]��p���Čv�Z���s���̂�
	8�̗v�f���̌v�Z�Ŋ��������邱�Ƃ��ł���B

	PP�v�f��32bit

	_mm256_i32gather_epi32
	https://www.bing.com/search?q=_mm256_i32gather_epi32&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127

	_mm256_load_si256
	https://www.bing.com/search?q=_mm256_load_si256&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127

	*/
#if defined(_MSC_VER)
	
#if defined(__GNUC__)

#endif


#ifdef HAVE_AVX2
	Value eval_allPP_AVX2(const Position & pos)
	{
		int bPP = 0, wPP = 0;
		BonaPiece *list_fb = pos.evallist().bplist_fb, *list_fw = pos.evallist().bplist_fw;

		__m256i zero = _mm256_setzero_si256();
		__m256i bPP256 = zero, wPP256 = zero;

		//40�v�f�ɑ΂��Čv�Z���s��
		for (int i = 0; i < 40; ++i) {

			const BonaPiece k0_b = list_fb[i], k0_w = list_fw[i];

			//pointer pp black||white
			const auto* p_pp_b = PP[k0_b];
			const auto* p_pp_w = PP[k0_w];

			int j = 0;
			//8�v�f��C�Ɍv�Z����
			//bonapiece��int16_t�ɂ����16�v�f��C�Ɍv�Z�ł����ˁH�H
			for (; j + 8 < i; j += 8) {

				//bonapiece��index��8�v�f���[�h����B
#if defined(_MSC_VER)
				__m256i indices_fb = _mm256_load_si256(reinterpret_cast<const __m256i*>(&list_fb[j]));
				__m256i indices_fw = _mm256_load_si256(reinterpret_cast<const __m256i*>(&list_fw[j]));
#endif
#if defined(__GNUC__)
				__m256i indices_fb = _mm256_load_si256(reinterpret_cast<const __m256i*>(&pos.evallist().bplist_fb[j]));
				__m256i indices_fw = _mm256_load_si256(reinterpret_cast<const __m256i*>(&pos.evallist().bplist_fb[j]));
#endif
				

				//p_pp_b��p_pp_w����8�v�f���M���U�[���Ă���B
				/*
				__m256i _mm256_i32gather_epi32(int const * base, __m256i vindex, const int scale);
				result[31:0] = mem[base+vindex[31:0]*scale];
				result[63:32] = mem[base+vindex[63:32]*scale];
				result[95:64] = mem[base+vindex[95:64]*scale];
				result127:96] = mem[base+vindex[127:96]*scale];
				result[159:128] = mem[base+vindex[159:128]*scale];
				result[191:160] = mem[base+vindex[191:160]*scale];
				result[223:192] = mem[base+vindex[223:192]*scale];
				result[255:224] = mem[base+vindex[255:224]*scale];

				������������32bit�ϐ���ǂݎ�낤�Ǝv������scale��1�Ȃ̂ł�...?�����߂�����
				�܂�tanuki-����̖{�ɍ��킹��4�ɂ��Ă�����...
				*/
				__m256i b = _mm256_i32gather_epi32(reinterpret_cast<const int*>(p_pp_b), indices_fb, 4);
				/*for (int k = 0; k < 8; k++) {
					int32_t a = PP[k0_b][list_fb[j + k]];
					int32_t aa = b.m256i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}*/
				__m256i w = _mm256_i32gather_epi32(reinterpret_cast<const int*>(p_pp_w), indices_fw, 4);
				//������ASSERT�ɂ������ĂȂ����Ă��Ƃ�PP����̓ǂݎ��͐������ł��Ă���
				/*for (int k = 0; k < 8; k++) {
					int32_t a = PP[k0_w][list_fw[j + k]];
					int32_t aa = w.m256i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}*/

				bPP256 = _mm256_add_epi32(bPP256, b);
				wPP256 = _mm256_add_epi32(wPP256, w);

				/*
				//����128bit��16bit�ϐ�����32bit�����ɕϊ�����
				__m256i blo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(b, 0));//0�Ƃ����̂�128*0
				__m256i wlo = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w, 0));
				for (int k = 0; k < 4; k++) {
					int32_t a = PP[k0_w][list_fw[j + k]];
					int32_t aa = wlo.m256i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}
				//bpp256,wpp256�ɑ������킹��
				bPP256 = _mm256_add_epi32(bPP256, blo);
				wPP256 = _mm256_add_epi32(wPP256, wlo);

				//���128bit��16bit�ϐ�����32bit�����ɕϊ�����
				//_mm256_extracti128_si256 �F���or����128bit��xmm���W�X�^�Ɋi�[����
				//_mm256_cvtepi16_epi32 :16 �r�b�g�̕����t���������� 32 �r�b�g�̐����ւ̕����g���p�b�N�h�ړ���������s : https://www.bing.com/search?q=_mm256_cvtepi16_epi32&pc=cosp&ptag=C1AE89FD93123&form=CONBDF&conlogo=CT3210127
				__m256i bhi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(b, 1));//0�Ƃ����̂�128*0
				__m256i whi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w, 1));
				for (int k = 4; k < 8; k++) {
					int32_t a = PP[k0_w][list_fw[j + k]];
					int32_t aa = whi.m256i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}
				//bpp256,wpp256�ɑ������킹��
				bPP256 = _mm256_add_epi32(bPP256, bhi);
				wPP256 = _mm256_add_epi32(wPP256, whi);*/
			}
			//4�v�f��C�ɂł���Ƃ���͈�C�ɂ���
			for (; j + 4 < i; j += 4) {
				__m128i indices_fb = _mm_load_si128(reinterpret_cast<const __m128i*>(&list_fb[j]));
				__m128i indices_fw = _mm_load_si128(reinterpret_cast<const __m128i*>(&list_fw[j]));
				__m128i b = _mm_i32gather_epi32(reinterpret_cast<const int*>(p_pp_b), indices_fb, 4);
				/*for (int k = 0; k < 4; k++) {
					int32_t a = PP[k0_b][list_fb[j + k]];
					int32_t aa = b.m128i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}*/
				__m128i w = _mm_i32gather_epi32(reinterpret_cast<const int*>(p_pp_w), indices_fw, 4);
				/*for (int k = 0; k < 4; k++) {
					int32_t a = PP[k0_w][list_fw[j + k]];
					int32_t aa = w.m128i_i32[k];
					if (a != aa) {
						cout << a << " " << aa << endl;
						ASSERT(0);
					}
				}*/

				//https://www.xlsoft.com/jp/products/intel/compilers/ccw/12/ug/intref_cls/common/intref_avx_castsi128_si256.htm
				//���128bit�͖���`�Ƃ����̂��C�ɂȂ��... ���܂������Ă�̂ł���ł�����͂�...
				bPP256 = _mm256_add_epi32(bPP256, _mm256_castsi128_si256(b));
				wPP256 = _mm256_add_epi32(wPP256, _mm256_castsi128_si256(w));

			}

			//�c��
			for (; j < i; ++j) {
#if defined(_MSC_VER)
				bPP += p_pp_b[list_fb[j]];
				wPP -= p_pp_w[list_fw[j]];
#endif			
#if defined(__GNUC__)
				bPP += p_pp_b[pos.evallist().bplist_fb[j]];
				wPP -= p_pp_w[pos.evallist().bplist_fb[j]];
#endif
				
			}

		}
		
		//256bit��bPPsum���܂Ƃ߂�
		//����Ȃ񂩂���AVX2���߂Ȃ��̂��H�Ȃ��炵��
#if defined(_MSC_VER)
		for (int l = 0; l < 8; l++) {
			bPP += (Value)bPP256.m256i_i32[l];
			wPP -= (Value)wPP256.m256i_i32[l];
			
		}

#elif defined(__GNUC__) 
		bPP += _mm256_extract_epi32(bPP256, 0) + _mm256_extract_epi32(bPP256, 1) + _mm256_extract_epi32(bPP256, 2) +
			_mm256_extract_epi32(bPP256, 3) + _mm256_extract_epi32(bPP256, 4) + _mm256_extract_epi32(bPP256, 5) +
			_mm256_extract_epi32(bPP256, 6) + _mm256_extract_epi32(bPP256, 7);
		wPP += _mm256_extract_epi32(wPP256, 0) + _mm256_extract_epi32(wPP256, 1) + _mm256_extract_epi32(wPP256, 2) +
			_mm256_extract_epi32(wPP256, 3) + _mm256_extract_epi32(wPP256, 4) + _mm256_extract_epi32(wPP256, 5) +
			_mm256_extract_epi32(wPP256, 6) + _mm256_extract_epi32(wPP256, 7);
#endif
		


		pos.state()->bpp = (bPP);
		pos.state()->wpp = wPP;
		//�]���l�z���g��16bit�Ŏ��܂邩�ǂ����m�F
		ASSERT(abs(bPP + wPP) / FV_SCALE < INT16_MAX);
	/*	Eval::eval_PP(pos);
		if (bPP != pos.state()->bpp || wPP != pos.state()->wpp) {
			cout << bPP << " " << wPP << endl;
			cout << pos.state()->bpp << " " << pos.state()->wpp << endl;
			ASSERT(0);
		}*/



		return Value((bPP + wPP) / FV_SCALE);


	}

#endif


#endif


#endif
	//�����v�Z

	void BonaPList::makebonaPlist(const Position & pos)
	{
		//������
		init();

		//��ԍ��ׂ̈̃J�E���^(�O���[�o���ł͂Ȃ��֐����ɒu�����Ƃł��̊֐��ɗ���x�ɒl�������������͂�)
		UniformNumber uniform[9] = {
			Num_Uniform,
			pawn1,//pawn1��0�ɂ��Ȃ��ƃ��X�g[0]�ɋ󂫂��o�Ă��܂��I�I
			lance1,
			knight1,
			silver1,
			bishop1,
			rook1,
			gold1,
			king1,
		};

		//�Տ�
		Bitboard occ = pos.occ_all() ;

		while (occ.isNot()) {

			Square sq = occ.pop();
			Piece pc = pos.piece_on(sq);
			Piece pt = piece_type(pc);
			//Color c = piece_color(pc);

			switch (pt)
			{
			case PAWN:case PRO_PAWN:
				bplist_fb[uniform[PAWN]] = bonapiece(sq, pc);
				bplist_fw[uniform[PAWN]] = bonapiece(hihumin_eye(sq),inverse(pc));
				sq2Uniform[sq] = uniform[PAWN];
				uniform[PAWN]++;
				break;
			case LANCE:case PRO_LANCE:
				bplist_fb[uniform[LANCE]] = bonapiece(sq, pc);
				bplist_fw[uniform[LANCE]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[LANCE];
				uniform[LANCE]++;
				break;
			case KNIGHT:case PRO_NIGHT:
				bplist_fb[uniform[KNIGHT]] = bonapiece(sq, pc);
				bplist_fw[uniform[KNIGHT]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[KNIGHT];
				uniform[KNIGHT]++;
				break;
			case SILVER:case PRO_SILVER:
				bplist_fb[uniform[SILVER]] = bonapiece(sq, pc);
				bplist_fw[uniform[SILVER]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[SILVER];
				uniform[SILVER]++;
				break;
			case BISHOP:case UNICORN:
				bplist_fb[uniform[BISHOP]] = bonapiece(sq, pc);
				bplist_fw[uniform[BISHOP]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[BISHOP];
				uniform[BISHOP]++;
				break;
			case ROOK:case DRAGON:
				bplist_fb[uniform[ROOK]] = bonapiece(sq, pc);
				bplist_fw[uniform[ROOK]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[ROOK];
				uniform[ROOK]++;
				break;
			case GOLD:
				bplist_fb[uniform[GOLD]] = bonapiece(sq, pc);
				bplist_fw[uniform[GOLD]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[GOLD];
				uniform[GOLD]++;
				break;
			case KING:
				bplist_fb[uniform[KING]] = bonapiece(sq, pc);
				bplist_fw[uniform[KING]] = bonapiece(hihumin_eye(sq), inverse(pc));
				sq2Uniform[sq] = uniform[KING];
				uniform[KING]++;
				break;
			default:
				cout <<endl<< pos << endl;
				cout << pos.occ_all() << endl;
				UNREACHABLE;
				break;
			}

		}//end of while occ

	
		 //���
		int num = 0;

		for (Color hc = BLACK; hc < ColorALL; hc++) {

			const Hand& h = pos.hand(hc);
			if (have_hand(h)) {

				for (Piece pt = PAWN; pt < KING; pt++) {

					//�R���͈Ӑ}�����L�q
					num = num_pt(h, pt);
					if (num !=0) {

						for (int i = 1; i < num + 1; i++) {
							bplist_fb[uniform[pt]] = bonapiece(hc, pt, i);
							bplist_fw[uniform[pt]] = bonapiece(opposite(hc), pt, i);
							hand2Uniform[hc][pt][i] = uniform[pt];
							uniform[pt]++;
						}
					}
				}
			}
		}
	}//end of makebplist


	void BonaPList::print_bplist()
	{

		for (int i = 0; i < Num_Uniform; i++) {
			cout << "fb:" << bplist_fb[i]<<" "<<bp2sq(bplist_fb[i])<<" "<<bpwithoutsq(bplist_fb[i])<<bp2color(bplist_fb[i])<<" "<<bp2piece.bp_to_piece(bpwithoutsq(bplist_fb[i]))<<endl;
			cout << "fw:" << bplist_fw[i] << " " << bp2sq(bplist_fw[i]) << " " << bpwithoutsq(bplist_fw[i]) << bp2color(bplist_fw[i]) << " " << bp2piece.bp_to_piece(bpwithoutsq(bplist_fw[i])) <<endl;
			cout << "inverse fb:" << inversebonapiece(bplist_fb[i]) << "inverse fw:" << inversebonapiece(bplist_fw[i]) << endl;
			cout << "sym" << sym_rightleft(bplist_fb[i]) << endl << endl;;
			ASSERT(inversebonapiece(bplist_fb[i]) == bplist_fw[i]);
			ASSERT(inversebonapiece(bplist_fw[i]) == bplist_fb[i]);
		}

	}//endof print bp
	

	void BonaPList::list_check() {
		for (int i = 0; i < Num_Uniform; i++) {
			ASSERT(inversebonapiece(bplist_fb[i]) == bplist_fw[i]);
			ASSERT(inversebonapiece(bplist_fw[i]) == bplist_fb[i]);
		}
	}
#ifdef MISC
	std::ostream & operator<<(std::ostream & os, const Eval::BonaPiece bp)
	{
		//bp<fe_handend�ł���΂���͎��
		if (bp < fe_hand_end) {

			//��������������ƂȂ�Ƃ��Ȃ�悤�Ȃ�������񂾂���,,,
			if (f_hand_pawn <= bp&&bp < e_hand_pawn) {
				cout << "f_handpawn + " << int(bp - f_hand_pawn) << endl;
			}
			else if (e_hand_pawn <= bp&&bp < f_hand_lance) {
				cout << "e_handpawn + " << int(bp - e_hand_pawn) << endl;
			}
			else if (f_hand_lance <= bp&&bp < e_hand_lance) {
				cout << "f_hand lance + " << int(bp - f_hand_lance) << endl;
			}
			else if (e_hand_lance <= bp&&bp < f_hand_knight) {
				cout << "e_handlance + " << int(bp - e_hand_lance) << endl;
			}
			else if (f_hand_knight <= bp&&bp < e_hand_knight) {
				cout << "f_handknight + " << int(bp - f_hand_knight) << endl;
			}
			else if (e_hand_knight <= bp&&bp < f_hand_silver) {
				cout << "e_hand_knight + " << int(bp - e_hand_knight) << endl;
			}
			else if (bp < e_hand_silver) {//�����̓R�������ŏ\�����H�H
				cout << "f_hand_silver + " << int(bp - f_hand_silver) << endl;
			}
			else if (bp < f_hand_gold) {
				cout << "e_hand_silver + " << int(bp - e_hand_silver) << endl;
			}
			else if (bp < e_hand_gold) {
				cout << "f_hand_gold + " << int(bp - f_hand_gold) << endl;
			}
			else if (bp < f_hand_bishop) {
				cout << "e_hand_gold + " << int(bp - e_hand_gold) << endl;
			}
			else if (bp < e_hand_bishop) {
				cout << "f_hand_bishop + " << int(bp - f_hand_bishop) << endl;
			}
			else if (bp < f_hand_rook) {
				cout << "e_hand_bishop + " << int(bp - e_hand_bishop) << endl;
			}
			else if (bp < e_hand_rook) {
				cout << "f_hand_rook + " << int(bp - f_hand_rook) << endl;
			}
			else if (bp < fe_hand_end) {
				cout << "e_hand_rook + " << int(bp - e_hand_rook) << endl;
			}

		}
		else {
			//�Տ�̋�
			if (f_pawn <= bp&&bp < e_pawn) {
				cout << "f_pawn + " << int(bp - f_pawn) << endl;
			}
			else if (e_pawn <= bp&&bp < f_lance) {
				cout << "e_pawn + " << int(bp - e_pawn) << endl;
			}
			else if (f_lance <= bp&&bp < e_lance) {
				cout << "f_lance + " << int(bp - f_lance) << endl;
			}
			else if (e_lance <= bp&&bp < f_knight) {
				cout << "e_lance + " << int(bp - e_lance) << endl;
			}
			else if (f_knight <= bp&&bp < e_knight) {
				cout << "f_knight + " << int(bp - f_knight) << endl;
			}
			else if (e_knight <= bp &&bp< f_silver) {
				cout << "e_knight + " << int(bp - e_knight) << endl;
			}
			else if (f_silver <= bp&&bp < e_silver) {
				cout << "f_silver + " << int(bp - f_silver) << endl;
			}
			else if (e_silver <= bp&&bp < f_gold) {
				cout << "e_silver + " << int(bp - e_silver) << endl;
			}
			else if (f_gold <= bp&&bp < e_gold) {
				cout << "f_gold + " << int(bp - f_gold) << endl;
			}
			else if (e_gold <= bp&&bp < f_bishop) {
				cout << "e_gold + " << int(bp - e_gold) << endl;
			}
			else if (bp<e_bishop) {
				cout << "f_bishop + " << int(bp - f_bishop) << endl;
			}
			else if (bp<f_unicorn) {
				cout << "e_bishop + " << int(bp - e_bishop) << endl;
			}
			else if (bp < e_unicorn) {
				cout << "f_unicorn + " << int(bp - f_unicorn) << endl;
			}
			else if (bp < f_rook) {
				cout << "e_unicorn + " << int(bp - e_unicorn) << endl;
			}
			else if (bp < e_rook) {
				cout << "f_rook + " << int(bp - f_rook) << endl;
			}
			else if (bp < f_dragon) {
				cout << "e_rook + " << int(bp - e_rook) << endl;
			}
			else if (bp < e_dragon) {
				cout << "f_dragon + " << int(bp - f_dragon) << endl;
			}
			else if (bp < f_king) {
				cout << "e_dragon + " << int(bp - e_dragon) << endl;
			}
			else if (bp < e_king) {
				cout << "f_king + " << int(bp - f_king) << endl;
			}
			else if (bp < fe_end2) {
				cout << "e_king + " << int(bp - e_king) << endl;
			}
			else {
				UNREACHABLE;
			}
		}
		return os;
	}
#endif
	//�R�}�����`�F�b�N���邽�߂̊֐��i�J�c���������R�}���̒l�������������������ŕ|���Ȃ��Ă����̂Ŋm�F���Ă����B�j
	void komawari_check()
	{
		for (Piece p = B_PAWN; p < PC_ALL;p++) {

			if (p < KING) {
				cout << p << " piecevalue " << piece_value[p]<< " capture" << capture_value[p]  << " promote" << diff_promote[p] << endl;
			}
			else {
				cout << p << " piecevalue " << piece_value[p] << " capture" << capture_value[p] << endl;
			}
		}
	}


	//bonapiece�̍��E�𔽓]������֐�
	BonaPiece sym_rightleft(const BonaPiece bp) {

		//����bp�͍��E���]�Ȃ�ďo���Ȃ��̂ł��̂܂܂̒l��Ԃ��B
		if (bp < fe_hand_end) {
			return bp;
		}

		int sq, symsq;
		//���ȊO

		//�c�^bitboard�Ȃ̂�rank=sq%9,file=sq/9
		//BonaPiece bprank, bpfile;
		//�Տ�̋�
		if (f_pawn <= bp&&bp < e_pawn) {
			sq = bp - f_pawn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_pawn + symsq);
		}
		else if (e_pawn <= bp&&bp < f_lance) {
			sq = bp - e_pawn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_pawn + symsq);
		}
		else if (f_lance <= bp&&bp < e_lance) {
			sq = bp - f_lance;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_lance + symsq);
		}
		else if (e_lance <= bp&&bp < f_knight) {
			sq = bp - e_lance;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_lance + symsq);
		}
		else if (f_knight <= bp&&bp < e_knight) {
			sq = bp - f_knight;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_knight + symsq);
		}
		else if (e_knight <= bp &&bp< f_silver) {
			sq = bp - e_knight;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_knight + symsq);
		}
		else if (f_silver <= bp&&bp < e_silver) {
			sq = bp - f_silver;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_silver + symsq);
		}
		else if (e_silver <= bp&&bp < f_gold) {
			sq = bp - e_silver;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_silver + symsq);
		}
		else if (f_gold <= bp&&bp < e_gold) {
			sq = bp - f_gold;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_gold + symsq);
		}
		else if (e_gold <= bp&&bp < f_bishop) {
			sq = bp - e_gold;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_gold + symsq);
		}
		else if (bp<e_bishop) {
			sq = bp - f_bishop;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_bishop + symsq);
		}
		else if (bp<f_unicorn) {
			sq = bp - e_bishop;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_bishop + symsq);
		}
		else if (bp < e_unicorn) {
			sq = bp - f_unicorn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_unicorn + symsq);
		}
		else if (bp < f_rook) {
			sq = bp - e_unicorn;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_unicorn + symsq);
		}
		else if (bp < e_rook) {
			sq = bp - f_rook;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_rook + symsq);
		}
		else if (bp < f_dragon) {
			sq = bp - e_rook;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_rook + symsq);
		}
		else if (bp < e_dragon) {
			sq = bp - f_dragon;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_dragon + symsq);
		}
		else if (bp < f_king) {
			sq = bp - e_dragon;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_dragon + symsq);
		}
		else if (bp < e_king) {
			sq = bp - f_king;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(f_king + symsq);
		}
		else if (bp < fe_end2) {
			sq = bp - e_king;
			symsq = sym_rl_sq(Square(sq));
			return BonaPiece(e_king + symsq);
		}
		else {
			UNREACHABLE;
			return BONA_PIECE_ZERO;
		}
		
	}


#if defined(LEARN) && defined(EVAL_PP)
	
	//void Eval::param_sym_ij() {
	void param_sym_ij() {
		

		
		
		bool check[fe_end2][fe_end2] = { false };
		memset(check, false, sizeof(check));


		for (BonaPiece i = BONA_PIECE_ZERO; i < fe_end2; i++) {

			for (BonaPiece j = BONA_PIECE_ZERO; j < fe_end2; j++) {

				if (check[i][j] != true) {

					check[i][j] =check[j][i]= true;

					int32_t a = PP[i][j], b = PP[j][i];
					PP[i][j] = PP[j][i] = (a + b) / 2;

				}


			}

		}

		write_FV();
		read_FV();
	}
	
#endif


#if defined(LEARN) && defined(EVAL_KPPT)
	void Eval::param_sym_ij() {

		//bool check_KPP[82][fe_end][fe_end] = { false };

		//for (Square ksq = SQ_ZERO; ksq < SQ_NUM; ksq++) {
		//	//kpp-----------------------------------------------------------
		//	for (BonaPiece bp1 = BONA_PIECE_ZERO; bp1 < fe_end; bp1++) {
		//		for (BonaPiece bp2 = BONA_PIECE_ZERO; bp2 < fe_end; bp2++) {

		//			if (check_KPP[ksq][bp1][bp2] == false) {
		//				check_KPP[ksq][bp1][bp2] = check_KPP[ksq][bp2][bp1] = true;
		//				std::array<int16_t, 2> a = KPP[ksq][bp1][bp2], b = KPP[ksq][bp2][bp1];
		//				KPP[ksq][bp1][bp2] = KPP[ksq][bp2][bp1] = (a + b) / 2;
		//			}
		//		}
		//	}
		//}
		
		

	}
#endif











}