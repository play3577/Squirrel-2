#include "eval_hash.h"
#include "bitop.h"

#ifdef EVAL_PP

#ifdef EVALHASH
EHashTable EHASH;
#endif

EHASH_Entry * EHashTable::probe(const Key key, bool & found) const
{
	EHASH_Entry* const tte = first_entry(key);
	
	if (tte->key() == key) {
		found = true;
		return tte;
	}

	found = false;
	return tte;
}

void EHashTable::resize(size_t mbSize)
{
	//2の累乗個にするために最大bitを取り出す。
	size_t newclaster_count = size_t(1) << find_msb((mbSize * 1024 * 1024) / sizeof(EHASH_Entry));

	//サイズが変わらなければreturn 
	if (newclaster_count == cluster_count) { return; }

	cluster_count = newclaster_count;
	//メモリ解放
	free(mem);

	mem = calloc(cluster_count * sizeof(EHASH_Entry) + eCacheLineSize - 1, 1);


	if (!mem)
	{
		//メモリ動的確保失敗
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}
	//tableの先頭ポインタをmemに
	table = (EHASH_Entry*)((uintptr_t(mem) + eCacheLineSize - 1) & ~(eCacheLineSize - 1));

	mask = cluster_count - 1;
	clear();
}







#endif
