#pragma once
#include "fundation.h"
#include "Hash.h"
#include <xmmintrin.h>//for prefetch(と言うかうちの環境でxmmintrinを使えるのか？？)
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
	//今のところかくのうする情報を思いつかないのでhashを長くしてそれを格納するか？
public:
	Move  move()  const { return (Move)move32; }
	Value value() const { return (Value)value16; }
	Value eval()  const { return (Value)eval16; }
	Depth depth() const { return (Depth)(depth8 * int(ONE_PLY)); }
	Bound bound() const { return (Bound)(genbound8 & 0x3); }
	Key key()const { return (Key)key32; }
	//未実装
	void save(Key k, Value v, Bound b, Depth d, Move m, Value ev, uint8_t g){
	
		//ASSERT(d / int(ONE_PLY) * int(ONE_PLY) == d);//halfplyの延長もあるのでここはコメントアウトする
		// Preserve any existing move for the same position
		//

		//mが存在する||positionのkeyの不一致があれば　　格納する指してを更新する。
		/*
		同じ局面に対して指してを更新する場合（m!=MOVENONE,key==key16） はいま格納されている指してよりも探索の結果良い指してが見つかっている可能性があるため指してを更新（下のifに引っかからなくても大丈夫）
		違う局面に対して指してを更新する（m!=MOVENONE,key!=key16） 下のifには必ず引っかかる。
		違う局面に対してMOVE_NONEを突っ込む（movenoneを突っ込んで嬉しいことがあるのか？？？）下のifには必ず引っかかる。
		*/
		if (m || (k >> 32) != key32) {
			move32 = (uint32_t)m;
		}
		// Don't overwrite more valuable entries
		/*
		keyの不一致であれば上で指してを更新しているので局面全体を更新する。
		|| 残り探索深さが大きければ（深い探索から返ってきた結果だということ）（また反復深化であるので浅い層の情報のほうが優先して格納されるべきである。）
		||	BOUND＿EXACTはPVNodeの探索結果であるので上書き
		*/
		if ((k >> 32) != key32
			|| d / ONE_PLY > depth8 - 4
			/* || g != (genBound8 & 0xFC) // Matching non-zero keys are already refreshed by probe() *///　genbound8は　TT.probeでリフレッシュされている
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

	//prefetch
	void prefetch(Key key) const {
		//http://kaworu.jpn.org/cpp/reinterpret_cast
		//http://jp.xlsoft.com/documents/intel/seminar/2_Sofrware%20Optimize.pdf
		_mm_prefetch(reinterpret_cast<char *>(first_entry(key)), _MM_HINT_T0);
	}
};

extern TranspositionTable TT;