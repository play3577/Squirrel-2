#pragma once
#include "fundation.h"
#include "position.h"
/*
bonanza ������evah_hash�Ƃ������̂�������
�m����wpp,bpp�����������̂Ɋi�[����Ε]���͑����Ȃ�Ǝv����
key������Ă��܂��ƔߎS�Ȍ��ʂ��N����B�ǂ������hash�����Ȃ��悤�ɂł���̂��낤���H

*/
struct EHASH_Entry
{
private:
	Key key;
	int32_t bpp = Value_error, wpp = Value_error;

public:


};