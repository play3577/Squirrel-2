#include "position.h"


#define matedrop
#define matemove
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

�������@�l�܂��邱�Ƃ��ł���
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b GSr4p 1 OK

�������@�ق��̋�Ŕ�ԂƂ��
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GSr4p 1 OK

�������@�������
position sfen lngp1gRnl/2G3Gb1/pp3Sppp/3k5/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GS3p 1 OK

�������@pin�S�}�������̂Ŕ�ԂƂ�Ȃ�
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GS3p 1 OK


�₤���@�@�@�l�܂��邱�Ƃ��ł���
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b Sr4p 1 OK
����@�ق��̋�Ŏ���
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b Sr4p 1 OK

��@pin�S�}�������̂Ŕ�ԂƂ�Ȃ�
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b S3p 1�@OK




���̈ړ��@�l��
position sfen lnsg1gsnl/2pppp1b1/pp1R2ppp/4k4/1B3R3/4G4/PPPP1PPPP/9/LNSGK1SNL b P 1

���ړ��@�l�܂Ȃ�
position sfen lnsg1gsnl/2pppp1b1/pp1R2ppp/4k4/5R3/4G4/PPPP1PPPP/1B7/LNSGK1SNL b P 1

�s���͈ړ������Ȃ�
position sfen lnsg1gsn1/2pppp1b1/pp1R2ppp/4k4/1B2lR3/4G4/PPPP1PPPP/9/LNSGK1SNL b P 1


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
//template<Color us> 
Move Position::mate1ply()
{
	const Color us = sidetomove();
	//ASSERT(us == sidetomove());
	const Color enemy = opposite(us);
	const Square eksq = ksq(enemy);//�l�܂���������ʂ̈ʒu�B
	const Hand h = hand(us);//��ԑ��̎�����B

	Bitboard f_effect = ZeroBB;
	Bitboard can_dropBB = andnot(ALLBB, occ(BLACK) | occ(WHITE));//��̂��Ȃ��ꏊBB
	const Occ_256 occ_without_eksq = occ256^SquareBB256[eksq];//����̉��̏ꏊ��������occupied
	//�ʂ��ړ�������O�̉��̎���8�}�X�Ŗ����̋�̌���������ꏊ��BB
	//���̕��@�͂ǂ����Ǝv�������....(�L��֥�M)
	Bitboard friend_effectBB[8] = {ZeroBB};
	Bitboard matecandicateBB;
	Bitboard can_escape;

	int d[8] = { -10,-9,-8,-1,1,8,9,10 };
	bool didrookdrop=false;
	bool didgolddrop = false;
	bool didbishopdrop = false;

	//---------���̂Ƃ�������������
	if (h == (Hand)0) { goto movecheck; }

	/*
	641
	7k2
	853
	*/
	//int dx[8] = { -9,-9,-9,0,0,9,9,9 }, dy[8] = { -1,0,1,-1,1,-1,0,1 };

	for (int i = 0; i < 8; i++) {
		Square around_ksq = eksq + d[i];
		if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {
			
			//���������������̏��ɂǂ����������Ă��邩�킩��̂ł������H(�Ƃы@��̌������킩��B)
			friend_effectBB[i] = attackers_to(us, around_ksq, occ256);
			if (friend_effectBB[i].isNot()) {
				//------------------------------------------------------------------------------------------
				//������....feffect�͋��ł������ɎՂ��ď�Ԃ��ς���Ă��܂����Ƃ�����̂�....
				//����͋��łꏊ�����߂�Ƃ��ɂ͎g���邪�A����������ꏊ���m�肳����Ƃ��ɂ͎g���Ȃ��B
				//------------------------------------------------------------------------------------------
				f_effect |= SquareBB[around_ksq];
			}
		}
	}
	//�܂��͂����܂œ����Ă��邩�m�F���邩(�L��֥�M)�@��OK
	//cout <<"feffect"<<endl<< f_effect << endl;

	//-----------------------------------------------------------------���
	//----------------------------------------------���
#ifdef matedrop
	if (num_pt(h, ROOK) != 0) {
		//�ߐډ���̋��łĂ閡���̌����̕����Ă���ꏊ�݂̂��l����(��Ԃ�stepeffect�Ȃ�č���Ă�����[�Ȃ��Ǝv���Ă�������ǂ���ȂƂ���Ŗ��ɗ��Ƃ͂Ȃ�(�L��֥�M))
		matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

//		cout <<"matecandicate"<<endl<< matecandicateBB << endl;
		Square to;
		
		//����������邱�Ƃ̂ł���A��藣�ꂽ�A�����̌��������݂����ꍇ
		while (matecandicateBB.isNot()) {
			to = matecandicateBB.pop();

			
	//		cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape =andnot( step_effect(enemy,KING,eksq),/*f_effect*/SquareBB[to]| rook_effect(occ_without_eksq, to));

//			cout <<"canescape"<<endl<< can_escape << endl;
			//�����������Ȃ��܂��͂ق��̋�ŉ�����ߊl�ł��Ȃ��@�B�B�B�B�B�B�B�B�B������ߊl���悤�Ƃ����pin����Ă����ꍇ�͂���͋l�݂ɂȂ��Ă��܂���
			//���̕��@�͂��Ȃ�x���Ǝv��
			//����ړ�������pin�S�}�̌������ʂ�Ȃ����`�F�b�N����̂͊ȒP�ł͂Ȃ���.......
			if (cancapture_checkpiece(to) == true) { 
				goto cant_matedrop_rook; 
			}
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B

			//set��cancapture�̌�ōs��
			set_occ256(to);
			put_piece(us, ROOK, to);

			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) { 
					remove_occ256(to);
					remove_piece(us, ROOK, to);
					goto cant_matedrop_rook; 
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B
			remove_occ256(to);
			remove_piece(us, ROOK, to);
			
			return make_drop(to,add_color(ROOK,us));
cant_matedrop_rook:;
			
		}
	
		//(����ŋl�܂Ȃ���΋���p�̎΂߂ƌj�̋���݂̂��l����)������͔����ɔ�����������̂���ʒu���ς���Ă���̂ł���ł͂��߁I�I
		//����ŋl�܂Ȃ��Ƃ������Ƃ���킩��͍̂��ԂƋ��̌��ł��ł͋l�܂Ȃ��Ƃ������炢�̂��̂�....��������...
		didrookdrop = true;
	}

	//�����܂�OK

	//---------------------------------��


	if (num_pt(h, GOLD) != 0) {

		Square kingback = ((us == BLACK) ? eksq - Square(1) : eksq + Square(1));
		//king���[�����ɂ���Ό��ɂ͑łĂȂ��̂�errorSq�ɂ��Ă���
		if ((us == BLACK&&sqtorank(eksq) == RankA) || (us == WHITE&&sqtorank(eksq) == RankI)) { kingback = Error_SQ; }

		matecandicateBB = can_dropBB&StepEffect[enemy][GOLD][eksq] & f_effect;
		
		//������Ԃ����ċl�܂Ȃ����Ƃ��m�F����Ă��āA�ʂ��[�����ɂ��Ȃ���΁A��납�����ł̂���߂�K�v������
		if (didrookdrop&&kingback != Error_SQ) { andnot(matecandicateBB, SquareBB[kingback]); }


		
//		cout << "matecandicate" << endl << matecandicateBB << endl;
		//����������邱�Ƃ̂ł���A��藣�ꂽ�A�����̌��������݂����ꍇ
		while (matecandicateBB.isNot()) {
			 const Square to = matecandicateBB.pop();

		
//			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect|*/SquareBB[to]|StepEffect[us][GOLD][to]);

//			cout << "canescape" << endl << can_escape << endl;
			
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_gold; }

			set_occ256(to);
			put_piece(us, GOLD, to);
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B
			while(can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, GOLD, to); 
					goto cant_matedrop_gold;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B������to��0�ɂȂ��Ă��܂��Ă���I�I
			remove_occ256(to);
			remove_piece(us, GOLD, to);
			//cout << "mate GOLD:"<<to<< endl;
			return make_drop(to, add_color(GOLD,us));
cant_matedrop_gold:;
			
		}
		didgolddrop = true;
	}

	//------------------------------------�p
	if (num_pt(h, BISHOP)) {

		matecandicateBB = can_dropBB&StepEffect[enemy][BISHOP][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape = andnot(step_effect(enemy, KING, eksq),/* f_effect*/SquareBB[to] | bishop_effect(occ_without_eksq,to));
//			cout << "canescape" << endl << can_escape << endl;

			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_bishop; }

			set_occ256(to);
			put_piece(us, BISHOP, to);
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
	//			cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, BISHOP, to);
					goto cant_matedrop_bishop;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B
			remove_occ256(to);
			remove_piece(us, BISHOP, to);
			//cout << "mate BISHOP" << endl;
			return make_drop(to, add_color(BISHOP,us));
cant_matedrop_bishop:;
		}
		didbishopdrop = true;
	}
	//-----------------------------------��
	/*
	���ŋl�܂Ȃ������ꍇ�͎΂ߌ�납��݂̂��l����B
	�p�ŋl�܂Ȃ������ꍇ�͑O����݂̂��l����
	�����ŋl�܂Ȃ������ꍇ�͍l����K�v�͂Ȃ��B
	*/
	if (num_pt(h, SILVER) != 0&&(!didbishopdrop||!didgolddrop)) {

		matecandicateBB = can_dropBB&StepEffect[enemy][SILVER][eksq] & f_effect;
		if (didbishopdrop) { matecandicateBB=matecandicateBB & StepEffect[enemy][GOLD][eksq]; }
		else if (didgolddrop) { andnot(matecandicateBB, StepEffect[enemy][GOLD][eksq]); }

//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape = andnot(step_effect(enemy, KING, eksq),/* f_effect */SquareBB[to]| StepEffect[us][SILVER][to]);
//			cout << "canescape" << endl << can_escape << endl;
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_silver; }

			set_occ256(to);
			put_piece(us, SILVER, to);
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, SILVER, to);
					goto cant_matedrop_silver;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B
			remove_occ256(to);
			remove_piece(us, SILVER, to);
			return make_drop(to, add_color(SILVER,us));
cant_matedrop_silver:;
		}
	}
	//---------------------------------����
	//��Ԃŋl�܂Ȃ������ꍇ�͍l����K�v�͂Ȃ�
	if (num_pt(h, LANCE) != 0&&!didrookdrop) {
		matecandicateBB = can_dropBB&StepEffect[enemy][LANCE][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect*/SquareBB[to] | lance_effect(occ_without_eksq,us,to));
//			cout << "canescape" << endl << can_escape << endl;
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_LANCE; }

			set_occ256(to);
			put_piece(us, LANCE, to);
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, LANCE, to);
					goto cant_matedrop_LANCE;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B
			remove_occ256(to);
			remove_piece(us, LANCE, to);
			return make_drop(to, add_color(LANCE,us));
		cant_matedrop_LANCE:;
		}
	}
	//---------------------------------�j�n
	if (num_pt(h, KNIGHT) != 0) {
		//�j�n�͂ق��̋�Ŏx���Ă��炤�K�v�͂Ȃ�
		matecandicateBB = can_dropBB&StepEffect[enemy][KNIGHT][eksq];
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//���������邱�Ƃ̂ł��鏡
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect*/SquareBB[to]);
//			cout << "canescape" << endl << can_escape << endl;

			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_KNIGHT; }

			set_occ256(to);
			put_piece(us, KNIGHT, to);
			//�����łȂ��ꍇ�͓�����̌��������؂��Ă����B
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//������B
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, KNIGHT, to);
					goto cant_matedrop_KNIGHT;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B
			}
			//goto���Ŕ�΂���Ȃ������Ƃ������Ƃ͂܂��ꂽ�B
			remove_occ256(to);
			remove_piece(us, KNIGHT, to);
			return make_drop(to, add_color(KNIGHT,us));
cant_matedrop_KNIGHT:;
		}

	}
#endif
movecheck:;

	//----------------------------------���������̈ړ��ɂ�鉤��



#ifdef matemove
	//dc_candicate�Ƃ͓�d������܂艤�ւ̖����̌������Ղ��Ă��閡���̋�B��������ŉ���������邱�Ƃ��ł���Γ�d����ɂȂ肤�邵�A��������O��邾���ł��Ԑډ���ɂȂ�B

	//pined�Ƃ�pin����Ă����̋������������Ă����̕����ȊO�֓������Ẳ���͂ł��Ȃ����A�������̕����ŉ�����ł����Ƃ��Ă�pin���Ă����Ƃ�Ȃ������ꍇ�͎��Ԃ����B
	//�܂�pin�S�}�ɂ�鉤��͂��Ȃ蕡�G�ŋl�܂��邱�Ƃ̂ł���\���͏��Ȃ��̂�,pin�S�}�ɂ�鉤��͍l���Ȃ��ق��������̂�������Ȃ�
	//Bitboard dc_candicate[ColorALL],pinned[ColorALL];

	////slider_blockers(us, eksq, dc_candicate[us], pinned[enemy]);//�U��
	//slider_blockers(enemy, ksq(us), dc_candicate[enemy], pinned[us]);//�� �ق����͎̂�����pin�S�}�̈ʒu

	//��̈ړ���B�ߐډ���݂̂��l����̂�esq�ɖ�����gold��u�������̊�@������ꏊ�A�����̋�̂��Ȃ��ꏊ�łȂ���΂Ȃ�Ȃ�
	const Bitboard movetoBB_GOLD = andnot(StepEffect[enemy][GOLD][eksq], occ(us));
	/*------------------------------------------------------------------------------------------
	����ł���͈͂ɂ���������������邱�Ƃ��ł���}�X�������̌����̂���}�X�ɓ��������Ƃ��A
		���̋���ق��̋�Ŏ��Ȃ��܂��͉��̓�����ɂق��̖����̌���������΋l�݁B

		�l���鉤��͋ߐډ���̂�

		����͐�ɍl������ɂ���Ď��̋�̈ړ����y�ł���Ƃ����������̂͂Ȃ��Ǝv���B
		��̈ړ��ɂ���ĔՏ�̌������ς���Ă��܂��̂�
	-------------------------------------------------------------------------------------------*/


	//�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�[�܂��͋l�܂���\���̍���������@OK�����p�X�͒ʂ����B
	Bitboard matecandicate_Gold = (occ_pt(us, GOLD)|occ_pt(us,PRO_PAWN)|occ_pt(us,PRO_LANCE)|occ_pt(us,PRO_NIGHT)|occ_pt(us,PRO_SILVER))&PsuedoGivesCheckBB[us][GOLD][eksq];
	//pin�S�}�𓮂������Ƃ��Ă͂����Ȃ��i�܂�pin�����Ă������Ƃ邱�Ƃŉ���ł���ꍇ�����邪����̓C���M�����[�Ȃ̂ōl���Ȃ��ق����������낤�j
	matecandicate_Gold = andnot(matecandicate_Gold, st->blocker[us]);

	//std::cout << matecandicate_Gold << std::endl;

	while (matecandicate_Gold.isNot()) {

		const Square from = matecandicate_Gold.pop();
		Bitboard toBB = movetoBB_GOLD&StepEffect[us][GOLD][from];
		Bitboard cankingescape;
		//�����菜��.......����Ȃ��Ƃ������Ȃ��񂾂����....
		const Piece removedpiece =piece_type( piece_on(from));
		ASSERT(removedpiece);

		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		//std::cout << toBB << endl;

		while (toBB.isNot())
		{
			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);
			//std::cout << to << endl << attackers_to(us, to, occ256) << endl;

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_mate_gold; }//�����ړ���ɖ����̋�̌����������Ă��Ȃ���΂Ƃ��Ă��܂��̂ō߂ɂȂ�Ȃ��B
			if (cancapture_checkpiece(to)) { goto cant_mate_gold; }//��������������ߊl�ł����B

			//set()��^=�Ȃ̂�,���ߊl���Ȃ���̉��肾�����ꍇ�Ƀo�O���������Ă��܂��I�I�I�I�I�I������H�H�����Ȃ��do_move�ł������������Ƃ��N�����Ă�̂ł�???
			//�C������
			if (!is_capture) { set_occ256(to); }

			//---------�悭�l������capture�̏ꍇ����̋��occ�����菜���Ă��邱�ƂɂȂ��Ă��Ȃ��̂�cancapture_checkpiece�ł������Ȃ��ƂɂȂ��ĂȂ����H�H
			//----------����to�̈ʒu�̋to�Ɍ�������邱�Ƃ͂ł��Ȃ��̂ő��v��...
			put_piece(us, GOLD, to);

			//��������͋ʂ������邱�Ƃ��ł��邩�ǂ����m�F
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][GOLD][to]));
			//std::cout << cankingescape << endl;
			while (cankingescape.isNot()) {

				const Square escapeto = cankingescape.pop();//������B
				//std::cout << "escapeto " << escapeto << endl;
				//cout << "attackers" << endl << attackers_to(us, escapeto, occ256) << endl;

				if (!attackers_to(us, escapeto, occ256).isNot()) { 
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, GOLD, to);
					goto cant_mate_gold;
				}//������ɍU�����̌������Ȃ��B  ������Ƃ����ł����̂Ŏ���to���l����B

			}

			//����while�𔲂��Ă��ꂽ�Ƃ������Ƃ͂܂���Ă��܂����Ƃ������ƁB
			//����Ȏ������@�ő��v���낤��.........
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//�ߊl�������ꍇ�͂����Ŏ�菜���Ă͂����Ȃ��I�I�I

			remove_piece(us, GOLD, to);
			return make_move(from,to, add_color(removedpiece,us));

cant_mate_gold:;

		}
		//��菜�������߂�
		//��΂ɖ߂��̂�Y��Ȃ��悤�ɂ���I�I�I�I�I�I�I�I�I�I�I�I
		set_occ256(from);
		put_piece(us, removedpiece, from);
		
	}
#if 0
	//-----------------------------------------��
	const Bitboard movetoBB_DRAGON = andnot(StepEffect[us][DRAGON][eksq],occ(us));//�ߐډ��肪�ł���c��4�}�X�@���ꂽ����͍l���Ȃ��@
	Bitboard mate_candicate_Dragon = occ_pt(us, DRAGON);//��Ԃ̏ꍇPsuedogives check�͍쐬�ł��Ȃ�
	while (mate_candicate_Dragon.isNot())
	{
		const Square from = mate_candicate_Dragon.pop();
		Bitboard toBB = movetoBB_DRAGON&(rook_effect(occ256, from) | StepEffect[us][KING][from]);
		Bitboard cankingescape;
		const Piece removedpiece = DRAGON;
		ASSERT(removedpiece == piece_type(piece_on(from)));
		//�����菜��
		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		while (toBB.isNot()) {
		
			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_matemove_dragon; }//�����ړ���ɖ����̋�̌����������Ă��Ȃ���΂Ƃ��Ă��܂��̂ō߂ɂȂ�Ȃ��B
			if (cancapture_checkpiece(to)) { goto cant_matemove_dragon; }//��������������ߊl�ł����B

			//���to�ɒu��
			if (!is_capture) { set_occ256(to); }
			put_piece(us, DRAGON, to);
			//������
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | rook_effect(occ256,to) | StepEffect[us][KING][to]));

			while (cankingescape.isNot())
			{
				const Square escapeto = cankingescape.pop();

				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, DRAGON, to);
					goto cant_matemove_dragon;
				}
			}
			//����while�𔲂��Ă��ꂽ�Ƃ������Ƃ͂܂���Ă��܂����Ƃ������ƁB
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//�ߊl�������ꍇ�͂����Ŏ�菜���Ă͂����Ȃ��I�I�I

			remove_piece(us, DRAGON, to);
			return make_move(from, to, add_color(removedpiece, us));


cant_matemove_dragon:;
		}
		set_occ256(from);
		put_piece(us, removedpiece, from);


	}

#endif
#if 0
	//-----------���j�R�[��
	const Bitboard movetoBB_UNICORN = andnot(StepEffect[us][UNICORN][eksq], occ(us));//�ߐډ��肪�ł���c��4�}�X�@���ꂽ����͍l���Ȃ��@
	Bitboard mate_candicate_UNICORN = occ_pt(us, UNICORN);//��Ԃ̏ꍇPsuedogives check�͍쐬�ł��Ȃ�
	while (mate_candicate_UNICORN.isNot())
	{
		const Square from = mate_candicate_UNICORN.pop();
		Bitboard toBB = movetoBB_UNICORN&(bishop_effect(occ256, from) | StepEffect[us][KING][from]);
		Bitboard cankingescape;
		const Piece removedpiece = UNICORN;
		ASSERT(removedpiece == piece_type(piece_on(from)));
		//�����菜��
		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		while (toBB.isNot()) {

			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_matemove_UNICORN; }//�����ړ���ɖ����̋�̌����������Ă��Ȃ���΂Ƃ��Ă��܂��̂ō߂ɂȂ�Ȃ��B
			if (cancapture_checkpiece(to)) { goto cant_matemove_UNICORN; }//��������������ߊl�ł����B

																		 //���to�ɒu��
			if (!is_capture) { set_occ256(to); }
			put_piece(us, UNICORN, to);
			//������
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | bishop_effect(occ256, to) | StepEffect[us][KING][to]));

			while (cankingescape.isNot())
			{
				const Square escapeto = cankingescape.pop();

				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, UNICORN, to);
					goto cant_matemove_UNICORN;
				}
			}
			//����while�𔲂��Ă��ꂽ�Ƃ������Ƃ͂܂���Ă��܂����Ƃ������ƁB
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//�ߊl�������ꍇ�͂����Ŏ�菜���Ă͂����Ȃ��I�I�I

			remove_piece(us, UNICORN, to);
			return make_move(from, to, add_color(removedpiece, us));


		cant_matemove_UNICORN:;
		}
		set_occ256(from);
		put_piece(us, removedpiece, from);


	}
#endif

#if 0
	//------------------------------��i����A���炸�����邽�߁A���Ȃ蕡�G����..............�j
	//��ɗ��n�𒲂ׂ��ق���������������Ȃ�
	Bitboard matecandicate_silver = (occ_pt(us,SILVER))&PsuedoGivesCheckBB[us][SILVER][eksq];
	//pin�S�}�𓮂������Ƃ��Ă͂����Ȃ��i�܂�pin�����Ă������Ƃ邱�Ƃŉ���ł���ꍇ�����邪����̓C���M�����[�Ȃ̂ōl���Ȃ��ق����������낤�j
	matecandicate_silver = andnot(matecandicate_silver, st->blocker[us]);
	const Bitboard movetoBB_SILVER = andnot(StepEffect[enemy][SILVER][eksq], occ(us));

	while (matecandicate_silver.isNot()) {

		bool canpromte_from = false;
		const Square from = matecandicate_silver.pop();
		if ((SquareBB[from] & canPromoteBB[us]).isNot()) { canpromte_from = true; }//from��3�i�ڈȓ��Ȃ̂Ő��邱�Ƃ��ł���
		Bitboard toBB = movetoBB_SILVER&StepEffect[us][SILVER][from];
		Bitboard canking_escape;
		ASSERT(piece_type(piece_on(from)) == SILVER); 
		ASSERT(piece_color(piece_on(from)) == us);
		//�܂������Ă��Ȃ��̂Ŏ�菜����͋�ł����B
		const Piece removedpiece = SILVER;
		remove_occ256(from);
		remove_piece(us,SILVER, from);
//		cout <<"toBB"<<endl<< toBB << endl;
		while (toBB.isNot())
		{
			const Square to = toBB.pop();//����ړ�������B
			bool canpromote_to;
			((SquareBB[to] & canPromoteBB[us]).isNot()) ? canpromote_to=true: canpromote_to=false;//�ړ���Ő���邩

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_mate_silver; }//�ړ���ɖ����̋�̌����������Ă��Ȃ���΂Ƃ��Ă��܂�
			if (cancapture_checkpiece(to)) { goto cant_mate_silver; }//to�ɋ��u���ƕߊl����Ă��܂�

			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!is_capture) { set_occ256(to); }
			put_piece(us, SILVER, to);
			canking_escape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][SILVER][to]));//���̓�������
//			cout << "cankingescape" << endl << canking_escape << endl;
			while (canking_escape.isNot()) {
				const Square escapeto = canking_escape.pop();//������B
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, SILVER, to);
					goto mate_progold;
				}
			}
			//�����Ă����Ƃ������Ƃ͋l�܂��邱�Ƃ��ł���
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//�ߊl�������ꍇ�͂����Ŏ�菜���Ă͂����Ȃ��I�I�I

			remove_piece(us, SILVER, to);
			return make_move(from, to, add_color(removedpiece, us));

		mate_progold:;
			//�Ȃ邱�Ƃ��ł���ꍇ(����ȕ��@����x����Ȃ�..)
			if (canpromte_from || canpromote_to) {
				if ((StepEffect[us][GOLD][to] & SquareBB[eksq]).isNot() == false) { goto cant_mate_silver; }//������ł��Ă��Ȃ�
				if (!is_capture) { set_occ256(to); }
				put_piece(us, GOLD, to);

				
				canking_escape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][GOLD][to]));//���̓�������
//				cout << "cankingescape pro" << endl << canking_escape << endl;
				while (canking_escape.isNot()) {
					const Square escapeto = canking_escape.pop();//������B
					if (!attackers_to(us, escapeto, occ256).isNot()) {
						if (!is_capture) { remove_occ256(to); }
						remove_piece(us, GOLD, to);
						goto cant_mate_silver;
					}
				}

				//�����Ă����Ƃ������Ƃ͋l�܂��邱�Ƃ��ł���
				set_occ256(from);
				put_piece(us, removedpiece, from);

				if (!is_capture) { remove_occ256(to); }//�ߊl�������ꍇ�͂����Ŏ�菜���Ă͂����Ȃ��I�I�I

				remove_piece(us, GOLD, to);
				return make_movepromote(from, to, add_color(removedpiece, us));
			}
		cant_mate_silver:;
		}//while toBB
		//��菜�������߂�
		set_occ256(from);
		put_piece(us, removedpiece, from);
	}
#endif





#endif
	return MOVE_NONE;
}
//Move Position::mate1ply() {
//	return sidetomove() == BLACK ? mate1ply<BLACK>() : mate1ply<WHITE>();
//}

/*
����������Ă������Ƃ�邩�ǂ���
�����ĂƂ����Ƃ��ɋʂɉ��肪������Ȃ���
*/
bool Position::cancapture_checkpiece(Square to) {

	const Color us = sidetomove();//�U�ߑ�
	const Color enemy = opposite(sidetomove());//�󂯑�
	const Square eksq = ksq(enemy);//�l�܂���������ʂ̈ʒu�B

	//�󂯑��̉����֊�@�̂����̈ʒu
	Bitboard enemygurder = effect_toBB_withouteffectking(enemy, to);

//	cout <<"gurader"<<endl<< enemygurder << endl;

	while (enemygurder.isNot()) {
		Square from = enemygurder.pop();
		Occ_256 occ_movedgurad = occ256|SquareBB256[to] ^ SquareBB256[from];//�������ߊl�̈ړ��������ꍇ��^Squarebb256���Ⴞ�߁B
		//��𓮂�������,���Ɍ�����������Ȃ������̂ł���͉�����ꂽ
		if (attackers_to(us, eksq, occ_movedgurad).isNot()==false) {
			return true;
		}
	}

	//�������ق��̋�Ŏ��Ȃ������B
	return false;
}

/*
sliderblockers�́@s���Ƃы@�킩�����Ă����̈ʒu�ipin�S�}�̈ʒu�j��Ԃ��B
����blocler��Տォ���菜���΂��͍U������Ă��܂��B

stm�ɂ͍U�����̐F�@�As�Ɏ󂯑��̋ʂ̈ʒu���i�[�����B
*/
void Position::slider_blockers(const Color stm, const Square s,Bitboard& dc_candicate,Bitboard& pinned) const
{
	dc_candicate = ZeroBB;
	pinned = ZeroBB;
	Bitboard betweengurad,betweenattack, pinners;

	//s�����ь����r�[��������čU�����̂ƂуS�}�ɓ�����΂��̏��͂��̋�ɂ���čU�����󂯂Ă���\��������B
	//pinners�͍U���S�}
	pinners = (
		(RookPsuedoAttack[s] & (occ_pt(stm, ROOK) | occ_pt(stm, DRAGON)))
		| BishopPsuedoAttack[s] & (occ_pt(stm, BISHOP) | occ_pt(stm, UNICORN))
		| (LancePsuedoAttack[opposite(stm)][s]&occ_pt(stm,LANCE))
		);

	while (pinners.isNot()) {

		Square to = pinners.pop();
		betweengurad = BetweenBB[s][to] & occ(opposite(stm));//���ꂪpin����Ă������
		betweenattack = BetweenBB[s][to] & occ(stm);//���ꂪ��d������

		//�ԂɈ������Ȃ�����(or�����ɋ�Ȃ�����)�̂ł��̋��pin����Ă���B
		//�����ɋ�Ȃ������ꍇ�ł�result�ɂ͉���������Ȃ��̂ő��v
		if (!more_than_one(betweenattack)) {dc_candicate |= betweenattack;}
		if (!more_than_one(betweengurad)) { pinned |= betweengurad; }
	}

	return;
}
