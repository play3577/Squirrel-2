#pragma once

/*
http://qhapaq.hatenablog.com/entry/2017/01/24/230851
http://qhapaq.hatenablog.com/entry/2017/01/31/195428

�P�D�I�ǂ܂ł̂P��̕��ώ��Ԃ����ς���i20��ڂŎc�莞��50���A120��ڂ��炢�Ō��������Ɨ\�z����Ȃ�P��1���ƌ������v�́j

�Q�D�ő�v�l���Ԃ����߂�i�ߎ��I�ɂ͕��ώ��Ԃ�5-7�{�j

�R�D�ǖʂ�ǂݐi�߂�

�S�D�����ɁA�ǂ݂��P��[�����閈�ɍœK�肪�ω�����m�������ς���

�T�D����ԁi�ɂ�����̐������������́j���S�ŋ��߂��m������������ǂ݂𒆒f����

���̒��œ��ɗD��Ă���̂��T�̕����ł��B
����Ԃ𑝂₷���ɁA�ǂ݂�ł��؂�����������������邱�Ƃ��~�\�ł�
�B�����i��`�F�X�j�͐[���ǂ߂ΓǂނقǁA�肪�ł܂�₷���Ȃ�Ɠ����ɓǂނׂ��ǖʂ������Ă��܂�����
�A�ߋ��̎����̓ǂ݂�M���āA�ϋl�܂����Ǝv�����畁�i�Ȃ���������ǂދǖʂ����u���Đ�ɐi�߂悤�Ƃ������z�ł��B

*/

#include "misc.h"
#include "search.h"


class TimeManeger {

private:
	TimePoint startTime;
	int optimumTime;
	int maximumTime;
public:
	void init(SearchLimit& limit, Color us, int ply);
	int optimum() const { return optimumTime; }
	int maximum()const { return maximumTime; }
	int elasped()const { return int(now() - startTime); }

};
extern TimeManeger TimeMan;
