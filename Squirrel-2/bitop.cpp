#include "bitop.h"


uint64_t find_lsb(uint64_t &bb)
{
	unsigned long index = 0;
	
	//lsbを探す。（将棋なので０ということはないと思われる。）
	_BitScanForward64(&index, bb);
	return (index);
}

uint64_t pop_lsb(uint64_t & bb)
{
	uint64_t lsb = find_lsb(bb);
	bb = bb&~(1 << lsb);
	return lsb;
}
