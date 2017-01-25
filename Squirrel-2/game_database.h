#pragma once

//�Z�I�̊w�K�`���ɒ����Ă����v���O����fg2gdb������̂ł����͋Z�I�����ɂ���i�\����Ȃ��j(����ɂ��Ă��f���炵��������)

#include "fundation.h"

#include <istream>
#include <map>
#include <vector>
using namespace std;



#if defined(_MSC_VER)
#define gamedatabasefile  "C:/book2/records_sum.txt"
#define nichkihu "C:/book2/2chkifu.csa1"
#define fg2800_2ch "C:/book2/fg_2800_2chkihu.csa"
#endif
#if defined(__unix__) 
#define gamedatabasefile "/home/daruma/fvPP/records_sum.txt"
#endif

enum GameResult
{
	black_win = 1,
	white_win = 2,
	draw = 0,
};

//�ꎎ�����̃f�[�^��ێ����邽�߂̍\����

struct Game
{
	//player
	string white_P, black_P;
	//����
	GameResult result;
	//�΋Ǔ� 
	string day;
	//�w����
	vector<Move> moves;
	int ply;
};


//�f�o�b�O�p�B
std::ostream& operator<<(std::ostream& os, const Game& game);


//�Q�[���f�[�^�X�g���[��
class GameDataStream {

private:
	istream& input_stream_;

public:
	GameDataStream(istream& is) : input_stream_(is) {}
	bool read_onegame(Game* game);//��Ǖ��̃f�[�^��ǂ݂����B�i�܂��c�肪�����true�j


};