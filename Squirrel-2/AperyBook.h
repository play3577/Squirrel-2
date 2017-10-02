#pragma once

#include "fundation.h"
#include "position.h"
#include <fstream>
#include <random>
/*
Apery�`����book��ǂݍ��ނ��߂̃N���X
�܂Ӓ�Ύg�������̂�....

������ă��C�u�����g�p�ɓ����邾�낤��.....
�܂�SDT5�͈ꉞ�S�g�p�\���C�u�����̍ŐV�ł�\������ł������̂Ŗ��͂Ȃ��͂�
*/

struct AperyBookEntry {
	Key key;
	uint16_t ToFromPromote;//0~6 to 7~13 from 14 promote
	uint16_t count;//�o����
	Value score;
};

class AperyBook :private std::ifstream {

private:
	//key
	Key BookKeypsq[PC_ALL][SQ_NUM];
	Key BookKeyhand[7][19];//7�����Ȃ��̂�Pawn=0�Ƃ���index�����邩��(�܂�Pt-1�Ō���)
	Key BookKeyTurn;

	std::mt19937_64 Bookmt;//�V�[�h�͉����w�肹���f�t�H���g�̒l���g���igcc(windows)��msvc�ŋ��ʂł��邱�Ƃ͊m�F�����j
	std::mt19937_64 random_;//�����_���Ɏw������Ƃ��p
	std::string filename_;
	size_t size_;

	
	
	//HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
	//							NOPIECE			PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD,
	int Piece2AperyHandPiece[8] = { HandPieceNum, HPawn,HLance,HKnight,HSilver,HBishop,HRook,HGold};

public:
	AperyBook() { std::random_device rd;  random_ = std::mt19937_64(rd()); }
	Move probe(const Position& pos, const std::string& filename_, const bool isPickBest);//��Վ��Ԃ�
	void init();//�n�b�V���̏�����
	Key RetPosBookKey(const Position &pos);

private:
	bool openbook(const std::string filename);
	void binary_search(const Key key);

};

extern AperyBook ABook;