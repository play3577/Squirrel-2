#pragma once
#include "fundation.h"
#include "Hash.h"
#include <xmmintrin.h>//for prefetch(�ƌ����������̊���xmmintrin���g����̂��H�H)
/*
�g�����X�|�W�V�����e�[�u���̈�̃G���g���[
shogi686�ł�Bitboard�ɕۑ����ׂ����e���i�[���Ă����B
���̂ق���entry�P��16byte�ł��邱�Ƃ��m���ɂł��邵�A
int8_t�g���Ă����ۂɂ�4bit�����g��Ȃ��Ă��������Ȃ��Ȃ�Ƃ������Ƃ��N����Ȃ��̂�
������̂ق��������̂�������Ȃ��B�i�R�[�h�͕��G�ɂȂ邪...�j

��͂�merom���񂷂��������l��...

*/
struct TPTEntry {

private:
	friend class TranspositionTable;
	uint32_t key32;//4byte
	int16_t eval16;//2byte
	int16_t value16;//2byte
	uint32_t move32;//4byte       ����͎��ۂɂ�24bit�i3byte�j����Ώ\���ł���B
	uint8_t genbound8;//1byte (��2bit��bound���)
	uint8_t depth8;//�[����256�܂łȂ̂�8bit�Ɏ��܂�
	uint8_t padding[2];//tpentry�͂܂��󂫂�����̂ő��̏����i�[�ł��܂���B�i�ƌ����������Ɠ��e�����ׂ����H�j
	//���̂Ƃ��납���̂���������v�����Ȃ��̂�hash�𒷂����Ă�����i�[���邩�H
public:
	Move  move()  const { return (Move)move32; }
	Value value() const { return (Value)value16; }
	Value eval()  const { return (Value)eval16; }
	Depth depth() const { return (Depth)(depth8 * int(ONE_PLY)); }
	Bound bound() const { return (Bound)(genbound8 & 0x3); }
	Key key()const { return (Key)key32; }
	//������
	void save(Key k, Value v, Bound b, Depth d, Move m, Value ev, uint8_t g){
	
		//ASSERT(d / int(ONE_PLY) * int(ONE_PLY) == d);//halfply�̉���������̂ł����̓R�����g�A�E�g����
		// Preserve any existing move for the same position
		//

		//m�����݂���||position��key�̕s��v������΁@�@�i�[����w���Ă��X�V����B
		/*
		�����ǖʂɑ΂��Ďw���Ă��X�V����ꍇ�im!=MOVENONE,key==key16�j �͂��܊i�[����Ă���w���Ă����T���̌��ʗǂ��w���Ă��������Ă���\�������邽�ߎw���Ă��X�V�i����if�Ɉ���������Ȃ��Ă����v�j
		�Ⴄ�ǖʂɑ΂��Ďw���Ă��X�V����im!=MOVENONE,key!=key16�j ����if�ɂ͕K������������B
		�Ⴄ�ǖʂɑ΂���MOVE_NONE��˂����ށimovenone��˂�����Ŋ��������Ƃ�����̂��H�H�H�j����if�ɂ͕K������������B
		*/
		if (m || (k >> 32) != key32) {
			move32 = (uint32_t)m;
		}
		// Don't overwrite more valuable entries
		/*
		key�̕s��v�ł���Ώ�Ŏw���Ă��X�V���Ă���̂ŋǖʑS�̂��X�V����B
		|| �c��T���[�����傫����΁i�[���T������Ԃ��Ă������ʂ��Ƃ������Ɓj�i�܂������[���ł���̂Ő󂢑w�̏��̂ق����D�悵�Ċi�[�����ׂ��ł���B�j
		||	BOUND�QEXACT��PVNode�̒T�����ʂł���̂ŏ㏑��
		*/
		if ((k >> 32) != key32
			|| d / ONE_PLY > depth8 - 4
			/* || g != (genBound8 & 0xFC) // Matching non-zero keys are already refreshed by probe() *///�@genbound8�́@TT.probe�Ń��t���b�V������Ă���
			|| b == BOUND_EXACT)
		{
			key32 = (uint32_t)(k >> 32);
			value16 = (int16_t)v;
			eval16 = (int16_t)ev;
			genbound8 = (uint8_t)(g | b);
			depth8 = (int8_t)(d / ONE_PLY);
		}
	}//end of save()
};
//�R���p�C�����G���[�`�F�b�NTPEntry��16byte�ł��邱�Ƃ�ۏ؂���B
static_assert(sizeof(TPTEntry) == 16, "");

/*
TPT�ɂ�2�ׂ̂���̃N���X�^�[���܂݁A���ꂼ��̃N���X�^��ClusterSize��TPTEntry�����B
���g����ł͂Ȃ����ꂼ��̃G���g���[�ɂ͈�̋ǖʂ̏�񂪊i�[�����B
�N���X�^���L���b�V�����C���Ɍׂ���Ȃ��悤�ɂ��邽�߂�
�N���X�^�[�̃T�C�Y�̓L���b�V�����C���̃T�C�Y�Ŋ���Ƃ��̂ł���l�ł���ׂ��ł���B
�ō��̃L���b�V���p�t�H�[�}���X�ׂ̈ɂł��邾������(as so0n as possible)�L���b�V�����C���̓v���t�F�b�`�����ׂ��ł���B
*/
//�v���t�F�b�`�@http://news.mynavi.jp/column/architecture/009/
/*
���[�v����鏈���̏ꍇ�A���[�v�̊J�n���_�ŁA
����ł͂Ȃ�����ɕK�v�ƂȂ�f�[�^�ɑ΂��ăv���t�F�b�`���߂𔭍s���A
�������A�N�Z�X���J�n����B��������ƁA���̉�̏������I���A���[�v�o�b�N���Ď��̉�̏������J�n���邱��ɂ́A
�v���t�F�b�`�����f�[�^���L���b�V���ɓ����Ă���A
�L���b�V���~�X�ɂ�郁�����A�N�Z�X�̑҂����ԂȂ��ɏ������J�n���邱�Ƃ��ł���B

���̂悤�ȗ��R����as early as possible�Ȃ̂ł���ƍl������B
*/
class TranspositionTable {

public:
	static constexpr int CacheLineSize = 64;
	static constexpr int ClusterSize = 4;

	//����ŃA���C�����g���ꂽ�̂ŃL���b�V�����C���ɂ܂����邱�Ƃ͖����ƍl������B
	struct alignas(64) Cluster {
		TPTEntry entry[ClusterSize];
	};
	//�N���X�^�[��64byte�ŌŒ�
	static_assert(sizeof(Cluster) == 64, "");

private:
	size_t cluster_count;
	Cluster* table;//SF8�ł�mem����cashlinesize-1���]���Ɋm�ۂ��Ă����肵����Squirrel�ł�align64���Ă���̂ł��Ȃ��Ă������Ƃ������B���ۂ͂ǂ��Ȃ̂��킩��Ȃ�
	void* mem;//�ꉞ�R�����p�ӂ��Ă���
	uint8_t generation_u8;//uint_8t�ł���̂�256�ɂȂ��0�ɖ߂�܂萮���_�I�Ɍ�����mod=256
	size_t mask;//cluster_count-1 ����v�Z����͖̂��ʂȂ̂Ŏ��O�Ɍv�Z���Ă���
public:
	~TranspositionTable() { free(mem); }//�f�R���X�g���N�^
	//go�R�}���h�܂���ponder�R�}���h����generation�͉��Z�����B
	void new_search() { generation_u8 += 4; }//����2bit��bound�Ɏg���邽��(0b100)�����₵�Ă���
	uint8_t generation() const { return generation_u8; }

	// The lowest order bits of the key are used to get the index of the cluster
	//�N���X�^��index�����߂邽�߂�key�̉��ʐ��r�b�g��p����B
	TPTEntry* first_entry(const Key key) const {
		ASSERT(mask);
		return &table[(size_t)key & (mask)].entry[0];
	}

	//�u���\�̃G���g���̒T��
	TPTEntry* probe(const Key key, bool& found) const;

	int hashfull() const;
	//TPT�̃T�C�Y��MB�P�ʂŃ��T�C�Y����B���̎��͕K���u���\���N���A���邱�ƁB
	void resize(size_t mbSize);
	//�u���\�̃N���A
	void clear() { std::memset(table, 0, cluster_count * sizeof(Cluster));}

	//prefetch
	void prefetch(Key key) const {
		//http://kaworu.jpn.org/cpp/reinterpret_cast
		//http://jp.xlsoft.com/documents/intel/seminar/2_Sofrware%20Optimize.pdf
		_mm_prefetch(reinterpret_cast<char *>(first_entry(key)), _MM_HINT_T0);
	}
};

extern TranspositionTable TT;