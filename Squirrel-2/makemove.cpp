#include "makemove.h"

using namespace std;


//�w����̈ړ��̐����֐��͋�했�ɓ��ꉻ���ċ��̐������l���Ȃ��獂�����̍H�v��}���Ă����B
/*==========================================================================================
���������Ε��̗��������߂�̂͏c�^bitboard�ł���΃r�b�g�V�t�g�łł���񂾂����I
��Ŏ����B
============================================================================================*/
template<Move_type mt>
ExtMove* make_move_PAWN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	Bitboard target2;
	//bool canpromotefrom=false;
	bool canpromoteto = false;

	//���̏�����b[0]��b[1]�ŕ��񉻂��������I�I�I�I
	while (occ_us.isNot()) {
		//from�̃��[�v

		Square sq = occ_us.pop();
		ASSERT(is_ok(sq));
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pc = add_color(PAWN, US));

		target2 = target&StepEffect[US][pt][sq];
		
		//from��pc�͂����ň�C��bitshift���Ă�����B���̂ق�������V�t�g���Ȃ��Ă����̂ō������ł���͂��B
		int from = sq << 7;
		int pc2 = pc << 17;

		while (target2.isNot()) {

			Square to = target2.pop();
			if (mt==Cap_Propawn) {

				//���capture�Ƌ(�����ł܂��Ȃ�邩�ǂ������肷��̂͂��Ȃ薳�ʂ��Ǝv��)
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

				//�Ȃ��ꍇ�͕K������B�i����Ȃ����Ƃɂ�郁���b�g�͂Ȃ��j
				if (canpromoteto) {
					movelist++->move = make_movepromote2(from, to, pc2);
				}
				else {
					//rank A I�ւ̂Ȃ炸�̈ړ��͋�����Ȃ�
					if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
						movelist++->move = make_move2(from, to, pc2);
					}
				}

			}
			else {
				//rank A I�ւ̂Ȃ炸�̈ړ��͋�����Ȃ�
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}




//���Ԃ̈ړ��̐����֐�
//����̏ꍇ�ƂȂ炸�̏ꍇ�Ŋ֐��𕪂�������������������Ȃ��B
ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, LANCE);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;
	//bool canpromotefrom = false;//�����X�͑O�ɂ����i�߂Ȃ��̂�from�Ő��̔��肷��K�v�͂Ȃ�
	bool canpromoteto = false;
	
	while (occ_us.isNot()) {

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		//Piece pt = piece_type(pc);
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));
		
		int obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������iuint8_t�ŏ\�����H�H�H�H�j
		target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int from = sq << 7;
		int pc2 = pc << 17;

		while (target2.isNot())
		{
			Square to = target2.pop();
			ASSERT(is_ok(to));
			canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

			if (canpromoteto) {
				//�Ȃ��Ȃ琬��B
				movelist++->move = make_movepromote2(from, to, pc2);
				//���łɂ�����3�i�ځA���Ԃɂ�����V�Ԗڂɂ͂Ȃ炸�ňړ����邱�Ƃ������B�i����Ȃ����Ƃɂ�郁���b�g������j
				if (sqtorank(to) == Cango) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
			else {
				//����Ȃ�
				//rank A I�ւ̂Ȃ炸�̈ړ��͋�����Ȃ�
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}

//�j�n�̈ړ��̐����֐� ��������͒x��
ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, KNIGHT);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;//�Ȃ��ꏊ�ł����Ă��R�i�ڂւ̈ړ��̂Ȃ炸�ł���΋���
	bool canpromoteto = false;//�O�ɂ����i�߂Ȃ��̂�to�����ł���

	while (occ_us.isNot())
	{
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));

		target2 = target&StepEffect[US][pt][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			ASSERT(is_ok(to));
			canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();

			if (canpromoteto) {
				//�Ȃ��Ȃ琬��B
				movelist++->move = make_movepromote2(from, to, pc2);
				//���łɂ�����3�i�ځA���Ԃɂ�����V�Ԗڂɂ͂Ȃ炸�ňړ����邱�Ƃ������B�i����Ȃ����Ƃɂ�郁���b�g������j
				if (sqtorank(to) == Cango) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
			else {
				//����Ȃ�
				//rank AB IH�ւ̂Ȃ炸�̈ړ��͋�����Ȃ���������������ƂȂ�Ƃ�����Ȃ���
				//target����ړ��ł��Ȃ�bitboard�������Ă����̂���Ԃ��i����ł͂��߂��j
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankB)&& (sqtorank(to) != RankI) && (sqtorank(to) != RankH)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}


	}

	return movelist;
}


ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, SILVER);
	Bitboard target2;
	bool canpromote= false;

	while (occ_us.isNot())
	{

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		ASSERT(piece_type(pos.piece_on(sq)) == SILVER);
		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}
		target2 = target&StepEffect[US][SILVER][sq];
		int from = sq << 7;
		int pc2 = pc << 17;
		while (target2.isNot()) {

			Square to = target2.pop();
			if (canpromote == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromote = true; }
			}
			if (canpromote) { movelist++->move = make_movepromote2(from, to, pc2);}
			movelist++->move = make_move2(from, to, pc2);//�Ȃ�Ȃ��w������������Ă���
				
		}
	}
	return movelist;
}

ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, BISHOP);
	Bitboard target2;
	Bitboard effect;
	bool canpromote = false;

	while (occ_us.isNot()) {


		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(BISHOP, US));

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}

		int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
	//	cout << effect << endl;
		target2 = target&effect;
		
		while (target2.isNot()) {
			Square to = target2.pop();

			if (canpromote == false)
				if((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromote = true; }

			//�����Ƃ��͂���Ȃ����点��
			if (canpromote) {
				movelist++->move = make_movepromote2(from, to, pc2);
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}

		}
	}
	return movelist;
}



ExtMove* make_move_UNICORN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, UNICORN);
	Bitboard target2;
	Bitboard effect;
	

	while (occ_us.isNot()) {


		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(UNICORN, US));


		int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)]|StepEffect[US][KING][sq];
		//cout << effect << endl;
		target2 = target&effect;

		while (target2.isNot()) {
			Square to = target2.pop();

			
				movelist++->move = make_move2(from, to, pc2);

		}
	}
	return movelist;
}

ExtMove* make_move_ROOK(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, ROOK);
	Bitboard target2;
	Bitboard effect;
	bool canpromote = false;

	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pt == ROOK);

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromote = true;
		}
		int from = sq << 7;
		int pc2 = pc << 17;

		int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		//cout << effect << endl;
		target2 = target&effect;
		while (target2.isNot()) {

			Square to = target2.pop();

			if (canpromote == false)
				if((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromote = true; }

			if (canpromote) {
				//�Ȃ��ꍇ�͕K������
				movelist++->move = make_movepromote2(from, to, pc2);
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}
		}
	}
	return movelist;
}


ExtMove* make_move_DRAGON(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, DRAGON);
	Bitboard target2;
	Bitboard effect;
	//bool canpromote = false;

	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pt == DRAGON);

		
		int from = sq << 7;
		int pc2 = pc << 17;

		int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[US][KING][sq];
		target2 = target&effect;
		while (target2.isNot()) {

			Square to = target2.pop();

			
				movelist++->move = make_move2(from, to, pc2);
		}
	}
	return movelist;
}


//GOLD�����̋�̈ړ��̐���
ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	
	//������͐���Ȃ�


	//GOLD�����̓z�͂ЂƂ�occ_pt�ɂ܂Ƃ߂Ă�����������������
	Bitboard occ_us = pos.occ_pt(US, GOLD)|pos.occ_pt(US,PRO_PAWN)|pos.occ_pt(US,PRO_LANCE)|pos.occ_pt(US,PRO_NIGHT)|pos.occ_pt(US,PRO_SILVER);
	Bitboard target2;
	//Bitboard effect;


	//���̏�����b[0]��b[1]�ŕ��񉻂��������I�I�I�I
	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		
		int from = sq << 7;
		int pc2 = pc << 17;
		target2 = target&StepEffect[US][GOLD][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			movelist++->move = make_move2(from, to, pc2);
		}

	}
	return movelist;
}

//���̎w���萶���i���E��͐������Ă��܂�Ȃ��悤�ɂ���j
ExtMove* make_move_KING(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	Color ENEMY = opposite(US);
	//������͐���Ȃ�


	//GOLD�����̓z�͂ЂƂ�occ_pt�ɂ܂Ƃ߂Ă�����������������
	Bitboard occ_us = pos.occ_pt(US, KING);
	Bitboard target2;
	//Bitboard effect;


	//���̏�����b[0]��b[1]�ŕ��񉻂��������I�I�I�I
	while (occ_us.isNot()) {
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);

		int from = sq << 7;
		int pc2 = pc << 17;
		target2 = target&StepEffect[US][KING][sq];

		while (target2.isNot()) {
			Square to = target2.pop();
			//���̈ړ���ɑ���̌����������Ă��邱�Ƃ͂߂����ɂȂ����ƂȂ̂ł����ō��@�m�F����̂ł͂Ȃ���񂵂ɂ���
			movelist++->move = make_move2(from, to, pc2);
			
		}

	}


	return movelist;
}



//��ł�
ExtMove* make_move_DROP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	auto hands = pos.hand(US);
	Square to;
	Piece pc;
	Bitboard target2;

	if (have_hand(hands)) {

		if (num_pt(hands, PAWN)) {
			target2 = target&~CantGo_PAWNLANCE[US]&~pos.pawnbb(US);
			pc = add_color(PAWN, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				//�ł����l�߂͂߂����ɂȂ����ƂȂ̂�do_move����Ƃ��Ɋm�F����B
				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, LANCE)) {
			target2 = target&~CantGo_PAWNLANCE[US];
			pc = add_color(LANCE, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, KNIGHT)) {
			target2 = target&~CantGo_KNIGHT[US];
			pc = add_color(KNIGHT, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		//�������ʂ�for���񂷂̂ł͂Ȃ���C�Ɏw���萶�����č������ł���B
		for(Piece pt = SILVER; pt < KING; pt++) {

			if (num_pt(hands, pt)) {

				target2 = target;
				pc = add_color(pt, US);
				int pc2 = pc<<17;
				while (target2.isNot()) {
					to = target2.pop();
					ASSERT(is_ok(to));
					movelist++->move = make_drop2(to, pc2);
				}
			}
		}
	}

	return movelist;
}




//movelist�z��̐擪�|�C���^���󂯎���āA�ŏI�|�C���^��Ԃ��悤�ɂ���B
//template���g�����Ƃŏ�����������炵�č�����
/*
����̎��Ԃ������Ȃ��ƃ����N�G���[�ɐ���I�h�h�h�h�v
*/
//���̊֐���movetype�𕪊򂳂��Ă邱�Ǝ��̂����܂�悭�Ȃ���������Ȃ�
template<Move_type mt>
ExtMove * move_generation(const Position& pos, ExtMove * movelist)
{
	//EVERSION�͕ʂ̊֐���p�ӂ���
	ASSERT(mt != Eversion);

	Color US = pos.sidetomove();
	Color ENEMY = opposite(US);


	if (mt != Drop) {

		const Bitboard target_nonPAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) :
			(mt == Quiet) ? ~pos.occ_all()&ALLBB ://����ALLBB�͂���H
			(mt == Recapture) ? SquareBB[move_to(pos.state()->lastmove)] :
			ALLBB;

		//������canpromote��p�ӂ��č������{��������Ƃ��ɂ�canpromote���m�F����̂͂΂��΂������B
		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | (canPromoteBB[US] & ~pos.occ(US)) :
			(mt == Quiet) ? (~pos.occ_all()&ALLBB)&~canPromoteBB[US]:
			target_nonPAWN;

		//�����̕��ԏ��Ԃ��l���������������H�i���Ƃ�ordering����̂ł����܂ł���K�v�͂Ȃ����j
		movelist = make_move_PAWN<mt>(pos, target_PAWN, movelist);
		movelist = make_move_LANCE(pos, target_nonPAWN, movelist);
		movelist = make_move_KNIGHT(pos, target_nonPAWN, movelist);
		movelist = make_move_SILVER(pos, target_nonPAWN, movelist);
		movelist = make_move_BISHOP(pos, target_nonPAWN, movelist);
		movelist = make_move_ROOK(pos, target_nonPAWN, movelist);
		movelist = make_move_ASGOLD(pos, target_nonPAWN, movelist);
		movelist = make_move_UNICORN(pos, target_nonPAWN, movelist);
		movelist = make_move_DRAGON(pos, target_nonPAWN, movelist);
		movelist = make_move_KING(pos, target_nonPAWN, movelist);
	}
	else {
		const Bitboard target_drop = ~pos.occ_all()&ALLBB;//ALLBB���}�X�N����͔̂ԊO�ɂ��P�������Ă��܂��Ă���ꏊ�����邩��
		movelist = make_move_DROP(pos, target_drop, movelist);
	}

	return movelist;
}

//����������Ă����̗����̒����Ă��Ȃ��Ƃ���ɓ�����B
//��肭�����Ă��邩�ǂ����͖����m�F���鍡���͂��������̂�(-_-)zzz
ExtMove * move_eversion(const Position& pos, ExtMove * movelist) {

	ASSERT(pos.is_incheck());
	ASSERT(pos.state()->checker.isNot());
	/*����������Ă��Ă��������B
		�Q�d����̏ꍇ�͓����邵������
		pin���܂͓������Ă͂����Ȃ��i�R����islegal�Ŋm�F����j
		�g�r�����̊ԂɊ����ē��点��B
		�����������瓦����B
	*/
	/*
	�������@
	checkers���牤��������Ă����̌����̏ꏊ�������Ă���bitboard�̍쐬������
	checkers�̃r�b�g���𐔂���
	
	�����������̍쐬������
	�Q�d����̏ꍇ�͉���������肵��������Ȃ����߂����ŏI���B

	��d����̏ꍇ�͂��������
	����������Ă��������Abetween�Ɋ����ē���w����𐶐�����
	*/

	int num_checker = 0;
	Square ksq = pos.ksq(pos.sidetomove());
	Square esq;
	Color ENEMY = opposite(pos.sidetomove());
	Bitboard checkers = pos.state()->checker;
	Bitboard enemy_effected = ZeroBB;

	int from2 = ksq << 7;
	int king2 = add_color(KING, pos.sidetomove())<<17;


	while (checkers.isNot()) {
		esq = checkers.pop();


		//���肪�������Ă��Ȃ��̂�incheck�ɂȂ��Ă��܂��Ă���
		if (pos.piece_on(esq) == NO_PIECE) { cout << pos << endl; ASSERT(0); };
		if(piece_color(pos.piece_on(esq)) != ENEMY) { cout << pos << endl; ASSERT(0); }

		++num_checker;
		enemy_effected |= effectBB(pos, piece_type(pos.piece_on(esq)), ENEMY, esq);

	}

	ASSERT(num_checker);

	//���̓�����
	Bitboard cankingmove= step_effect(BLACK,KING, ksq)&~pos.occ(pos.sidetomove())&~enemy_effected;
	//�ʂ��ړ�������Ɍ��������邩�ǂ�����islegal�Œ��ׂ�
	while (cankingmove.isNot()) {
		Square to = cankingmove.pop();
		movelist++->move = make_move2(from2, to, king2);
	}
	
	//�Q�d����͓����邵������
	if (num_checker > 1) {
		return movelist;
	}

	//��͉���������Ă���w�������邩����Ɋ����ē��邩betweenBB���J��Ԃō���Ă����Ă悩����..
	Bitboard target_drop = BetweenBB[ksq][esq];
	Bitboard target = target_drop | SquareBB[esq];
	movelist = make_move_PAWN<Cap_Propawn>(pos, target, movelist);
	movelist = make_move_PAWN<Quiet>(pos, target, movelist);
	movelist = make_move_LANCE(pos, target, movelist);
	movelist = make_move_KNIGHT(pos, target, movelist);
	movelist = make_move_SILVER(pos, target, movelist);
	movelist = make_move_BISHOP(pos, target, movelist);
	movelist = make_move_ROOK(pos, target, movelist);
	movelist = make_move_ASGOLD(pos, target, movelist);
	movelist = make_move_UNICORN(pos, target, movelist);
	movelist = make_move_DRAGON(pos, target, movelist);
	//movelist = make_move_KING(pos, target, movelist);

	//drop��łĂ�̂͋�Ƌ�̊Ԃ����I�I
	movelist = make_move_DROP(pos, target_drop, movelist);

	return movelist;

}

ExtMove * move_recapture(const Position & pos, ExtMove * movelist, Square recapsq)
{
	const Bitboard target = SquareBB[recapsq];

	movelist = make_move_PAWN<Cap_Propawn>(pos, target, movelist);//recapture�Ȃ̂�cappropawn�ł����ƍl������
	movelist = make_move_LANCE(pos, target, movelist);
	movelist = make_move_KNIGHT(pos, target, movelist);
	movelist = make_move_SILVER(pos, target, movelist);
	movelist = make_move_BISHOP(pos, target, movelist);
	movelist = make_move_ROOK(pos, target, movelist);
	movelist = make_move_ASGOLD(pos, target, movelist);
	movelist = make_move_UNICORN(pos, target, movelist);
	movelist = make_move_DRAGON(pos, target, movelist);
	movelist = make_move_KING(pos, target, movelist);

	return movelist;
}



ExtMove * test_move_generation(const Position & pos, ExtMove * movelist)
{
	if (pos.is_incheck()) {
		movelist = move_eversion(pos, movelist);
	}
	else {
		movelist = move_generation<Cap_Propawn>(pos, movelist);
		movelist = move_generation<Quiet>(pos, movelist);
		movelist = move_generation<Drop>(pos, movelist);
	}
	return movelist;
}




//ExtMove * move_generation<Cap_Propawn>(const Position& pos, ExtMove * movelist);