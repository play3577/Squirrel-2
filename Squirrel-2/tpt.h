#pragma once
#include "fundation.h"

/*
トランスポジションテーブルの一つのエントリー
shogi686ではBitboardに保存すべき内容を格納していた。
そのほうがentry１つが16byteであることを確実にできるし、
int8_t使っても実際には4bitしか使わなくてもったいなくなるということが起こらないので
そちらのほうがいいのかもしれない。（コードは複雑になるが...）

やはりmeromさんすごく賢い人だ...

*/
struct TPTEntry {

private:
	uint32_t key32;//4byte
	int16_t eval16;//2byte
	int16_t value16;//2byte
	uint32_t move32;//4byte       これは実際には24bit（3byte）あれば十分である。
	uint8_t genbound8;//1byte (下2bitはbound情報)
	uint8_t depth8;//深さは256までなので8bitに収まる
	uint8_t padding[2];//tpentryはまだ空きがあるので他の情報も格納できますよ。（と言うかもっと内容を削るべきか？）
public:
	Move  move()  const { return (Move)move32; }
	Value value() const { return (Value)value16; }
	Value eval()  const { return (Value)eval16; }
	Depth depth() const { return (Depth)(depth8 * int(ONE_PLY)); }
	Bound bound() const { return (Bound)(genbound8 & 0x3); }
	
	//未実装
	void save(){}
};
//コンパイル時エラーチェックTPEntryが16byteであることを保証する。
static_assert(sizeof(TPTEntry) == 16, "");



/*
TPTには2のべき乗のクラスターを含み、それぞれのクラスタにClusterSize個のTPTEntryを持つ。
中身が空ではないそれぞれのエントリーには一つの局面の情報が格納される。
クラスタがキャッシュラインに跨がらないようにするために
クラスターのサイズはキャッシュラインのサイズで割るとこのできる値であるべきである。
最高のキャッシュパフォーマンスの為にできるだけ早く(as so0n as possible)キャッシュラインはプリフェッチされるべきである。
*/
//プリフェッチ　http://news.mynavi.jp/column/architecture/009/
/*
ループを回る処理の場合、ループの開始時点で、
今回ではなく次回に必要となるデータに対してプリフェッチ命令を発行し、
メモリアクセスを開始する。そうすると、その回の処理を終わり、ループバックして次の回の処理を開始するころには、
プリフェッチしたデータがキャッシュに入っており、
キャッシュミスによるメモリアクセスの待ち時間なしに処理を開始することができる。

このような理由からas early as possibleなのであると考えられる。
*/
class TranspositionTable {

	static constexpr int CacheLineSize = 64;
	static constexpr int ClusterSize = 4;

	//これでアライメントされたのでキャッシュラインにまたがることは無いと考えられる。
	struct alignas(64) Cluster {
		TPTEntry entry[ClusterSize];
	};
	//クラスターは64byteで固定
	static_assert(sizeof(Cluster) == 64, "");
};