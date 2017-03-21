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
定跡を読み込むため機能を用意する。
定跡の形式は現在一般的に用いられているやねうら王フォーマット。
http://yaneuraou.yaneu.com/2016/02/05/%E5%B0%86%E6%A3%8B%E3%82%BD%E3%83%95%E3%83%88%E7%94%A8%E3%81%AE%E6%A8%99%E6%BA%96%E5%AE%9A%E8%B7%A1%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88%E3%81%AE/
（上のリンクに乗っているのは拡張前のフォーマット）

（しかしもしかしたら今のSquirrelのsfen()では　持ち駒の出力の順番が違って定跡を使えないかもしれないので、もしそうであればsfen()を修正してやる必要がある）


やねうら王フォーマット

sfen ln1gk1snl/1r1s2gb1/p1pppppp1/1p6p/7P1/2P3P2/PP1PPP2P/1B5R1/LNSGKGSNL b - 9
6i7h 8d8e 0 32 62
3i4h 8d8e 0 32 17
3i3h 8d8e 0 32 2

(sfen文字列)
(その局面での指し手) (予想される応手（なければ"none"）) (その指し手で進めたときの評価値) (評価値を出したときの探索深さ)　（出現頻度）

*/

/*
mapにbookentryとsfenの値をペアにして格納する
*/
struct BookEntry {
	Move move, counter;
	Value value;
	Depth depth;
	int frequency;

	//コンストラクタ
	BookEntry(const Move m1, const Move m2, const Value v, const Depth d, const int f) {
		move = m1;
		counter = m2;
		value = v;
		depth = d;
		frequency = f;
	}

	//ソートのための演算子
	bool operator < (const BookEntry a) { return (this->frequency > a.frequency); }
};

//一つの局面に対してbookentryは多数ありうるのでvectorにしておく。
extern  std::map<std::string, std::vector<BookEntry>> book;


//定跡データストリーム
class BookDataStream {

private:
	std::istream& input_stream_;

public:
	BookDataStream(std::istream& is) : input_stream_(is) {}

	//datastreamからbookの作成を行う。
	bool preparebook();

	bool makebook();

	bool write_book(string filename);

};



namespace BOOK {
	void init();
}