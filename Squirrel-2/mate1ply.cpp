#include "position.h"

/*
������l�߃e�X�g�ǖ�

��Ԃ����@�l�܂��邱�Ƃ��ł���
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b RSr4p 1 OK

��Ԃ����@�ق��̋�Ŕ�ԂƂ��
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RSr4p 1 ok

��Ԃ����@�������
position sfen lngp1gRnl/2G3Gb1/pp3Sppp/3k5/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RS3p 1 OK

��Ԃ����@pin�S�}�������̂Ŕ�ԂƂ�Ȃ�
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RS3p 1 OK


�₤���@�@�@�l�܂��邱�Ƃ��ł���
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b RSr4p 1
����@�ق��̋�Ŏ���
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RSr4p 1
*/


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

������͔����ɔ�����������̂���ʒu���ς���Ă���̂ł���ł͂��߁I�I�I

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
	cout <<"feffect"<<endl<< f_effect << endl;

	//-----------------------------------------------------------------���
	bool didrookdrop = false;
	//�܂��͔��
	if (num_pt(h, ROOK) != 0) {
		
		//--------------------���

		//�ߐډ���̋��łĂ閡���̌����̕����Ă���ꏊ�݂̂��l����(��Ԃ�stepeffect�Ȃ�č���Ă�����[�Ȃ��Ǝv���Ă�������ǂ���ȂƂ���Ŗ��ɗ��Ƃ͂Ȃ�(�L��֥�M))
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

		cout <<"matecandicate"<<endl<< matecandicateBB << endl;

		//����������邱�Ƃ̂ł���A��藣�ꂽ�A�����̌��������݂����ꍇ
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			Bitboard can_escape =andnot( step_effect(enemy,KING,eksq),f_effect| rook_effect(occ_without_eksq, to));

			cout <<"canescape"<<endl<< can_escape << endl;
			//�����������Ȃ��܂��͂ق��̋�ŉ�����ߊl�ł��Ȃ��@�B�B�B�B�B�B�B�B�B������ߊl���悤�Ƃ����pin����Ă����ꍇ�͂���͋l�݂ɂȂ��Ă��܂���
			//���̕��@�͂��Ȃ�x���Ǝv��
			//����ړ�������pin�S�}�̌������ʂ�Ȃ����`�F�b�N����̂͊ȒP�ł͂Ȃ���.......
			if (can_escape.isNot()==false&& cancapture_checkpiece(to)==false) { return true; }//����ŋl�܂��ꂽ�B
		}

		//(����ŋl�܂Ȃ���΋���p�̎΂߂ƌj�̋���݂̂��l����)������͔����ɔ�����������̂���ʒu���ς���Ă���̂ł���ł͂��߁I�I
		//����ŋl�܂Ȃ��Ƃ������Ƃ���킩��͍̂��ԂƋ��̌��ł��ł͋l�܂Ȃ��Ƃ������炢�̂��̂�....��������...
		didrookdrop = true;
	}
	//-------------------��
	

	return false;
}
/*
����������Ă������Ƃ�邩�ǂ���
�����ĂƂ����Ƃ��ɋʂɉ��肪������Ȃ���
*/
bool Position::cancapture_checkpiece(Square to) {

	const Color us = sidetomove();
	const Color enemy = opposite(sidetomove());
	const Square eksq = ksq(enemy);//�l�܂���������ʂ̈ʒu�B

	//�󂯑��̉����֊�@�̂����̈ʒu
	Bitboard enemygurder = effect_toBB_withouteffectking(enemy, to);

	cout <<"gurader"<<endl<< enemygurder << endl;

	while (enemygurder.isNot()) {
		Square from = enemygurder.pop();
		Occ_256 occ_movedgurad = occ256^SquareBB256[to] ^ SquareBB256[from];
		//��𓮂�������,���Ɍ�����������Ȃ������̂ł���͉�����ꂽ
		if (attackers_to(us, eksq, occ_movedgurad).isNot()==false) {
			return true;
		}
	}

	//�������ق��̋�Ŏ��Ȃ������B
	return false;
}