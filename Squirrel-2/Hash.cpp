#include "Hash.h"
#include <random>
#include <bitset>

using namespace std;

namespace Zoblist {
	//������bit0�͊m����0�ɂ���B
	Key psq[PC_ALL][SQ_NUM];
	//�Ȃ�Ŏ�����̖������l�����Ȃ��̂���
	//http://yaneuraou.yaneu.com/2015/12/16/%E9%80%A3%E8%BC%89%E3%82%84%E3%81%AD%E3%81%86%E3%82%89%E7%8E%8Bmini%E3%81%A7%E9%81%8A%E3%81%BC%E3%81%86%EF%BC%817%E6%97%A5%E7%9B%AE/
	//�������Q��
	Key hand[ColorALL][KING];


	//psq��hand��zobrist�̏�����
	void init() {
		//����������̏�����
		std::random_device rd;
		std::mt19937_64 mt(rd());

		//�Ֆ�
		for (Piece pc = B_PAWN; pc < PC_ALL; pc++) {
			for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
				psq[pc][sq] = (mt()&~uint64_t(1));
				//�m����0bit��0�ɂ��Ēu���Ȃ���΂Ȃ�Ȃ�
				ASSERT((psq[pc][sq] & uint64_t(1)) == 0);
				//cout << "psq"<<pc<<sq << static_cast<std::bitset<64>>(psq[pc][sq]) << endl;
			}
		}

		//���
		for (Color c = BLACK; c < ColorALL; c++) {

			for (Piece pt = PAWN; pt < KING; pt++) {
				hand[c][pt] = (mt()&~uint64_t(1));
				ASSERT((hand[c][pt] & uint64_t(1)) == 0);
				//cout << "hand" << c << pt << static_cast<std::bitset<64>>(hand[c][pt]) << endl;
			}
		}
	}



}