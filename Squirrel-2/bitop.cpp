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
	//bb = bb&~(1 << lsb);

	//ここを_blsr_u64()で代用できる？merom686さんのアドバイス(これAVX2命令じゃん(´･_･`))
#ifdef HAVE_AVX2
	bb = _blsr_u64(bb);
#else
	bb &= bb - 1;
#endif
	return lsb;
}


//
//uint64_t pop_lsb_blsr(uint64_t & bb)
//{
//	uint64_t lsb = find_lsb(bb);
//	//bb = bb&~(1 << lsb);
//
//	//ここを_blsr_u64()で代用できる？merom686さんのアドバイス(これAVX2命令じゃん(´･_･`))
//	//https://www.xlsoft.com/jp/products/intel/compilers/manual/cpp_all_os/GUID-D1A555E9-A30C-4A1F-8048-CA9A0DFA7811.htm
//	//http://daruma3940.hatenablog.com/entry/2016/10/10/104132
//	bb=_blsr_u64(bb);
//
//	return lsb;
//}
