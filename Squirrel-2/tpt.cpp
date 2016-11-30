#include "tpt.h"
#include "bitop.h"

TranspositionTable TT; // Our global transposition table


// TranspositionTable::resize() sets the size of the transposition table,
// measured in megabytes. Transposition table consists of a power of 2 number
// of clusters and each cluster consists of ClusterSize number of TTEntry.
/*
この関数はTPTのサイズをMB単位でresizeする
TPTは２の累乗個のクラスタを持ち、クラスタにはclustersize個のttentryがある。
*/
void TranspositionTable::resize(const size_t mbSize)
{
	//2の累乗個にするために最大bitを取り出す。
	size_t newclaster_count = size_t(1) << find_msb((mbSize * 1024 * 1024) / sizeof(Cluster));

	//サイズが変わらなければreturn 
	if (newclaster_count == cluster_count) { return; }

	cluster_count = newclaster_count;
	//メモリ解放
	free(mem);

	/*＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝
	SFではCacheLineSize - 1　だけ余分に確保していたがウチではalignes(64)を指定してるから揃えてくれると思う（適当）
	＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝*/
	mem = calloc(cluster_count * sizeof(Cluster), 1);


	if (!mem)
	{
		//メモリ動的確保失敗
		std::cerr << "Failed to allocate " << mbSize
			<< "MB for transposition table." << std::endl;
		exit(EXIT_FAILURE);
	}
	//tableの先頭ポインタをmemに
	table = (Cluster*)(uintptr_t(mem));
	generation_u8 = 0;
	mask = cluster_count - 1;
	clear();
}


// TranspositionTable::probe() looks up the current position in the transposition
// table. It returns true and a pointer to the TTEntry if the position is found.
// Otherwise, it returns false and a pointer to an empty or least valuable TTEntry
// to be replaced later. The replace value of an entry is calculated as its depth
// minus 8 times its relative age. TTEntry t1 is considered more valuable than
// TTEntry t2 if its replace value is greater than that of t2.
/*
TranspositionTable::probe()はトランスポジションテーブルから現在の局面を探す（keyを用いて）
局面を見つけた場合はtrueと局面のttentryのポインタを返す。
見つからなければfalseと空のentryまたは置き換えられるための価値の低いttentryへのポインタを返す。
エントリの価値は(depth-8)*(相対age)
もしt1のreplace valueがt2より高ければt1はt2より価値があると考えられる
*/
TPTEntry * TranspositionTable::probe(const Key key, bool & found) const
{
	//keyは64bitでありTPTEntryに格納されているkeyは上位32bit
	//keyの下位数ビットを用いてclusterをさがす。
	TPTEntry* const tte = first_entry(key);
	const uint32_t key32 = key >> 32;  // Use the high 32 bits as key inside the cluster　上位32bitでクラスタ内のエントリを求める
	//見つかったエントリは上位32bitと下位数ビットはhashが一致したentryといえるので十分信頼性がある。

	//ここでクラスタ内からエントリを探す
	for (int i = 0; i < ClusterSize; ++i) {
		//keyの存在しない空きentryを見つけた||keyの一致を確認
		if (!tte[i].key32 || tte[i].key32 == key32)
		{
			//keyが一致してgenerationがいまのジェネレーションとは違った場合ジェネレーションを更新。
			if ((tte[i].genbound8 & 0xFC) != generation_u8 && tte[i].key32)
				tte[i].genbound8 = uint8_t(generation_u8 | tte[i].bound()); // Refresh

			 //一致キーならtrue　不一致ならfalseして　ポインタを返す。
			return found = (bool)tte[i].key32, &tte[i];
		}
	}

	//ここから↓クラスタ内に空エントリ、一致エントリが見つからなかった場合



	// Find an entry to be replaced according to the replacement strategy
	//書き換え戦略を用いて置き換えられるエントリを見つける
	TPTEntry* replace = tte;
	for (int i = 1; i < ClusterSize; ++i) {
		// Due to our packed storage format(uint8_t) for generation and its cyclic
		// nature we add 259 (256 is the modulus plus 3 to keep the lowest
		// two bound bits from affecting the result) to calculate the entry
		// age correctly even after generation8 overflows into the next cycle.
		/*
		generationの保存のフォーマット(uint8_t)と周期的な特性により、
		259(256+3)(256は法整数論とかで出て来る法(mod) 3は下位2bitが結果に影響をおよぼすのを防ぐために足す)
		259を足すことで世代がオーバーフローを起こしたあとも正しくエントリーの世代を比較できる


		generationは下位２ビットがゼロ　genboundは下位２ビットがゼロではない。
		よってgeneration8-genbound8はboundのぶんをひきすぎてしまうことが起こりうる。
		その分を補正しようというのが3。
		３を足しても&0xFC(252)をするので下位2bitは無視できる
		*/
		//残り深さは深い方がいい、generationは新しい方が価値が高い
		if (replace->depth8 - ((259 + generation_u8 - replace->genbound8) & 0xFC) * 2
				> tte[i].depth8 - ((259 + generation_u8 - tte[i].genbound8) & 0xFC) * 2) {
			//replaceを更新
			replace = &tte[i];
		}
	}	
	
	return found = false, replace;

}

//千分率で埋まっているtptentryの数を返す
int TranspositionTable::hashfull() const
{
	int cnt = 0;
	for (int i = 0; i < 1000 / ClusterSize; i++)
	{
		const TPTEntry* tte = &table[i].entry[0];
		for (int j = 0; j < ClusterSize; j++) {
			if ((tte[j].genbound8 & 0xFC) == generation_u8) {
				cnt++;
			}
		}
	}
	return cnt;
}

