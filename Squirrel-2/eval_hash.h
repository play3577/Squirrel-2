#pragma once
#include "fundation.h"
#include "position.h"
/*
bonanza 見たらevah_hashというものがあった
確かにwpp,bppをそういうのに格納すれば評価は早くなると思うが
keyが被ってしまうと悲惨な結果が起こる。どうすればhashが被らないようにできるのだろうか？

*/
struct EHASH_Entry
{
private:
	Key key;
	int32_t bpp = Value_error, wpp = Value_error;

public:


};