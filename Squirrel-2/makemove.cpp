#include "makemove.h"

using namespace std;

#define fastdrop
#define shiftpawn

#ifdef shiftpawn
template<Color US>
ExtMove* make_move_PAWN_bitshift(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	//�c�^�ł���Ε��̌���������}�X��bitshift�œ�����
	Bitboard target2=(US==BLACK? (occ_us>>1):(occ_us<<1))&target;

	const Rank rank1_teban = (US == BLACK ? RankA : RankI);
	const Direction south_teban = (US == BLACK ? DOWN : UP);//�����̂���}�X���炻�̌����̌��������߂邽�߂Ɏg���B
	const int pc2 = add_color(PAWN, US) << 17;
	bool canpromoteto = false;

	while (target2.isNot())
	{
		Square to = target2.pop();
		Square from = to + south_teban;
		int from2 = from << 7;

		//if��mt�܂ŕ�����͖̂��ʂ��������itarget�ł��łɍl������Ă���̂Łj(�L��֥�M)

		canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
		if (canpromoteto) {
			movelist++->move = make_movepromote2(from2, to, pc2);
		}
		else {
			movelist++->move = make_move2(from2, to, pc2);
		}
	}
	return movelist;
}

ExtMove* make_move_PAWN_bitshift(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	if (pos.sidetomove() == BLACK) {
		return make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
	}
	else {
		return make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
	}
}
#else
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
		ASSERT(pc == add_color(PAWN, US));

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
					//rank A I�ւ̂Ȃ炸�̈ړ��͋�����Ȃ�(���̏����͕K�v�Ȃ�)target�ł����target�����Ă��Ȃ�
					/*if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
						movelist++->move = make_move2(from, to, pc2);
					}*/
					movelist++->move = make_move2(from, to, pc2);
				}

			}
			else if(mt==Quiet) {
				//rank A I�ւ̂Ȃ炸�̈ړ��͋�����Ȃ��i���̏����͕K�v�Ȃ� target�ł����target�����Ă��Ȃ��j
				/*if ((sqtorank(to) != RankA) && (sqtorank(to) != RankI)) {
					movelist++->move = make_move2(from, to, pc2);
				}*/
				movelist++->move = make_move2(from, to, pc2);
			}
			else if (mt == Eversion) {
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
				if (canpromoteto) {
					movelist++->move = make_movepromote2(from, to, pc2);
				}
				else {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}
	}
	return movelist;
}
#endif


//���Ԃ̈ړ��̐����֐�
//����̏ꍇ�ƂȂ炸�̏ꍇ�Ŋ֐��𕪂�������������������Ȃ��B
template<Color US>
ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, LANCE);
	Bitboard target2;
	Rank Cango = (US == BLACK) ? RankC : RankG;
	//bool canpromotefrom = false;//�����X�͑O�ɂ����i�߂Ȃ��̂�from�Ő��̔��肷��K�v�͂Ȃ�
	bool canpromoteto = false;
	
	while (occ_us.isNot()) {

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		//Piece pt = piece_type(pc);
		ASSERT(pc == add_color(LANCE, US));
		ASSERT(is_ok(sq));
		
		//int obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������iuint8_t�ŏ\�����H�H�H�H�j
		//target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		target2 = target&LanceEffect[US][sq][obstacle_tate];
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

ExtMove* make_move_LANCE(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return 	pos.sidetomove() == BLACK ?  make_move_LANCE<BLACK>(pos, target, movelist) : make_move_LANCE<WHITE>(pos, target, movelist);
}

//�j�n�̈ړ��̐����֐� ��������͒x��
template<Color US>
ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());

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
		ASSERT(pc == add_color(KNIGHT, US));
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

ExtMove* make_move_KNIGHT(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_KNIGHT<BLACK>(pos, target, movelist) : make_move_KNIGHT<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, SILVER);
	Bitboard target2;
	bool canpromotefrom= false;

	while (occ_us.isNot())
	{
		//�K��������canpromote��false�ɖ߂��Ă����I
		canpromotefrom = false;
		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		ASSERT(piece_type(pos.piece_on(sq)) == SILVER);
		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}
		target2 = target&StepEffect[US][SILVER][sq];
		int from = sq << 7;
		int pc2 = pc << 17;
		while (target2.isNot()) {
			bool canpromoteto = false;
			Square to = target2.pop();
			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}
			if (canpromotefrom||canpromoteto) { movelist++->move = make_movepromote2(from, to, pc2);}
			movelist++->move = make_move2(from, to, pc2);//�Ȃ�Ȃ��w������������Ă���
				
		}
	}
	return movelist;
}


ExtMove* make_move_SILVER(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_SILVER<BLACK>(pos, target, movelist) : make_move_SILVER<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, BISHOP);
	Bitboard target2;
	Bitboard effect;
	bool canpromotefrom = false;

	while (occ_us.isNot()) {
		//������false�ɖ߂��Ȃ���΂Ȃ�Ȃ��I
		canpromotefrom = false;

		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(BISHOP, US));

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}

		/*int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;*/
		int obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
	//	cout << effect << endl;
		target2 = target&effect;
		
		while (target2.isNot()) {
			bool canpromoteto = false;
			Square to = target2.pop();

			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}
			//�����Ƃ��͂���Ȃ����点��
			if (canpromotefrom==true||canpromoteto==true) {
				movelist++->move = make_movepromote2(from, to, pc2);
				//�����ɕs���������Ă��������������̂Ŋw�K���͐������Ă݂�����͂肻��ȋǖʊw�K����K�v���Ȃ��̂Ő������
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}

		}
	}
	return movelist;
}


ExtMove* make_move_BISHOP(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_BISHOP<BLACK>(pos, target, movelist) : make_move_BISHOP<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_UNICORN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, UNICORN);
	Bitboard target2;
	Bitboard effect;
	

	while (occ_us.isNot()) {


		Square sq = occ_us.pop();
		Piece pc = pos.piece_on(sq);
		int from = sq << 7;
		int pc2 = pc << 17;
		ASSERT(pc == add_color(UNICORN, US));


		//int obstacle_plus45 = (pos.occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		//int obstacle_Minus45 = (pos.occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		int obstacle_plus45 = (pos.ret_occ_256().b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (pos.ret_occ_256().b64(3) >> occ256_shift_table_m45[sq])&effectmask;
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


ExtMove* make_move_UNICORN(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_UNICORN<BLACK>(pos, target, movelist) : make_move_UNICORN<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_ROOK(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

	Bitboard occ_us = pos.occ_pt(US, ROOK);
	Bitboard target2;
	Bitboard effect;
	bool canpromotefrom = false;
	

	while (occ_us.isNot()) {
		//������false�ɖ߂��Ȃ���΂Ȃ�Ȃ��I
		canpromotefrom = false;
		

		const Square sq = occ_us.pop();
		const Piece pc = pos.piece_on(sq);
		const Piece pt = piece_type(pc);
		ASSERT(pt == ROOK);

		if ((SquareBB[sq] & canPromoteBB[US]).isNot()) {
			canpromotefrom = true;
		}
		int from = sq << 7;
		int pc2 = pc << 17;

		/*
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		
		*/

		/*int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;*/
		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		//cout << effect << endl;
		target2 = target&effect;
		while (target2.isNot()) {
			bool canpromoteto = false;

			Square to = target2.pop();

			if (canpromotefrom == false) {
				if ((SquareBB[to] & canPromoteBB[US]).isNot()) { canpromoteto = true; }
			}

			if (canpromotefrom==true||canpromoteto==true) {
				//�Ȃ��ꍇ�͕K������(�����ɐ��炸���܂܂�Ă��邱�Ƃ��������̂Ŋw�K���͐��炸����������)
				ASSERT((SquareBB[sq] & canPromoteBB[US]).isNot() || (SquareBB[to] & canPromoteBB[US]).isNot());
				movelist++->move = make_movepromote2(from, to, pc2);
			}
			else {
				movelist++->move = make_move2(from, to, pc2);
			}
		}
	}
	return movelist;
}

ExtMove* make_move_ROOK(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_ROOK<BLACK>(pos, target, movelist) : make_move_ROOK<WHITE>(pos, target, movelist);
}

template<Color US>
ExtMove* make_move_DRAGON(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());

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

		int8_t obstacle_tate = (pos.ret_occ_256().b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (pos.ret_occ_256().b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		/*int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;*/

	/*	int8_t obstacle_tate = (pos.occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (pos.occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;*/
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[US][KING][sq];
		target2 = target&effect;
		while (target2.isNot()) {

			Square to = target2.pop();

			
			movelist++->move = make_move2(from, to, pc2);
		}
	}
	return movelist;
}


ExtMove* make_move_DRAGON(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_DRAGON<BLACK>(pos, target, movelist) : make_move_DRAGON<WHITE>(pos, target, movelist);
}

//GOLD�����̋�̈ړ��̐���
template<Color US>
ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT( US == pos.sidetomove());
	
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


ExtMove* make_move_ASGOLD(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_ASGOLD<BLACK>(pos, target, movelist) : make_move_ASGOLD<WHITE>(pos, target, movelist);
}


//���̎w���萶���i���E��͐������Ă��܂�Ȃ��悤�ɂ���j
template<Color US>
ExtMove* make_move_KING(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	ASSERT(US == pos.sidetomove());
//	Color ENEMY = opposite(US);
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


ExtMove* make_move_KING(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_KING<BLACK>(pos, target, movelist) : make_move_KING<WHITE>(pos, target, movelist);
}

#ifndef fastdrop


//��ł�
ExtMove* make_move_DROP(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	auto hands = pos.hand(US);
	Square to;
	Piece pc;
	Bitboard target2;

	if (have_hand(hands)) {

		if (num_pt(hands, PAWN)) {

			//target2 = target&~CantGo_PAWNLANCE[US]&~pos.pawnbb(US);
			// _mm_andnot_si128�͂P���߁H�H
			target2 = andnot(andnot(target, CantGo_PAWNLANCE[US]), pos.pawnbb(US));
			pc = add_color(PAWN, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				//�����łQ���������Ă��邱�Ƃ͂Ȃ��Ǝv������ǂ��ꉞ�m�F���Ă���
				//�����œ�����͂����ė��Ă���̂�pawnbb���������������� �i�����j
				/*if (pos.check_nihu(make_drop2(to, pc2)) == true) {
				cout << "nihu " << endl;
				cout << "target2"<<endl << target2 << endl;
				cout << make_drop2(to, pc2) << endl;
				cout << pos << endl;
				cout << "pbb black" << endl << pos.pawnbb(BLACK) << endl;
				cout << "pbb white" << endl << pos.pawnbb(WHITE) << endl;
				UNREACHABLE;
				}*/


				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, LANCE)) {
			target2 = andnot(target, CantGo_PAWNLANCE[US]);
			pc = add_color(LANCE, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		if (num_pt(hands, KNIGHT)) {
			target2 = andnot(target, CantGo_KNIGHT[US]);
			pc = add_color(KNIGHT, US);
			int pc2 = pc << 17;
			while (target2.isNot()) {
				to = target2.pop();
				ASSERT(is_ok(to));
				movelist++->move = make_drop2(to, pc2);
			}
		}

		//�������ʂ�for���񂷂̂ł͂Ȃ���C�Ɏw���萶�����č������ł���B
		for (Piece pt = SILVER; pt < KING; pt++) {

			if (num_pt(hands, pt)) {

				target2 = target;
				pc = add_color(pt, US);
				int pc2 = pc << 17;
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

#endif // !fastdrop
#ifdef fastdrop
//Apery�Q�l�̋��
template<Color US>
ExtMove* make_move_DROP_fast(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();
	auto hands = pos.hand(US);
	Square to;
	Piece pc;
	Bitboard target2;

	if (have_hand(hands)) {

		//������2���͖h����Ă��邪�������l�߂͂����ł͌��Ă��Ȃ��Bisleagal�Ō���B
		if (num_pt(hands, PAWN)) {
			pc = add_color(PAWN, US);
			int pc2 = pc << 17;
			target2 = andnot(andnot(target, CantGo_PAWNLANCE[US]), pos.pawnbb(US));
			foreachBB(target2, to, movelist++->move = make_drop2(to, pc2););
		}
		if (have_except_pawn(hands)) {
			Move Drop[6];
			int num = 0;
			if (have_pt(hands, KNIGHT)) {Drop[num++] = make_drop(SQ_ZERO, add_color(KNIGHT, US));}
			int no_knight_index = num;//�j�n���Ȃ��ꍇ
			if (have_pt(hands, LANCE)) {Drop[num++] = make_drop(SQ_ZERO, add_color(LANCE, US));}
			int no_lance_index = num;//���Ԃ��Ȃ��ꍇ
			if (have_pt(hands, SILVER)) { Drop[num++] = make_drop(SQ_ZERO, add_color(SILVER, US)); }
			if (have_pt(hands, GOLD)) { Drop[num++] = make_drop(SQ_ZERO, add_color(GOLD, US)); }
			if (have_pt(hands, BISHOP)) { Drop[num++] = make_drop(SQ_ZERO, add_color(BISHOP, US)); }
			if (have_pt(hands, ROOK)) { Drop[num++] = make_drop(SQ_ZERO, add_color(ROOK, US)); }

			if (no_lance_index == 0) {

				target2 = target;
				//���Ԃƌj�n���Ȃ��̂Ŗ����̂��Ȃ��}�X�ɑł�����𐶐�����΂���
				switch (num)
				{
					//0�ɂȂ邱�Ƃ͂��蓾�Ȃ�
				case 1:
					foreachBB(target2, to, { unroller1({movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 2:
					foreachBB(target2, to, { unroller2({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 3:
					foreachBB(target2, to, { unroller3({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 4:
					foreachBB(target2, to, { unroller4({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}
			}
			else {
				//����ȊO�̃P�[�X
				/*------------------------------------------------------
				����ȊO�̃P�[�X�ɂ�
				���ԁ@����@�Ȃ�
				�j�n�@����@�Ȃ�
				���̑��@����@�Ȃ�
				��3�̃P�[�X������B
				���Ԃ́@1�i�ڂɑłĂȂ�
				�j�n�͂P�`�Q�i�ڂɑłĂȂ��B

				�܂��͈�i�ڂւ̋�����������ԈȊO�ɑ΂��Đ���
				2�i�ڂւ̋���������j�n�ȊO�ɑ΂��Đ����B
				3�i�ڈȍ~�͂��ׂĂ̋��𐶐�����
				---------------------------------------------------------*/
				Bitboard rank1 = target& CantGo_PAWNLANCE[US];//1�i��(9�i��)
				Bitboard rank2 = target& (US==BLACK?RankBB[RankB]:RankBB[RankH]);//2�i�ځi8�i�ځj
				target2 = andnot(target, CantGo_KNIGHT[US]);//3~9
				/*cout << rank1 << endl;
				cout << rank2 << endl;
				cout << target2 << endl;*/
				switch (num-no_lance_index)
				{
				case 0:
					break;
				case 1:
					foreachBB(rank1, to, { unroller1({ movelist++->move = (Move)(Drop[i+no_lance_index] + to); }) });
					break;
				case 2:
					foreachBB(rank1, to, { unroller2({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				case 3:
					foreachBB(rank1, to, { unroller3({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				case 4:
					foreachBB(rank1, to, { unroller4({ movelist++->move = (Move)(Drop[i + no_lance_index] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}

				switch (num - no_knight_index)
				{
				case 0:
					break;
				case 1:
					foreachBB(rank2, to, { unroller1({ movelist++->move = (Move)(Drop[i+no_knight_index] + to); }) });
					break;
				case 2:
					foreachBB(rank2, to, { unroller2({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 3:
					foreachBB(rank2, to, { unroller3({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 4:
					foreachBB(rank2, to, { unroller4({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				case 5:
					foreachBB(rank2, to, { unroller5({ movelist++->move = (Move)(Drop[i + no_knight_index] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}

				switch (num)
				{
				case 1:
					foreachBB(target2, to, { unroller1({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 2:
					foreachBB(target2, to, { unroller2({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 3:
					foreachBB(target2, to, { unroller3({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 4:
					foreachBB(target2, to, { unroller4({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 5:
					foreachBB(target2, to, { unroller5({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				case 6:
					foreachBB(target2, to, { unroller6({ movelist++->move = (Move)(Drop[i] + to); }) });
					break;
				default:
					UNREACHABLE;
					break;
				}
			}
		}


	}


	return movelist;
}

ExtMove* make_move_DROP_fast(const Position& pos, const Bitboard& target, ExtMove* movelist) {
	return (pos.sidetomove() == BLACK) ? make_move_DROP_fast<BLACK>(pos, target, movelist) : make_move_DROP_fast<WHITE>(pos, target, movelist);
}

#endif
/*

int8_t obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
int8_t obstacle_yoko = (occ256.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;

*/


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

	Color US = pos.sidetomove();//������US�p�ӂ��Ă���Ă��Ƃ͂�����template���ł�����
	Color ENEMY = opposite(US);


	if (mt != Drop) {

		const Bitboard target_nonPAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) :
			(mt == Quiet) ? ~pos.occ_all() :
			(mt == Recapture) ? SquareBB[move_to(pos.state()->lastmove)] :
			ALLBB;

		//������canpromote��p�ӂ��č������{��������Ƃ��ɂ�canpromote���m�F����̂͂΂��΂������B
		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | andnot(canPromoteBB[US],pos.occ(US)) :
			(mt == Quiet) ? andnot((~pos.occ_all()),canPromoteBB[US]):
			target_nonPAWN;

		//�����̕��ԏ��Ԃ��l���������������H�i���Ƃ�ordering����̂ł����܂ł���K�v�͂Ȃ����j
		if (US == BLACK) {
			movelist = make_move_PAWN_bitshift<BLACK>(pos, target_PAWN, movelist);
			movelist = make_move_LANCE<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_KNIGHT<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_SILVER<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_BISHOP<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_ROOK<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_ASGOLD<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_UNICORN<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_DRAGON<BLACK>(pos, target_nonPAWN, movelist);
			movelist = make_move_KING<BLACK>(pos, target_nonPAWN, movelist);
		}
		else {
			movelist = make_move_PAWN_bitshift<WHITE>(pos, target_PAWN, movelist);
			movelist = make_move_LANCE<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_KNIGHT<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_SILVER<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_BISHOP<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_ROOK<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_ASGOLD<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_UNICORN<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_DRAGON<WHITE>(pos, target_nonPAWN, movelist);
			movelist = make_move_KING<WHITE>(pos, target_nonPAWN, movelist);
		}
	}
	else {
		const Bitboard target_drop = ~pos.occ_all();//ALLBB���}�X�N����͔̂ԊO�ɂ��P�������Ă��܂��Ă���ꏊ�����邩��
		//
		if (US == BLACK) {
			movelist = make_move_DROP_fast<BLACK>(pos, target_drop, movelist);
		}
		else {
			movelist = make_move_DROP_fast<WHITE>(pos, target_drop, movelist);
		}
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
	const Color US = pos.sidetomove();
	const Square ksq = pos.ksq(US);
	Square esq;
	const Color ENEMY = opposite(US);
	Bitboard checkers = pos.state()->checker;
	Bitboard enemy_effected = ZeroBB;

	int from2 = ksq << 7;
	int king2 = add_color(KING, US)<<17;


	while (checkers.isNot()) {
		esq = checkers.pop();


		//���肪�������Ă��Ȃ��̂�incheck�ɂȂ��Ă��܂��Ă���
		//if (pos.piece_on(esq) == NO_PIECE) { cout << pos << endl; ASSERT(0); };
		//if(piece_color(pos.piece_on(esq)) != ENEMY) { cout << pos << endl; ASSERT(0); }
		ASSERT(pos.piece_on(esq) != NO_PIECE);
		ASSERT(piece_color(pos.piece_on(esq)) == ENEMY);


		++num_checker;
		enemy_effected |= effectBB(pos, piece_type(pos.piece_on(esq)), ENEMY, esq);

	}

	ASSERT(num_checker);

	//���̓�����
	Bitboard cankingmove= andnot(andnot(step_effect(BLACK,KING, ksq),pos.occ(US)),enemy_effected);
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
	/*movelist = make_move_PAWN<Cap_Propawn>(pos, target, movelist);
	movelist = make_move_PAWN<Quiet>(pos, target, movelist);*/
	if (US == BLACK) {
		movelist = make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
		movelist = make_move_LANCE<BLACK>(pos, target, movelist);
		movelist = make_move_KNIGHT<BLACK>(pos, target, movelist);
		movelist = make_move_SILVER<BLACK>(pos, target, movelist);
		movelist = make_move_BISHOP<BLACK>(pos, target, movelist);
		movelist = make_move_ROOK<BLACK>(pos, target, movelist);
		movelist = make_move_ASGOLD<BLACK>(pos, target, movelist);
		movelist = make_move_UNICORN<BLACK>(pos, target, movelist);
		movelist = make_move_DRAGON<BLACK>(pos, target, movelist);
		//movelist = make_move_KING(pos, target, movelist);

		//drop��łĂ�̂͋�Ƌ�̊Ԃ����I�I
		//movelist = make_move_DROP(pos, target_drop, movelist);
		movelist = make_move_DROP_fast<BLACK>(pos, target_drop, movelist);
	}
	else {
		movelist = make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
		movelist = make_move_LANCE<WHITE>(pos, target, movelist);
		movelist = make_move_KNIGHT<WHITE>(pos, target, movelist);
		movelist = make_move_SILVER<WHITE>(pos, target, movelist);
		movelist = make_move_BISHOP<WHITE>(pos, target, movelist);
		movelist = make_move_ROOK<WHITE>(pos, target, movelist);
		movelist = make_move_ASGOLD<WHITE>(pos, target, movelist);
		movelist = make_move_UNICORN<WHITE>(pos, target, movelist);
		movelist = make_move_DRAGON<WHITE>(pos, target, movelist);
		//movelist = make_move_KING(pos, target, movelist);

		//drop��łĂ�̂͋�Ƌ�̊Ԃ����I�I
		//movelist = make_move_DROP(pos, target_drop, movelist);
		movelist = make_move_DROP_fast<WHITE>(pos, target_drop, movelist);

	}
	return movelist;

}

ExtMove * move_recapture(const Position & pos, ExtMove * movelist, Square recapsq)
{
	const Bitboard target = SquareBB[recapsq];

	const Color US = pos.sidetomove();
	if (US == BLACK) {
		movelist = make_move_PAWN_bitshift<BLACK>(pos, target, movelist);
		movelist = make_move_LANCE<BLACK>(pos, target, movelist);
		movelist = make_move_KNIGHT<BLACK>(pos, target, movelist);
		movelist = make_move_SILVER<BLACK>(pos, target, movelist);
		movelist = make_move_BISHOP<BLACK>(pos, target, movelist);
		movelist = make_move_ROOK<BLACK>(pos, target, movelist);
		movelist = make_move_ASGOLD<BLACK>(pos, target, movelist);
		movelist = make_move_UNICORN<BLACK>(pos, target, movelist);
		movelist = make_move_DRAGON<BLACK>(pos, target, movelist);
		movelist = make_move_KING<BLACK>(pos, target, movelist);//����ő���̌����̂���ꏊ�Ɉړ����Ă��܂����Ƃ����邪�����islegal�ł���ׂĂ���͂�
	}
	else {
		movelist = make_move_PAWN_bitshift<WHITE>(pos, target, movelist);
		movelist = make_move_LANCE<WHITE>(pos, target, movelist);
		movelist = make_move_KNIGHT<WHITE>(pos, target, movelist);
		movelist = make_move_SILVER<WHITE>(pos, target, movelist);
		movelist = make_move_BISHOP<WHITE>(pos, target, movelist);
		movelist = make_move_ROOK<WHITE>(pos, target, movelist);
		movelist = make_move_ASGOLD<WHITE>(pos, target, movelist);
		movelist = make_move_UNICORN<WHITE>(pos, target, movelist);
		movelist = make_move_DRAGON<WHITE>(pos, target, movelist);
		movelist = make_move_KING<WHITE>(pos, target, movelist);//����ő���̌����̂���ꏊ�Ɉړ����Ă��܂����Ƃ����邪�����islegal�ł���ׂĂ���͂�
	}
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
//
//ExtMove * test_drop_fast(const Position & pos, ExtMove * movelist)
//{
//	return nullptr;
//}


/*
Cap_Propawn,//�����͔�Ԃ̐�������Ƃ��A���̐����Ȃ��Ƃ�lance night�̃i�����܂߂�Ƃ� �F�X�l����K�v������
Quiet,
Eversion,
Recapture,
Drop,
*/

//ExtMove * move_generation<Cap_Propawn>(const Position& pos, ExtMove * movelist);
template ExtMove* move_generation<Cap_Propawn>(const Position& pos, ExtMove* movelist);
template ExtMove* move_generation<Quiet>(const Position& pos, ExtMove* movelist);
template ExtMove* move_generation<Drop>(const Position& pos, ExtMove* movelist);



#if 1
template<Piece pt>
ExtMove* make_checkdrop(const Position& pos,const Bitboard target, ExtMove * movelist) {

	const Color us = pos.sidetomove();
	auto hands = pos.hand(us);
	int pc2 = add_color(pt,us) << 17;
	Bitboard target2 = target;
	if (num_pt(hands, pt) != 0) {
		while (target2.isNot()) {
			Square to = target2.pop();
			movelist++->move = make_drop2(to, pc2);
		}
	}
	return movelist;
}




/*
�Î~�T���Ŏg��quiet�ȉ��萶��

�����̏ꍇ
�P�����̔�ь������Ղ��Ă������ǂ���
�Q����ł���͈͂ɋ��ł�
�R����ł���͈͂ɋ���ړ�������
��3�̉���̕��@������

�����łP�ƂR�ɂ͏d��������̂łP��3���������Ȃ��悤�ɂ��Ȃ���΂Ȃ�Ȃ��B
��������ɐ������Ă����ׂ����Ǝv���̂�3�����ɐ������ׂ��H�H



�e�X�g�ǖ�
2�d����\�@position sfen lnsgkgsnl/1r5b1/pp2G1ppp/9/9/4RB3/PPPPPPPPP/9/LNSGK1SNL b P3p 1 OK
position sfen lnsgkgsnl/1r5b1/pp2GLppp/9/9/4RB3/PPPPP1PP1/9/LNSGK1SN1 b 3P3p 1 OK

drop�͏������肵���ق���������������Ȃ�
*/


template<Color us,bool is_eksq_infield>
ExtMove * move_generation_quietcheck__(const Position & pos, ExtMove * movelist) {

	ASSERT(!pos.is_incheck());
	ASSERT(us == pos.sidetomove());
	
	//const Color us = pos.sidetomove();

	const Color enemy = opposite(us);
	const Square eksq = pos.ksq(opposite(us));
	//const bool is_eksq_infield=(SquareBB[eksq] & canPromoteBB[us]).isNot();//����ʂ��G�w�ɂ��邩�ǂ���
	ASSERT(is_eksq_infield == (SquareBB[eksq] & canPromoteBB[us]).isNot());
	Bitboard apart_check_brocker = pos.state()->pinner[us];//����pinner_us��OK�̂͂��i���O�̕t�������܂��������j����𓮂����Ɗ֐߉���ɂȂ�

	//�P�����̔�ь������Ղ��Ă������ǂ���
	while (apart_check_brocker.isNot())
	{
		const Square from = apart_check_brocker.pop();
		const Piece pt = piece_type( pos.piece_on(from));
		const int from2 = from << 7;
		const int pc2 = pos.piece_on(from) << 17;
		if (pt == KING) { continue; }//king�������̂͂�΂����Ȃ̂ł�߂Ƃ�
		ASSERT(piece_color(pos.piece_on(from)) == us);
		//�ړ���ifrom��eksq�̒�����i���̒�����ɂ͔�уS�}������j���Ƌ󂫉���ɂȂ�Ȃ��@���ډ���͕ʂōl����̂ŏȂ��j

		Bitboard target;
		if (pt == PAWN) {
			target = andnot(StepEffect[us][PAWN][from], LineBB[from][eksq] | effectBB(pos.ret_occ_256(), pt, enemy, eksq)|pos.occ_all());
			target = andnot(target, canPromoteBB[us]);
		}
		else {
			target = andnot(effectBB(pos.ret_occ_256(), pt, us, from), LineBB[from][eksq] | effectBB(pos.ret_occ_256(), pt, enemy, eksq) | pos.occ_all());
		}
		while (target.isNot())
		{
			Square to = target.pop();
			movelist++->move = make_move2(from2, to, pc2);
		}

	}
	//�Q����ł���͈͂ɋ��ł�
	//�R����ł���͈͂ɋ���ړ�������

	//is_ksqIncheck��template���ł����...
	Bitboard target;

	//��
	target = andnot(StepEffect[enemy][PAWN][eksq], pos.occ_all() | canPromoteBB[us]);
	movelist=make_move_PAWN_bitshift(pos, target, movelist);

	//��
	target = andnot(lance_effect(pos.ret_occ_256(), enemy, eksq), pos.occ_all());
	movelist = make_checkdrop<LANCE>(pos, target, movelist);
	if (is_eksq_infield) { target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all()); }//�����ƍ��ĂȂ���,�܂���̂�...
	movelist = make_move_LANCE(pos, target, movelist);

	//�j�n
	target = andnot(StepEffect[enemy][KNIGHT][eksq], pos.occ_all());
	movelist= make_checkdrop<KNIGHT>(pos, target, movelist);
	if (is_eksq_infield) {target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all());}//���������I�Ȃ񂾂��d���Ȃ�
	movelist = make_move_KNIGHT(pos, target, movelist);

	//��
	target = andnot(StepEffect[enemy][SILVER][eksq], pos.occ_all());
	movelist = make_checkdrop<SILVER>(pos, target, movelist);
	if (is_eksq_infield) { target = andnot(target | (StepEffect[enemy][GOLD][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_SILVER(pos, target, movelist);

	//��
	target= andnot(StepEffect[enemy][GOLD][eksq], pos.occ_all());
	movelist = make_move_ASGOLD(pos, target, movelist);
	movelist=make_checkdrop<GOLD>(pos,target, movelist);

	//���
	target = andnot(rook_effect(pos.ret_occ_256(), eksq), pos.occ_all());
	movelist = make_checkdrop<ROOK>(pos,target, movelist);
	if (is_eksq_infield) {target = andnot(target|(StepEffect[us][KING][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_ROOK(pos, target, movelist);

	//�p
	target = andnot(bishop_effect(pos.ret_occ_256(), eksq), pos.occ_all()); 
	movelist = make_checkdrop<BISHOP>(pos,target, movelist);
	if (is_eksq_infield) {target = andnot(target| (StepEffect[us][KING][eksq]& canPromoteBB[us]), pos.occ_all());}
	movelist = make_move_BISHOP(pos, target, movelist);
	

	//��
	target = andnot(rook_effect(pos.ret_occ_256(), eksq) | StepEffect[us][KING][eksq], pos.occ_all());
	movelist = make_move_DRAGON(pos, target, movelist);
	//���j�R�[��
	target = andnot(bishop_effect(pos.ret_occ_256(), eksq) | StepEffect[us][KING][eksq], pos.occ_all());
	movelist = make_move_UNICORN(pos, target, movelist);

	return movelist;
}


template<Color us>
ExtMove * move_generation_quietcheck_(const Position & pos, ExtMove * movelist) {
	const Square eksq = pos.ksq(opposite(us));
	if ((SquareBB[eksq] & canPromoteBB[us]).isNot()) { return move_generation_quietcheck__<us, true>(pos, movelist); }
	else { return move_generation_quietcheck__<us, false>(pos, movelist); }
}


ExtMove * move_generation_quietcheck(const Position & pos, ExtMove * movelist) {
	if (pos.sidetomove() == BLACK) { return move_generation_quietcheck_<BLACK>(pos, movelist); }
	else { return move_generation_quietcheck_<WHITE>(pos, movelist); }
}



ExtMove * test_quietcheck(const Position & pos, ExtMove * movelist) {

	//const Color us = pos.sidetomove();
	//const Square eksq = pos.ksq(opposite(us));
	//const bool is_eksq_infield=(SquareBB[eksq] & canPromoteBB[us]).isNot();//����ʂ��G�w�ɂ��邩�ǂ���

	movelist = move_generation_quietcheck(pos, movelist);

	return movelist;
}


#endif