#include "position.h"

/*
���l�ߊ֐�

��ł�����̉����l�܂���悤�ȍ�����𔭌��ł���΂�����̂���

---------------�ƂуS�}�ɂ�闣�ꂽ����
�ߐډ���ŋl�ނ̂Ȃ痣�ꂽ����ł��萔�͂����邪�l�ށB�t�͐^�ł͂Ȃ��B
�ƂуS�}�ɂ�闣�ꂽ������l����̂͑���̎����S�}���Ȃ��ꍇ�݂̂��l����B

---------------���
(��̂��Ȃ��ꏊ)����(ksq�ɑ���̋��ɑł��Ĕ�����������̂���ꏊ)����(�ߐډ���ł���΂ق��̋�Ɏx�����Ă���Ƃ���)�݂̂��l����B
;;;;;;
ksq��occ�����菜��occ2
�ʂ̓�����ɂق��̋�̌���������B
�܂��͂ق��̋�ł��̋��ߊl�ł��Ȃ��B
���̏ꍇ�͋l�݁B

�l����̂�
�@���(����ŋl�܂Ȃ���΋���p�̎΂߂ƌj�̋���݂̂��l����)
�A�p(����Őς܂Ȃ���΋���̏c���ƍ��Ԃƌj�̋���݂̂��l����B�i��Ԃ͎����Ă��Ȃ��������Ƃ���Ŋm�F����Ă���B�j)
�B���i�l�܂Ȃ���΋�̎΂ߌ��,�j�n�A���ԁj
4��
5����
6�j�n

���̏��ԁB

---------------��̈ړ�
����ł���͈͂ɂ���������������邱�Ƃ��ł���}�X�������̌����̂���}�X�ɓ��������Ƃ��A
���̋���ق��̋�Ŏ��Ȃ��܂��͉��̓�����ɂق��̖����̌���������΋l�݁B

����͐�ɍl������ɂ���Ď��̋�̈ړ����y�ł���Ƃ����������̂͂Ȃ��Ǝv���B
��̈ړ��ɂ���ĔՏ�̌������ς���Ă��܂��̂�

---------------�Ԑډ���@�󂫉���
����̎�������݂��Ȃ��Ƃ��ɍl����
���ł����}�X�Ɖ��̂�����+���ł������Ɉړ��ł��鑊��̋����̂ł͋l�܂Ȃ��B
*/
bool Position::mate1ply()
{
	const Color us = sidetomove();
	const Color enemy = opposite(sidetomove());
	const Square eksq = ksq(enemy);//�l�܂���������ʂ̈ʒu�B
	const Hand h = hand(sidetomove());//��ԑ��̎�����B

	const Occ_256 occ_without_eksq = occ256^SquareBB256[eksq];//����̉��̏ꏊ��������occupied
	Bitboard can_dropBB = andnot(ALLBB, occ(BLACK) | occ(WHITE));//��̂��Ȃ��ꏊBB

	//�ʂ��ړ�������O�̉��̎���8�}�X�Ŗ����̋�̌���������ꏊ��BB
	//���̕��@�͂ǂ����Ǝv�������....(�L��֥�M)
	Bitboard friend_effectBB[8] = {ZeroBB};
	Bitboard f_effect=ZeroBB;
	/*
	641
	7k2
	853
	*/
	//int dx[8] = { -9,-9,-9,0,0,9,9,9 }, dy[8] = { -1,0,1,-1,1,-1,0,1 };
	int d[8] = { -10,-9,-8,-1,1,8,9,10 };
	for (int i = 0; i < 8; i++) {
		Square around_ksq = eksq + d[i];
		if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {
			
			//���������������̏��ɂǂ����������Ă��邩�킩��̂ł������H(�Ƃы@��̌������킩��B)
			friend_effectBB[i] = attackers_to(us, around_ksq, occ256);
			if (friend_effectBB[i].isNot()) {
				f_effect |= SquareBB[around_ksq];
			}
		}
	}
	//�܂��͂����܂œ����Ă��邩�m�F���邩(�L��֥�M)�@��OK
	cout << f_effect << endl;

	//-------------���

	//�܂��͔��
	if (num_pt(h, ROOK) != 0) {
		
		//--------------------���

		//�ߐډ���̋��łĂ閡���̌����̕����Ă���ꏊ�݂̂��l����(��Ԃ�stepeffect�Ȃ�č���Ă�����[�Ȃ��Ǝv���Ă�������ǂ���ȂƂ���Ŗ��ɗ��Ƃ͂Ȃ�(�L��֥�M))
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
			Bitboard can_escape =andnot( step_effect(enemy,KING,eksq),f_effect| rook_effect(occ_without_eksq, to));
			if (can_escape.isNot()) { return true; }
		}

		//(����ŋl�܂Ȃ���΋���p�̎΂߂ƌj�̋���݂̂��l����)�p������Ίp�ƌj�n�����ł����B

	}



	return false;
}
