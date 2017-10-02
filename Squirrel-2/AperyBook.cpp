#include "AperyBook.h"
#include "makemove.h"



//https://ameblo.jp/nana-2007-july/entry-10104294377.html
bool AperyBook::openbook(const std::string filename)
{
	if (is_open()) close();


	ifstream::open(filename.c_str(), std::ifstream::in | std::ifstream::binary | std::ios::ate);

	if (!is_open())
		return false;

	//tellgはファイルのカレント位置を返す（ここではファイルの末尾にシークされているのでファイルサイズ）
	size_ = tellg() / sizeof(AperyBookEntry);

	//ファイルの終端に達した|内部動作が原因で入力操作が失敗した|ストリームバッファの入出力操作が失敗した  以外であればgoodになる　
	if (good() == false){
		cerr << "Failed Open Apery Book" << std::endl;
		exit(0);
	}

	filename_ = filename;

	return true;
}


//zobristhashの初期化
void AperyBook::init()
{
	for (Piece pc = NO_PIECE; pc < PC_ALL; pc++) for (Square sq = SQ_ZERO; sq < SQ_NUM;sq++) {
		BookKeypsq[pc][sq] = Bookmt();
	}

	
	//HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
	for (HandPiece pt = HPawn; pt < HandPieceNum; pt++) for(int num=0;num<19;num++) {
		BookKeyhand[pt][num] = Bookmt();
	}
	BookKeyTurn = Bookmt();
}

//現局面のBookのためのkeyを返す
Key AperyBook::RetPosBookKey(const Position & pos)
{
	Key key = 0;

	Bitboard bb = pos.occ_all();

	while (bb.isNot()) {
		Square sq = bb.pop();
		key ^= BookKeypsq[pos.piece_on(sq)][sq];
	}

	const Hand h = pos.hand(pos.sidetomove());

	for (Piece pt = PAWN; pt < KING; pt++) {
		ASSERT(pt <= GOLD&&pt >= PAWN);
		key ^= BookKeyhand[Piece2AperyHandPiece[pt]][num_pt(h, pt)];
	}

	if (pos.sidetomove() == WHITE) key ^= BookKeyTurn;

	return key;
}


//bookはkeyの小さい順に並んでいる！！
void AperyBook::binary_search(const Key key)
{
	size_t ub = size_ - 1, lb = 0, mid;
	AperyBookEntry entry;

	while (lb<ub && good()) {

		mid = (ub + lb) / 2;
		ASSERT(lb <= mid&&mid <= ub);

		//begとはstreamのbeginのこと
		seekg(mid * sizeof(AperyBookEntry), std::ios_base::beg);
		read(reinterpret_cast<char*>(&entry), sizeof(entry));

		//key==entry.keyとなるentryを見つけてもほしいのは同じkeyを持つ集合の先頭entryであるので探索を続ける
		if (key <= entry.key)
			ub = mid;
		else
			lb = mid + 1;
	}

	ASSERT(ub == lb);

	//bookを発見したのでseekのいちをlowのいちにする
	seekg(lb * sizeof(AperyBookEntry), std::ios_base::beg);

}



Move AperyBook::probe(const Position & pos, const std::string & filename, const bool isPickBest)
{
	AperyBookEntry entry;
	uint16_t best = 0;
	uint16_t sum = 0;
	Move m = MOVE_NONE;
	const Key key = RetPosBookKey(pos);
	const int minBookScore = -114514;
	Value value = Value_Zero;

	//おかしいことが起こっていればここでreturn する
	if (filename_ != filename&& openbook(filename.c_str()) == false) {
		return MOVE_NONE;
	}

	//2分探索してkeyを持つ集合の先頭book要素を探す
	binary_search(key); //全然違うkeyのentryのいちに来てしまう...

	// 現在の局面における定跡手の数だけループする。
	//readだけでseekは進んでると思われ
	while (read(reinterpret_cast<char*>(&entry), sizeof(entry)), entry.key == key && good()) {

		best = std::max(best, entry.count);//さされた回数が多いほうがbest
		sum += entry.count;

		if (minBookScore <= entry.score//少なくとも最低scoreを上回っていること

										 /*
										 ランダムに選ぶ場合は乱数をsumで割ったときのあまりがcount以下になれば（countの大きい方が指されやすくなるのでランダムとは言い難いが許容範囲なのだろうか...）
										 bestなもの(出現回数において)を返す場合は  bestcount==entry.countとなるものを選ぶ。（これはwhileループを回るうちに更新されていく）
										 */
			&& ((random_() % sum < entry.count)||(isPickBest && entry.count == best))
		)
		{

			//move情報の抜き出し
			const int move = entry.ToFromPromote;
			const Square to = (Square)(move & 0x7f);
			ASSERT(is_ok(to));
			const Square from = (Square)((move >> 7) & 0x7f);
			const bool ispromote = (bool)(move && (1 << 14));
			const bool isdrop = (bool)(from >= 81);


			if (isdrop) {
				//駒打ち
				const Piece pt = (Piece)(from - SQ_NUM + 1);
				const Piece pc = add_color(pt, pos.sidetomove());
				ASSERT(PAWN <= pt&&pt < KING);
				m = make_drop(to, pc);
			}
			else {
				//コマ移動
				ASSERT(is_ok(from));
				const Piece pc = pos.piece_on(from);
				ASSERT(pc != NO_PIECE);
				//const Piece captured = pos.piece_on(to);
				if (ispromote) {
					m = make_move(from, to, pc);
				}
				else {
					m = make_movepromote(from, to, pc);
				}
			}

			value = entry.score;
		}
	}
	//cerr << m << " value:" << value << endl;
	return m;
}


AperyBook ABook;