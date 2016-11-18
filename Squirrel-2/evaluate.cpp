#include "evaluate.h"
#include "position.h"


namespace Eval {


	int32_t PP[fe_end2][fe_end2];

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
			return BonaPiece(f_hand_bishop + (num-1));
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

	Value eval(const Position & pos)
	{
		Value pp,value;
		Value material= pos.state()->material;


		//�v�Z�ς�
		if (pos.state()->bpp != Value_error) {
			ASSERT(pos.state()->wpp != Value_error);
			pp=Value((pos.state()->bpp + pos.state()->wpp) / FV_SCALE);

		}
		else if (pos.state()->previous->bpp != Value_error) {
			//�����v�Z�\
			ASSERT(pos.state()->previous->wpp != Value_error);

			pp = eval_diff_PP(pos);
		}
		else {
			//�S�v�Z�I
			pp = eval_PP(pos);
		}

		/*if (pp != eval_PP(pos)) {
			cout << " diff " << pp << " evalfull " << eval_PP(pos) << endl;
			UNREACHABLE;
		}*/
		//pp = eval_PP(pos);

		value = material + pp;

		//�R���͊m�F�ς�
		//Value  full = eval_material(pos);
		////��̍����v�Z���ł��Ă��邩�`�F�b�N
		//if (value != full) {
		//	cout << pos << endl;
		//	ASSERT(0);
		//}


		return (pos.sidetomove() == BLACK) ? value : -value;

		/*Value value = eval_material(pos);
		return (pos.sidetomove() == BLACK) ? value : -value;*/
	}

	Value eval_PP(const Position & pos)
	{
		int32_t bPP=0, wPP=0;
		BonaPiece *list_fb=pos.evallist().bplist_fb, *list_fw=pos.evallist().bplist_fw;

		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < i; j++) {
				//list�̓��e�̏��Ԃ͂ǂ��ł������Ǝv���̂������...
				bPP += PP[list_fb[i]][list_fb[j]];
				wPP -= PP[list_fw[i]][list_fw[j]];
			}
		}
		pos.state()->bpp = (bPP);
		pos.state()->wpp = wPP;
		//�]���l�z���g��16bit�Ŏ��܂邩�ǂ����m�F
		ASSERT(abs(bPP + wPP) / FV_SCALE < INT16_MAX);

		return Value((bPP+wPP) / FV_SCALE);
	}

	//�����v�Z
	Value eval_diff_PP(const Position & pos)
	{
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
		const Eval::UniformNumber moveduniform1 = now->dirtyuniform[0];
		const Eval::UniformNumber moveduniform2 = now->dirtyuniform[1];
		
		


		const Eval::BonaPiece* now_list_fb = list.bplist_fb;
		const Eval::BonaPiece* now_list_fw = list.bplist_fw;

		const Eval::BonaPiece newbp1_fb = list.bplist_fb[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fb = list.bplist_fb[now->dirtyuniform[1]];
		const Eval::BonaPiece newbp1_fw = list.bplist_fw[now->dirtyuniform[0]];
		const Eval::BonaPiece newbp2_fw = list.bplist_fw[now->dirtyuniform[1]];


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
			for (int i = moveduniform1+1; i < 40; i++) {
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
			for (int i = moveduniform1+1; i < 40; i++) {
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
			ASSERT(PP[oldbp1_fw][oldbp2_fw]== PP[oldbp2_fw][oldbp1_fw]);
			ASSERT(PP[newbp1_fb][newbp2_fb]== PP[newbp2_fb][newbp1_fb]);
			ASSERT(PP[newbp1_fw][newbp2_fw]== PP[newbp2_fw][newbp1_fw]);

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

		pos.state()->bpp = bPP;
		pos.state()->wpp = wPP;

		eval_PP(pos);
		//if (Value((bPP + wPP) / FV_SCALE) != eval_PP(pos)) {
		if (bPP!=pos.state()->bpp||wPP!=pos.state()->wpp) {

			cout << pos << endl;
			cout << "oldlist" << endl;
			for (int i = 0; i < Num_Uniform; i++) {
				cout <<"fb:"<< old_list_fb[i];
				cout << "fw:" << old_list_fw[i] << endl;
			}
			cout << " diff " << Value((bPP + wPP) / FV_SCALE) << " evalfull " << eval_PP(pos) << endl;
			cout << "bpp " << bPP << " " << pos.state()->bpp << endl;
			cout << "wpp " << wPP << " " << pos.state()->wpp << endl;
			UNREACHABLE;
		}

		return Value((bPP + wPP) / FV_SCALE);

	}//�����v�Z

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
		Bitboard occ = pos.occ_all();

		while (occ.isNot()) {

			Square sq = occ.pop();
			Piece pc = pos.piece_on(sq);
			Piece pt = piece_type(pc);
			Color c = piece_color(pc);

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

					if (num = num_pt(h, pt)) {

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
			cout << "fb:" << bplist_fb[i];
			cout << "fw:" << bplist_fw[i] << endl;
		}



		//cout << "from black" << endl << endl;

		//for (int i = 0; i < Num_Uniform; i++) {

		//	auto bp = bplist_fb[i];

		//	//bp<fe_handend�ł���΂���͎��
		//	if (bp < fe_hand_end) {

		//		//��������������ƂȂ�Ƃ��Ȃ�悤�Ȃ�������񂾂���,,,
		//		if (f_hand_pawn <= bp&&bp < e_hand_pawn) {
		//			cout << "f_handpawn + " << int(bp - f_hand_pawn) << endl;
		//		}
		//		else if (e_hand_pawn <= bp&&bp < f_hand_lance) {
		//			cout << "e_handpawn + " << int(bp - e_hand_pawn) << endl;
		//		}
		//		else if (f_hand_lance <= bp&&bp < e_hand_lance) {
		//			cout << "f_hand lance + " << int(bp - f_hand_lance) << endl;
		//		}
		//		else if (e_hand_lance <= bp&&bp < f_hand_knight) {
		//			cout << "e_handlance + " << int(bp - e_hand_lance) << endl;
		//		}
		//		else if (f_hand_knight <= bp&&bp < e_hand_knight) {
		//			cout << "f_handknight + " << int(bp - f_hand_knight) << endl;
		//		}
		//		else if (e_hand_knight <= bp&&bp < f_hand_silver) {
		//			cout << "e_hand_knight + " << int(bp - e_hand_knight) << endl;
		//		}
		//		else if (bp < e_hand_silver) {//�����̓R�������ŏ\�����H�H
		//			cout << "f_hand_silver + " << int(bp - f_hand_silver) << endl;
		//		}
		//		else if (bp < f_hand_gold) {
		//			cout << "e_hand_silver + " << int(bp - e_hand_silver) << endl;
		//		}
		//		else if (bp < e_hand_gold) {
		//			cout << "f_hand_gold + " << int(bp - f_hand_gold) << endl;
		//		}
		//		else if(bp < f_hand_bishop) {
		//			cout << "e_hand_gold + " << int(bp - e_hand_gold) << endl;
		//		}
		//		else if (bp < e_hand_bishop) {
		//			cout << "f_hand_bishop + " << int(bp - f_hand_bishop) << endl;
		//		}
		//		else if (bp < f_hand_rook) {
		//			cout << "e_hand_bishop + " << int(bp - e_hand_bishop) << endl;
		//		}
		//		else if (bp < e_hand_rook) {
		//			cout << "f_hand_rook + " << int(bp - f_hand_rook) << endl;
		//		}
		//		else if (bp < fe_hand_end) {
		//			cout << "e_hand_rook + " << int(bp - e_hand_rook) << endl;
		//		}

		//	}
		//	else {
		//		//�Տ�̋�
		//		if (f_pawn <= bp&&bp < e_pawn) {
		//			cout << "f_pawn + " << int(bp - f_pawn) << endl;
		//		}
		//		else if (e_pawn <= bp&&bp < f_lance) {
		//			cout << "e_pawn + " << int(bp - e_pawn) << endl;
		//		}
		//		else if (f_lance <= bp&&bp < e_lance) {
		//			cout << "f_lance + " << int(bp - f_lance) << endl;
		//		}
		//		else if (e_lance <= bp&&bp < f_knight) {
		//			cout << "e_lance + " << int(bp - e_lance) << endl;
		//		}
		//		else if (f_knight <= bp&&bp < e_knight) {
		//			cout << "f_knight + " << int(bp - f_knight) << endl;
		//		}
		//		else if (e_knight <= bp &&bp< f_silver) {
		//			cout << "e_knight + " << int(bp - e_knight) << endl;
		//		}
		//		else if (f_silver <= bp&&bp < e_silver) {
		//			cout << "f_silver + " << int(bp - f_silver) << endl;
		//		}
		//		else if (e_silver <= bp&&bp < f_gold) {
		//			cout << "e_silver + " << int(bp - e_silver) << endl;
		//		}
		//		else if (f_gold <= bp&&bp < e_gold) {
		//			cout << "f_gold + " << int(bp - f_gold) << endl;
		//		}
		//		else if (e_gold <= bp&&bp < f_bishop) {
		//			cout << "e_gold + " << int(bp - e_gold) << endl;
		//		}
		//		else if (bp<e_bishop) {
		//			cout << "f_bishop + " << int(bp - f_bishop) << endl;
		//		}
		//		else if (bp<f_unicorn) {
		//			cout << "e_bishop + " << int(bp - e_bishop) << endl;
		//		}
		//		else if (bp < e_unicorn) {
		//			cout << "f_unicorn + " << int(bp - f_unicorn) << endl;
		//		}
		//		else if (bp < f_rook) {
		//			cout << "e_unicorn + " << int(bp - e_unicorn) << endl;
		//		}
		//		else if (bp < e_rook) {
		//			cout << "f_rook + " << int(bp - f_rook) << endl;
		//		}
		//		else if (bp < f_dragon) {
		//			cout << "e_rook + " << int(bp - e_rook) << endl;
		//		}
		//		else if (bp < e_dragon) {
		//			cout << "f_dragon + " << int(bp - f_dragon) << endl;
		//		}
		//		else if (bp < f_king) {
		//			cout << "e_dragon + " << int(bp - e_dragon) << endl;
		//		}
		//		else if (bp < e_king) {
		//			cout << "f_king + " << int(bp - f_king) << endl;
		//		}
		//		else if (bp < fe_end2) {
		//			cout << "e_king + " << int(bp - e_king) << endl;
		//		}
		//		else {
		//			UNREACHABLE;
		//		}
		//	}
		//}//from black

		//cout << endl << "from white" << endl << endl;

		//for (int i = 0; i < Num_Uniform; i++) {

		//	auto bp = bplist_fw[i];

		//	//bp<fe_handend�ł���΂���͎��
		//	if (bp < fe_hand_end) {

		//		//��������������ƂȂ�Ƃ��Ȃ�悤�Ȃ�������񂾂���,,,
		//		if (f_hand_pawn <= bp&&bp < e_hand_pawn) {
		//			cout << "f_handpawn + " << int(bp - f_hand_pawn) << endl;
		//		}
		//		else if (e_hand_pawn <= bp&&bp < f_hand_lance) {
		//			cout << "e_handpawn + " << int(bp - e_hand_pawn) << endl;
		//		}
		//		else if (f_hand_lance <= bp&&bp < e_hand_lance) {
		//			cout << "f_hand lance + " << int(bp - f_hand_lance) << endl;
		//		}
		//		else if (e_hand_lance <= bp&&bp < f_hand_knight) {
		//			cout << "e_handlance + " << int(bp - e_hand_lance) << endl;
		//		}
		//		else if (f_hand_knight <= bp&&bp < e_hand_knight) {
		//			cout << "f_handknight + " << int(bp - f_hand_knight) << endl;
		//		}
		//		else if (e_hand_knight <= bp&&bp < f_hand_silver) {
		//			cout << "e_hand_knight + " << int(bp - e_hand_knight) << endl;
		//		}
		//		else if (bp < e_hand_silver) {//�����̓R�������ŏ\�����H�H
		//			cout << "f_hand_silver + " << int(bp - f_hand_silver) << endl;
		//		}
		//		else if (bp < f_hand_gold) {
		//			cout << "e_hand_silver + " << int(bp - e_hand_silver) << endl;
		//		}
		//		else if (bp < e_hand_gold) {
		//			cout << "f_hand_gold + " << int(bp - f_hand_gold) << endl;
		//		}
		//		else if (bp < f_hand_bishop) {
		//			cout << "e_hand_gold + " << int(bp - e_hand_gold) << endl;
		//		}
		//		else if (bp < e_hand_bishop) {
		//			cout << "f_hand_bishop + " << int(bp - f_hand_bishop) << endl;
		//		}
		//		else if (bp < f_hand_rook) {
		//			cout << "e_hand_bishop + " << int(bp - e_hand_bishop) << endl;
		//		}
		//		else if (bp < e_hand_rook) {
		//			cout << "f_hand_rook + " << int(bp - f_hand_rook) << endl;
		//		}
		//		else if (bp < fe_hand_end) {
		//			cout << "e_hand_rook + " << int(bp - e_hand_rook) << endl;
		//		}

		//	}
		//	else {
		//		//�Տ�̋�
		//		if (f_pawn <= bp&&bp < e_pawn) {
		//			cout << "f_pawn + " << int(bp - f_pawn) << endl;
		//		}
		//		else if (e_pawn <= bp&&bp < f_lance) {
		//			cout << "e_pawn + " << int(bp - e_pawn) << endl;
		//		}
		//		else if (f_lance <= bp&&bp < e_lance) {
		//			cout << "f_lance + " << int(bp - f_lance) << endl;
		//		}
		//		else if (e_lance <= bp&&bp < f_knight) {
		//			cout << "e_lance + " << int(bp - e_lance) << endl;
		//		}
		//		else if (f_knight <= bp&&bp < e_knight) {
		//			cout << "f_knight + " << int(bp - f_knight) << endl;
		//		}
		//		else if (e_knight <= bp &&bp< f_silver) {
		//			cout << "e_knight + " << int(bp - e_knight) << endl;
		//		}
		//		else if (f_silver <= bp&&bp < e_silver) {
		//			cout << "f_silver + " << int(bp - f_silver) << endl;
		//		}
		//		else if (e_silver <= bp&&bp < f_gold) {
		//			cout << "e_silver + " << int(bp - e_silver) << endl;
		//		}
		//		else if (f_gold <= bp&&bp < e_gold) {
		//			cout << "f_gold + " << int(bp - f_gold) << endl;
		//		}
		//		else if (e_gold <= bp&&bp < f_bishop) {
		//			cout << "e_gold + " << int(bp - e_gold) << endl;
		//		}
		//		else if (bp<e_bishop) {
		//			cout << "f_bishop + " << int(bp - f_bishop) << endl;
		//		}
		//		else if (bp<f_unicorn) {
		//			cout << "e_bishop + " << int(bp - e_bishop) << endl;
		//		}
		//		else if (bp < e_unicorn) {
		//			cout << "f_unicorn + " << int(bp - f_unicorn) << endl;
		//		}
		//		else if (bp < f_rook) {
		//			cout << "e_unicorn + " << int(bp - e_unicorn) << endl;
		//		}
		//		else if (bp < e_rook) {
		//			cout << "f_rook + " << int(bp - f_rook) << endl;
		//		}
		//		else if (bp < f_dragon) {
		//			cout << "e_rook + " << int(bp - e_rook) << endl;
		//		}
		//		else if (bp < e_dragon) {
		//			cout << "f_dragon + " << int(bp - f_dragon) << endl;
		//		}
		//		else if (bp < f_king) {
		//			cout << "e_dragon + " << int(bp - e_dragon) << endl;
		//		}
		//		else if (bp < e_king) {
		//			cout << "f_king + " << int(bp - f_king) << endl;
		//		}
		//		else if (bp < fe_end2) {
		//			cout << "e_king + " << int(bp - e_king) << endl;
		//		}
		//		else {
		//			UNREACHABLE;
		//		}
		//	}
		//}//from white




	}//endof print bp
	


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
}