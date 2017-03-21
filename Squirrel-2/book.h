#pragma once
#include "fundation.h"
#include "misc.h"
#include "position.h"
#include "usi.h"

#include <istream>
#include <map>
#include <vector>

using namespace std;

/*
��Ղ�ǂݍ��ނ��ߋ@�\��p�ӂ���B
��Ղ̌`���͌��݈�ʓI�ɗp�����Ă����˂��牤�t�H�[�}�b�g�B
http://yaneuraou.yaneu.com/2016/02/05/%E5%B0%86%E6%A3%8B%E3%82%BD%E3%83%95%E3%83%88%E7%94%A8%E3%81%AE%E6%A8%99%E6%BA%96%E5%AE%9A%E8%B7%A1%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88%E3%81%AE/
�i��̃����N�ɏ���Ă���̂͊g���O�̃t�H�[�}�b�g�j

�i�����������������獡��Squirrel��sfen()�ł́@������̏o�͂̏��Ԃ�����Ē�Ղ��g���Ȃ���������Ȃ��̂ŁA���������ł����sfen()���C�����Ă��K�v������j


��˂��牤�t�H�[�}�b�g

sfen ln1gk1snl/1r1s2gb1/p1pppppp1/1p6p/7P1/2P3P2/PP1PPP2P/1B5R1/LNSGKGSNL b - 9
6i7h 8d8e 0 32 62
3i4h 8d8e 0 32 17
3i3h 8d8e 0 32 2

(sfen������)
(���̋ǖʂł̎w����) (�\�z����鉞��i�Ȃ����"none"�j) (���̎w����Ői�߂��Ƃ��̕]���l) (�]���l���o�����Ƃ��̒T���[��)�@�i�o���p�x�j

*/

/*
map��bookentry��sfen�̒l���y�A�ɂ��Ċi�[����
*/
struct BookEntry {
	Move move, counter;
	Value value;
	Depth depth;
	int frequency;

	//�R���X�g���N�^
	BookEntry(const Move m1, const Move m2, const Value v, const Depth d, const int f) {
		move = m1;
		counter = m2;
		value = v;
		depth = d;
		frequency = f;
	}

	//�\�[�g�̂��߂̉��Z�q
	bool operator < (const BookEntry a) { return (this->frequency > a.frequency); }
};

//��̋ǖʂɑ΂���bookentry�͑������肤��̂�vector�ɂ��Ă����B
extern  std::map<std::string, std::vector<BookEntry>> book;


//��Ճf�[�^�X�g���[��
class BookDataStream {

private:
	std::istream& input_stream_;

public:
	BookDataStream(std::istream& is) : input_stream_(is) {}

	//datastream����book�̍쐬���s���B
	bool preparebook();

	bool makebook();

	bool write_book(string filename);

};



namespace BOOK {
	void init();
}