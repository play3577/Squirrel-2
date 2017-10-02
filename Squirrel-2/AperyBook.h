#pragma once

#include "fundation.h"
#include "position.h"
#include <fstream>
#include <random>
/*
Apery形式のbookを読み込むためのクラス
まふ定石使いたいので....

これってライブラリ使用に当たるだろうか.....
まあSDT5は一応全使用可能ライブラリの最新版を申し込んでおいたので問題はないはず
*/

struct AperyBookEntry {
	Key key;
	uint16_t ToFromPromote;//0~6 to 7~13 from 14 promote
	uint16_t count;//出現回数
	Value score;
};

class AperyBook :private std::ifstream {

private:
	//key
	Key BookKeypsq[PC_ALL][SQ_NUM];
	Key BookKeyhand[7][19];//7しかないのはPawn=0としてindexを見るから(つまりPt-1で見る)
	Key BookKeyTurn;

	std::mt19937_64 Bookmt;//シードは何も指定せずデフォルトの値を使う（gcc(windows)とmsvcで共通であることは確認した）
	std::mt19937_64 random_;//ランダムに指させるとき用
	std::string filename_;
	size_t size_;

	
	
	//HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
	//							NOPIECE			PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD,
	int Piece2AperyHandPiece[8] = { HandPieceNum, HPawn,HLance,HKnight,HSilver,HBishop,HRook,HGold};

public:
	AperyBook() { std::random_device rd;  random_ = std::mt19937_64(rd()); }
	Move probe(const Position& pos, const std::string& filename_, const bool isPickBest);//定跡手を返す
	void init();//ハッシュの初期化
	Key RetPosBookKey(const Position &pos);

private:
	bool openbook(const std::string filename);
	void binary_search(const Key key);

};

extern AperyBook ABook;