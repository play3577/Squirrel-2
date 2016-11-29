#pragma once
#include "fundation.h"

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
	uint32_t key32;//4byte
	int16_t eval16;//2byte
	int16_t value16;//2byte
	uint32_t move32;//4byte       ����͎��ۂɂ�24bit�i3byte�j����Ώ\���ł���B
	uint8_t genbound8;//1byte (��2bit��bound���)
	uint8_t depth8;//�[����256�܂łȂ̂�8bit�Ɏ��܂�
	uint8_t padding[2];//tpentry�͂܂��󂫂�����̂ő��̏����i�[�ł��܂���B�i�ƌ����������Ɠ��e�����ׂ����H�j
public:
	Move  move()  const { return (Move)move32; }
	Value value() const { return (Value)value16; }
	Value eval()  const { return (Value)eval16; }
	Depth depth() const { return (Depth)(depth8 * int(ONE_PLY)); }
	Bound bound() const { return (Bound)(genbound8 & 0x3); }
	
	//������
	void save(){}
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

	static constexpr int CacheLineSize = 64;
	static constexpr int ClusterSize = 4;

	//����ŃA���C�����g���ꂽ�̂ŃL���b�V�����C���ɂ܂����邱�Ƃ͖����ƍl������B
	struct alignas(64) Cluster {
		TPTEntry entry[ClusterSize];
	};
	//�N���X�^�[��64byte�ŌŒ�
	static_assert(sizeof(Cluster) == 64, "");
};