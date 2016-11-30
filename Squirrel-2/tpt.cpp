#include "tpt.h"
#include "bitop.h"

TranspositionTable TT; // Our global transposition table


// TranspositionTable::resize() sets the size of the transposition table,
// measured in megabytes. Transposition table consists of a power of 2 number
// of clusters and each cluster consists of ClusterSize number of TTEntry.
/*
���̊֐���TPT�̃T�C�Y��MB�P�ʂ�resize����
TPT�͂Q�̗ݏ�̃N���X�^�������A�N���X�^�ɂ�clustersize��ttentry������B
*/
void TranspositionTable::resize(const size_t mbSize)
{
	//2�̗ݏ�ɂ��邽�߂ɍő�bit�����o���B
	size_t newclaster_count = size_t(1) << find_msb((mbSize * 1024 * 1024) / sizeof(Cluster));

	//�T�C�Y���ς��Ȃ����return 
	if (newclaster_count == cluster_count) { return; }

	cluster_count = newclaster_count;
	//���������
	free(mem);

	/*����������������������������������������������������������������������������������������������������
	SF�ł�CacheLineSize - 1�@�����]���Ɋm�ۂ��Ă������E�`�ł�alignes(64)���w�肵�Ă邩�瑵���Ă����Ǝv���i�K���j
	����������������������������������������������������������������������������������������������������������*/
	mem = calloc(cluster_count * sizeof(Cluster), 1);


	if (!mem)
	{
		//���������I�m�ێ��s
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}
	//table�̐擪�|�C���^��mem��
	table = (Cluster*)(uintptr_t(mem));
	generation_u8 = 0;
	mask = cluster_count - 1;
	clear();
}


// TranspositionTable::probe() looks up the current position in the transposition
// table. It returns true and a pointer to the TTEntry if the position is found.
// Otherwise, it returns false and a pointer to an empty or least valuable TTEntry
// to be replaced later. The replace value of an entry is calculated as its depth
// minus 8 times its relative age. TTEntry t1 is considered more valuable than
// TTEntry t2 if its replace value is greater than that of t2.
/*
TranspositionTable::probe()�̓g�����X�|�W�V�����e�[�u�����猻�݂̋ǖʂ�T���ikey��p���āj
�ǖʂ��������ꍇ��true�Ƌǖʂ�ttentry�̃|�C���^��Ԃ��B
������Ȃ����false�Ƌ��entry�܂��͒u���������邽�߂̉��l�̒Ⴂttentry�ւ̃|�C���^��Ԃ��B
�G���g���̉��l��(depth-8)*(����age)
����t1��replace value��t2��荂�����t1��t2��艿�l������ƍl������
*/
TPTEntry * TranspositionTable::probe(const Key key, bool & found) const
{
	//key��64bit�ł���TPTEntry�Ɋi�[����Ă���key�͏��32bit
	//key�̉��ʐ��r�b�g��p����cluster���������B
	TPTEntry* const tte = first_entry(key);
	const uint32_t key32 = key >> 32;  // Use the high 32 bits as key inside the cluster�@���32bit�ŃN���X�^���̃G���g�������߂�
	//���������G���g���͏��32bit�Ɖ��ʐ��r�b�g��hash����v����entry�Ƃ�����̂ŏ\���M����������B

	//�����ŃN���X�^������G���g����T��
	for (int i = 0; i < ClusterSize; ++i) {
		//key�̑��݂��Ȃ���entry��������||key�̈�v���m�F
		if (!tte[i].key32 || tte[i].key32 == key32)
		{
			//key����v����generation�����܂̃W�F�l���[�V�����Ƃ͈�����ꍇ�W�F�l���[�V�������X�V�B
			if ((tte[i].genbound8 & 0xFC) != generation_u8 && tte[i].key32)
				tte[i].genbound8 = uint8_t(generation_u8 | tte[i].bound()); // Refresh

			 //��v�L�[�Ȃ�true�@�s��v�Ȃ�false���ā@�|�C���^��Ԃ��B
			return found = (bool)tte[i].key32, &tte[i];
		}
	}

	//�������火�N���X�^���ɋ�G���g���A��v�G���g����������Ȃ������ꍇ



	// Find an entry to be replaced according to the replacement strategy
	//���������헪��p���Ēu����������G���g����������
	TPTEntry* replace = tte;
	for (int i = 1; i < ClusterSize; ++i) {
		// Due to our packed storage format(uint8_t) for generation and its cyclic
		// nature we add 259 (256 is the modulus plus 3 to keep the lowest
		// two bound bits from affecting the result) to calculate the entry
		// age correctly even after generation8 overflows into the next cycle.
		/*
		generation�̕ۑ��̃t�H�[�}�b�g(uint8_t)�Ǝ����I�ȓ����ɂ��A
		259(256+3)(256�͖@�����_�Ƃ��ŏo�ė���@(mod) 3�͉���2bit�����ʂɉe��������ڂ��̂�h�����߂ɑ���)
		259�𑫂����ƂŐ��オ�I�[�o�[�t���[���N���������Ƃ��������G���g���[�̐�����r�ł���


		generation�͉��ʂQ�r�b�g���[���@genbound�͉��ʂQ�r�b�g���[���ł͂Ȃ��B
		�����generation8-genbound8��bound�̂Ԃ���Ђ������Ă��܂����Ƃ��N���肤��B
		���̕���␳���悤�Ƃ����̂�3�B
		�R�𑫂��Ă�&0xFC(252)������̂ŉ���2bit�͖����ł���
		*/
		//�c��[���͐[�����������Ageneration�͐V�����������l������
		if (replace->depth8 - ((259 + generation_u8 - replace->genbound8) & 0xFC) * 2
				> tte[i].depth8 - ((259 + generation_u8 - tte[i].genbound8) & 0xFC) * 2) {
			//replace���X�V
			replace = &tte[i];
		}
	}	
	
	return found = false, replace;

}

//�番���Ŗ��܂��Ă���tptentry�̐���Ԃ�
int TranspositionTable::hashfull() const
{
	int cnt = 0;
	for (int i = 0; i < 1000 / ClusterSize; i++)
	{
		const TPTEntry* tte = &table[i].entry[0];
		for (int j = 0; j < ClusterSize; j++) {
			if ((tte[j].genbound8 & 0xFC) == generation_u8) {
				cnt++;
			}
		}
	}
	return cnt;
}

