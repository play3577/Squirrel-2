#pragma once
#include "fundation.h"
#include "position.h"
#include <xmmintrin.h>
/*
bonanza ������evah_hash�Ƃ������̂�������
�m����wpp,bpp�����������̂Ɋi�[����Ε]���͑����Ȃ�Ǝv����
key������Ă��܂��ƔߎS�Ȍ��ʂ��N����B�ǂ������hash�����Ȃ��悤�ɂł���̂��낤���H

�Ƃ肠����key��64bit�ɂ��ĂȂ��Ȃ����Ȃ��悤�ɂ��Ă���

�����ĂƂ肠����PP�̏ꍇ��EHASH���쐬����

*/
#ifdef EVAL_PP
struct EHASH_Entry
{
private:
	friend class EHashTable;
	Key key_;
	int32_t bpp_ = Value_error, wpp_ = Value_error;

public:
	Key key()const { return key_; }
	int32_t bpp()const { return bpp_; }
	int32_t wpp()const { return wpp_; }
	void save(const Key k, const int32_t bp, const int32_t wp) {
		key_ = k;
		bpp_ = bp;
		wpp_ = wp;
	}
};
static_assert(sizeof(EHASH_Entry) == 16, "16");

class EHashTable {

public:
	static constexpr int eCacheLineSize = 64;
	//static constexpr int eClusterSize = 4;
	/*struct ECluster
	{
		EHASH_Entry entry[eClusterSize];
	};*/
	static_assert(eCacheLineSize % sizeof(EHASH_Entry) == 0, "claster");
private:
	size_t cluster_count;
	EHASH_Entry* table;
	void* mem;
	size_t mask;
public:
	~EHashTable() { free(mem); }//�f�R���X�g���N�^
	// The lowest order bits of the key are used to get the index of the cluster
	//�N���X�^��index�����߂邽�߂�key�̉��ʐ��r�b�g��p����B
	EHASH_Entry* first_entry(const Key key) const {
		ASSERT(mask);
		return &table[(size_t)key & (mask)];
	}
	EHASH_Entry* probe(const Key key, bool& found) const;
	void resize(size_t mbSize);
	//�u���\�̃N���A
	void clear() { memset(table, 0, cluster_count * sizeof(EHASH_Entry)); }

	void prefetch(Key key) const {
		//http://kaworu.jpn.org/cpp/reinterpret_cast
		//http://jp.xlsoft.com/documents/intel/seminar/2_Sofrware%20Optimize.pdf
		_mm_prefetch(reinterpret_cast<char *>(first_entry(key)), _MM_HINT_T0);
	}
};

#ifdef EVALHASH
extern EHashTable EHASH;
#endif

#endif

