#pragma once

#include "fundation.h"

/*
transposition table�𗘗p���邽�߂�Zoblisthash
*/

typedef uint64_t Key;

//http://yaneuraou.yaneu.com/2015/12/17/%E9%80%A3%E8%BC%89%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8Bmini%E3%81%A7%E9%81%8A%E3%81%BC%E3%81%86%EF%BC%818%E6%97%A5%E7%9B%AE/
namespace Zoblist {
	/*
	��L�u���O�����p

	HashEntry�����ƌ��ƂňقȂ�ꏊ�ł��邱�Ƃ�S�ۂ���ɂ́A
	���ƌ��ƂŁAhash�̂ǂ�����bit���قȂ�l�ɂȂ��Ă���Ηǂ����A
	��̂悤�ɃG���g���[�̃A�h���X�����߂�Ƃ�����A���̂��߂Ɏg����bit��bit0�����Ȃ��B
	�����ȊO��bit�̓A�h���X�̌v�Z�ɕK���g���Ƃ͌�������������ł���B
	�����Ő��Ȃ�bit0��0�A���Ȃ�bit0��1�Ƃ���悤�Ȏ����ɂ���B

	��������Ƃ����قǂ�psq,hand�̗����e�[�u���Ȃǂ�bit0�͎�ԏ��Ƃ��Ďg���̂�0�ɂ��Ă����Ȃ���΂Ȃ�Ȃ��B
	bit0��0�ł��鐔���������瑫��������������bit0��0�̂܂܂ł���̂ŁAbit0�ɉe�����y�ڂ��Ȃ��B

	�������Ă����āA��ԂƂ���hash key��bit0��p����B
	�����قǂ̗����e�[�u���̏���������\�[�X�R�[�h�Ō����Ƃ����Zobrist::side�����̂��߂̒萔�ł���B
	*/

	//���Ȃ�bit0��0�A���Ȃ�bit0��1�Ƃ���悤�Ȏ����ɂ���
	const Key side= uint64_t(1);

	//������bit0�͊m����0�ɂ���B
	extern Key psq[PC_ALL][SQ_NUM];
	//�Ȃ�Ŏ�����̖������l�����Ȃ��̂���
	//http://yaneuraou.yaneu.com/2015/12/16/%E9%80%A3%E8%BC%89%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8Bmini%E3%81%A7%E9%81%8A%E3%81%BC%E3%81%86%EF%BC%817%E6%97%A5%E7%9B%AE/
	//�������Q��
	extern Key hand[ColorALL][KING];

	//psq��hand��zobrist�̏�����
	void init();//end of init zobrist hash

}
