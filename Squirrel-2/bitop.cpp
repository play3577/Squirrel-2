#include "bitop.h"


uint64_t find_lsb(uint64_t &bb)
{
	unsigned long index = 0;
	
	//lsb��T���B�i�����Ȃ̂łO�Ƃ������Ƃ͂Ȃ��Ǝv����B�j
	_BitScanForward64(&index, bb);
	return (index);
}

uint64_t pop_lsb(uint64_t & bb)
{
	uint64_t lsb = find_lsb(bb);
	bb = bb&~(1 << lsb);
	return lsb;
}
