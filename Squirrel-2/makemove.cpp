#include "makemove.h"

using namespace std;


//�w����̈ړ��̐����֐��͋�했�ɓ��ꉻ���ċ��̐������l���Ȃ��獂�����̍H�v��}���Ă����B
template<Move_type mt>
ExtMove* make_move_PAWN(const Position& pos, const Bitboard& target, ExtMove* movelist) {

	Color US = pos.sidetomove();

	Bitboard occ_us = pos.occ_pt(US, PAWN);
	Bitboard target2;
	bool canpromotefrom=false;
	bool canpromoteto = false;

	while (occ_us.isNot()) {
		//from�̃��[�v

		Square sq = occ_us.pop();
		ASSERT(is_ok(sq));
		Piece pc = pos.piece_on(sq);
		Piece pt = piece_type(pc);
		ASSERT(pc = add_color(PAWN, US));

		target2 = target&StepEffect[c][pt][sq];
		if (mt==Cap_Propawn) {
			canpromotefrom = (SquareBB[sq] & canPromoteBB[US]).isNot();
		}
		//from��pc�͂����ň�C��bitshift���Ă�����B���̂ق�������V�t�g���Ȃ��Ă����̂ō������ł���͂��B
		int from = sq << 6;
		int pc2 = pc << 14;

		while (target2.isNot()) {

			Square to = target2.pop();
			if (mt==Cap_Propawn) {
				//���capture�Ƌ
				canpromoteto = (SquareBB[to] & canPromoteBB[US]).isNot();
				//�Ȃ��ꍇ�͕K������B�i����Ȃ����Ƃɂ�郁���b�g�͂Ȃ��j
				if (canpromoteto || canpromotefrom) {
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
template<Move_type mt>
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
		
		int obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;//7bit�����K�v�Ȃ��̂�int�ł������iuint8_t�ŏ\�����H�H�H�H�j
		target2 = target&LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[US][sqtorank(sq)];
		int from = sq << 6;
		int pc2 = pc << 14;

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

//�j�n�̈ړ��̐����֐�
template<Move_type mt>
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
		int from = sq << 6;
		int pc2 = pc << 14;
		ASSERT(pc = add_color(LANCE, US));
		ASSERT(is_ok(sq));

		target2 = target&StepEffect[c][pt][sq];

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
				//target����ړ��ł��Ȃ�bitboard�������Ă����̂���Ԃ�
				if ((sqtorank(to) != RankA) && (sqtorank(to) != RankB)&& (sqtorank(to) != RankI) && (sqtorank(to) != RankH)) {
					movelist++->move = make_move2(from, to, pc2);
				}
			}
		}


	}


}











//movelist�z��̐擪�|�C���^���󂯎���āA�ŏI�|�C���^��Ԃ��悤�ɂ���B
//template���g�����Ƃŏ�����������炵�č�����
/*
����̎��Ԃ������Ȃ��ƃ����N�G���[�ɐ���I�h�h�h�h�v
*/
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

		const Bitboard target_PAWN =
			(mt == Cap_Propawn) ? pos.occ(ENEMY) | (canPromoteBB[US] & ~pos.occ(US)) :
			(mt == Quiet) ? (~pos.occ_all()&ALLBB)&~canPromoteBB[US];



	}
	else {
		const Bitboard target_drop = ~pos.occ_all()&ALLBB;//ALLBB���}�X�N����͔̂ԊO�ɂ��P�������Ă��܂��Ă���ꏊ�����邩��

	}


}