#pragma once
#include "fundation.h"
#include "Hash.h"
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
	friend class TranspositionTable;
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

public:
	static constexpr int CacheLineSize = 64;
	static constexpr int ClusterSize = 4;

	//これでアライメントされたのでキャッシュラインにまたがることは無いと考えられる。
	struct alignas(64) Cluster {
		TPTEntry entry[ClusterSize];
	};
	//クラスターは64byteで固定
	static_assert(sizeof(Cluster) == 64, "");

private:
	size_t cluster_count;
	Cluster* table;//SF8ではmemからcashlinesize-1分余分に確保していたりしたがSquirrelではalign64しているのでしなくてもいいとおもう。実際はどうなのかわからない
	void* mem;//一応コレも用意しておく
	uint8_t generation_u8;//uint_8tであるので256になると0に戻るつまり整数論的に言うとmod=256
	size_t mask;//cluster_count-1 毎回計算するのは無駄なので事前に計算しておく
public:
	~TranspositionTable() { free(mem); }//デコンストラクタ
	//goコマンドまたはponderコマンド毎にgenerationは加算される。
	void new_search() { generation_u8 += 4; }//下位2bitはboundに使われるため(0b100)ずつ増やしていく
	uint8_t generation() const { return generation_u8; }

	// The lowest order bits of the key are used to get the index of the cluster
	//クラスタのindexを求めるためにkeyの下位数ビットを用いる。
	TPTEntry* first_entry(const Key key) const {
		ASSERT(mask);
		return &table[(size_t)key & (mask)].entry[0];
	}

	//置換表のエントリの探索
	TPTEntry* probe(const Key key, bool& found) const;

	int hashfull() const;
	//TPTのサイズをMB単位でリサイズする。その時は必ず置換表をクリアすること。
	void resize(size_t mbSize);
	//置換表のクリア
	void clear() { std::memset(table, 0, cluster_count * sizeof(Cluster));}
};

extern TranspositionTable TT;