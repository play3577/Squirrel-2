#include "book.h"

#include <sstream>
#include <fstream>

//datastreamからbookの作成を行う。

//一つの局面に対してbookentryは多数ありうるのでvectorにしておく。
std::map<std::string, std::vector<BookEntry>> book;

bool BookDataStream::preparebook() {

	Position pos;

	std::string line;
	std::string sfen;

	vector<BookEntry> bookentrys;

	while (true) {
		//1行読み込み
		if (!getline(input_stream_, line)) {
			if (sfen.size() != 0 && bookentrys.size() != 0) {
				book[pos.make_sfen()] = bookentrys;
			}
			break;
		}

		if (line.find("sfen") != string::npos) {

			if (sfen.size() != 0 && bookentrys.size() != 0) {
				book[sfen] = bookentrys;
			}

			sfen = line;
			bookentrys.clear();
		}
		else if (line.size() == 0||line.find('#')!=string::npos) {

		}
		else {
			ASSERT(sfen.size() != 0);
			pos.set(sfen);
			std::istringstream bookinfo(line);
			string smove, scounter, svalue, sdepth, sfrequency;
			bookinfo >> smove;
			bookinfo >> scounter;
			bookinfo >> svalue;
			bookinfo >> sdepth;
			bookinfo >> sfrequency;

			Move move = Sfen2Move(smove, pos);
			if (move == MOVE_NONE || !pos.is_legal(move) || pos.check_nihu(move)==true) {
				cout << pos << endl;
				cout << "LEGAL " << pos.is_legal(move) << endl;
				cout << "uchihu " << pos.is_uchihu(pos.sidetomove(), move_to(move)) << endl;
				pos.print_existpawnBB();
				cout << "ksq " << pos.state()->ksq_[opposite(pos.sidetomove())] << endl;
				cout << "move " << move << endl;
				cout << pos.make_sfen() << endl;
				ASSERT(0);
			}
			StateInfo si;
			pos.do_move(move, &si);
			Move counter;
			if (scounter != "none") {
				counter = Sfen2Move(scounter, pos);
			}
			else {
				counter = MOVE_NONE;
			}
			if (counter != MOVE_NONE) {
				if (!pos.is_legal(counter) || pos.check_nihu(counter) == true) {
					cout << pos << endl;
					cout << "LEGAL " << pos.is_legal(counter) << endl;
					cout << "uchihu " << pos.is_uchihu(pos.sidetomove(), move_to(counter)) << endl;
					cout << "move " << counter << endl;
					ASSERT(0);
				}
			}

			BookEntry be(move, counter, Value(stoi(svalue)), Depth(stoi(sdepth)), stoi(sfrequency));
			bookentrys.push_back(be);

		}

	}//末端までファイルを読み込む


	if (book.size() == 0) {
		cout << " cant read book" << endl;
		return false;
	}
	return true;
}




void BOOK::init() {

	ifstream datafile(Options["bookpath"].str().c_str());
	BookDataStream bookdatastream(datafile);

	bookdatastream.preparebook();
}


/*
定跡を作成するための機能

今のソフトの棋力ではソフトに考えさせて定跡を作成するのは困難であると考えられるため、floodgateの棋譜から作成する。

この前の大会では定跡を使わないのが流行であったが
まふ定跡の件があったりして定跡作成はしたほうがいいと思われる。
後半に時間を温存するためにも....


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
#if defined(MAKEBOOK)
#include "evaluate.h"
#include "learner.h"
#include "makemove.h"
bool BOOK::makebook() {


	std::vector<Game> games;


	//設定
	int readgames = 35000;//読み込む棋譜の数　最大でも31000位

	int maxply = 40;//学習の初期局面用に60手までのも用意しておく


	ifstream gamedata(gamedatabasefile);
	

	cout << "readgames ";
	//cin >> readgames;

	//棋譜を読み込む
	Game g;

	for (int i = 0; i < readgames; ++i) {

		
		if (read_onegame(gamedata, &g)
			&& g.moves.size() >= 60
			&& g.result != 0)
		{
			games.push_back(g);
		}
	}
	cout << endl << "finish readgames" << endl;
	cout << games.size() << endl;
	//=====================
	//初期化
	//====================
	book.clear();
	Thread th;
	Position pos;
	//th.cleartable();
	StateInfo si[500];
	//定跡を作成する。
	//並列に作成したいが出現回数がおかしくなってしまうのを防ぐため並列にしない。
	
	for (int64_t num = 0; num < games.size(); num++) {
		if (num % 1000 == 0) { cout << "." << endl; }



		auto game = games[num];
		pos.set_hirate();
		th.cleartable();

		for (int ply = 0; ply <maxply; ply++) {
			si[ply].clear();
			const Move m = game.moves[ply];
			//const Move ponderm = game.moves[ply + 1];
			if (!pos.is_legal(m) || !pos.pseudo_legal(m)) { goto NEXTGAME; }

			//勝った側
			if (game.result == int(pos.sidetomove() + 1)) {

				//pos.do_move(m, &si[ply]);
				////book使用時にponderしようとすると処理が大変なのでしない
				////if (!pos.is_legal(ponderm) || !pos.pseudo_legal(ponderm)) { goto NEXTGAME; }
				//pos.undo_move();

				//winsideからのみにすべきOR定跡作成に力が入れられてきた2017年の棋譜のみから抽出すべき？？

				//やばい流れも入ってたのでこれは必要そう
				//勝った側だけを登録する場合はかえって邪魔な可能性もある
				/*th.set(pos);
				th.l_alpha = -Value_Infinite;
				th.l_beta = Value_Infinite;
				th.l_depth = 10;
				Eval::eval(pos);
				const Value score = th.think();
				if (score < -200) { goto NEXTGAME; }*/

				pos.ply_from_startpos = ply + 1;
				const string sfen = pos.make_sfen();
				//move ponder value depth frequency
				//これ一旦vectorかなんかに格納しておいて最後に一気に追加すべきかも（途中で悪い流れだったことが判明することもあるはずなので)
				BookEntry be(m, MOVE_NONE, Value(0), Depth(0), 1);

				auto bookentry = book.find(sfen);

				if (bookentry != book.end()) {
					bool find = false;

					std::vector<BookEntry>& entrys = bookentry->second;
					//すでにこの局面この指し手が登録されていれば出現回数を追加する。
					for (int i = 0; i < entrys.size(); i++) {

						if (entrys[i].move == m
							//&&entrys[i].counter == ponderm
							) {
							entrys[i].frequency++;
							find = true;
						}

					}
					//そうでなければ新しい差し手として追加
					if (find != true) { book[sfen].push_back(be); }
					//出る順でソート
					sort(bookentry->second.begin(), bookentry->second.end());
				}
				else {
					book[sfen].push_back(be);
				}
			}
		NEXTMOVE:;
			pos.do_move(m, &si[ply]);
			
		}//while ply
NEXTGAME:;

	}//while games



	//ここでbookを書き出す
	write_book("C:/book2/newbook.db");

	cout << "finish" << endl;
	return true;
}

bool BOOK::write_book(string filename)
{
	ofstream of(filename);

	auto i = book.begin();

	//ここi++でいけるかな????
	for (; i != book.end(); i++) {

		/*
sfen ln1gk1snl/1r1s2gb1/p1pppppp1/1p6p/7P1/2P3P2/PP1PPP2P/1B5R1/LNSGKGSNL b - 9
6i7h 8d8e 0 32 62
3i4h 8d8e 0 32 17
3i3h 8d8e 0 32 2

(sfen文字列)
(その局面での指し手) (予想される応手（なければ"none"）) (その指し手で進めたときの評価値) (評価値を出したときの探索深さ)　（出現頻度）
		bookでponderをするのは厄介なのでもうぜんぶnoneにしておく

		*/
		of << i->first << endl;
		for (int m = 0; m < i->second.size(); m++) {
			of << i->second[m].move << " " << /*i->second[m].counter*/"none" << " " << i->second[m].value << " " << i->second[m].depth << " " << i->second[m].frequency << endl;
		}
	}
	of.close();
	return true;
}

//----------------------------------------------------------------------------------------------

inline double calc_ucb(double winrate, int this_playnum, int all_plynum) {
	const double V = 1.5;
	return (winrate + pow(V*log(all_plynum) / this_playnum, 0.5));
}


struct Node;

vector<Node> Nodes;

std::map<std::string, std::vector<BookEntry>> book_ucb;

struct ucbMove {
	double ucb = -1000;
	Move move = MOVE_NONE;
	//Node* p_nextnode = nullptr;//vectorをポインタで管理しようとするとおかしくなる！だからrandomforestでもindexで管理してたのか...
	int nextindex = -1;
	Value score=Value_Zero;//この手を指した後の評価値
	int numThisArmTried = 0;//この手が展開された回数

	double winrate() {
		return win_sig(score);
	}

	ucbMove(double ucb_, Move m_) :ucb(ucb_), move(m_) {};
	ucbMove() {};
	ucbMove(Move m_) :move(m_) {};
};

// ucbMoveの並べ替えを行なうので比較オペレーターを定義しておく。
inline bool operator<(const ucbMove& first, const ucbMove& second) {
	return first.ucb > second.ucb;
}
inline bool operator>(const ucbMove& first, const ucbMove& second) {
	return first.ucb < second.ucb;
}

inline std::ostream& operator<<(std::ostream& os, ucbMove m) { os << m.move << " ucb(" << m.ucb << ") score("<<m.score<<")"; return os; }


struct Node
{
	vector<ucbMove> moves;//この局面における合法手
	bool isleaf = true;//この局面がleaf nodeか
	//ucbMove* lastMove = nullptr;
	Move lastmove;
	//Node* previous = nullptr;//以前の局面
	int previousindex = -1;
	string sfen_;//この局面のsfen文字列
	int depth=0;//この局面の深さ
	int allplayoutnum=0;//このノードの手を調べた回数
	int nodeindex = -1;


	//movesリストの初期化
	void make_movesvector(const Position& pos) {
		//moveはたくさん生成されるが将棋は悪い手が多いので探索後にsortして上位5手ぐらいにまで絞ってもいいだろう
		ExtMove psuedo[600],*pe;
		pe = psuedo;
		pe = test_move_generation(pos, psuedo);
		ptrdiff_t num_move = pe - psuedo;
		for (ptrdiff_t i = 0; i < num_move; i++) {

			if (pos.is_legal(psuedo[i].move)&&pos.pseudo_legal(psuedo[i].move)) {
				ucbMove ucbmove = ucbMove(psuedo[i].move);
				moves.emplace_back(ucbmove);
			}
		}
	}

	//movesリストのソートとリサイズ
	void sort_resize_moves() {
		//ここで大きいほうに並び替えられる。
		std::stable_sort(moves.begin(), moves.end(), [](const ucbMove& m1, const ucbMove& m2) {return m1.score > m2.score; });
		//大きいほうから6手だけ残す
		if (moves.size() < 6) return;

		moves.resize(6);


		//二分探索で評価値が閾値以下になってしまう場所を探す
		//int lb = 0,ub=moves.size();
		//int mid;
		//int threshold = -200;
		//while (ub - lb > 1) {
		//	mid = (ub + lb) / 2;
		//	if (moves[mid].score> threshold) {
		//		//解の範囲は[mid,ub]に絞られる
		//		lb = mid;
		//	}
		//	else {
		//		ub = mid;
		//	}
		//}
		////評価値が閾値以下の値をリストから消す
		//moves.resize(lb);

	}


};

inline std::ostream& operator<<(std::ostream& os, Node n) { os <<"node:move[0]"<< n.moves[0]; return os; }


#include <random>



//ucbが最大となる末端局面まで移動する
void movebestUcbPos(Position& pos, int index) {

	ucbMove* bestucbmove = nullptr;
	Value v;
	double maxucb = -1145148101919, ucb;
	StateInfo si;

	//ucbが最大となる差し手を探す
	Node* node = &Nodes[index];
	for (ucbMove& move : node->moves) {
		//ucb = move.ucb;
		calc_ucb(win_sig(v), node->moves[i].numThisArmTried, node->allplayoutnum);

		if (ucb > maxucb) {
			maxucb = ucb;
			bestucbmove = &move;
		}
	}
	//↑のようなことをせずともucbの値が出るたびにその値でソートすればよろしい
	//sortしてしまうとpointerでさかのぼれない！
	//bestucbmove = &node->moves[0];
	ASSERT(bestucbmove != nullptr);

	pos.set(node->sfen_);


	//if (bestucbmove->p_nextnode == nullptr) {
	if (bestucbmove->nextindex == -1) {
		//ここで末端局面に移動した
		pos.do_move(bestucbmove->move, &si);

		//ここでNodeも作っておく
		Node nextnode;
		nextnode.depth = node->depth + 1;
		nextnode.make_movesvector(pos);
		nextnode.sfen_ = pos.make_sfen();
		nextnode.lastmove = bestucbmove->move;
		nextnode.previousindex = node->nodeindex;
		nextnode.depth = node->depth + 1;
		nextnode.nodeindex = index + 1;

		Nodes.push_back(nextnode);

		bestucbmove->nextindex=Nodes.size()-1;

		return;
	}
	else {
		movebestUcbPos(pos, bestucbmove->nextindex);
	}
	return;
}


void BOOK::makebook_ucb() {

	Position pos;
	Thread th;
	pos.set_hirate();
	const string filepath = "ucbbook.db";

	//rootについての処理

	Node root;
	Value v;
	Move m;
	StateInfo si;


	root.depth = 1;
	root.sfen_ = pos.make_sfen();
	root.make_movesvector(pos);
	root.allplayoutnum=1;
	//ここでrootmoveの処理
	for (int i = 0; i < root.moves.size(); i++) {

		si.clear();
		pos.do_move(root.moves[i].move, &si);//この手を指した時の評価値を計算

		th.set(pos);
		th.l_depth = 11;
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		v = -th.think();//相手から見た評価値になってる！
		if (v == -Value_Infinite) { continue; }
		root.moves[i].score = v;
		root.moves[i].numThisArmTried += 1;
		root.moves[i].ucb = calc_ucb(win_sig(v), root.moves[i].numThisArmTried, root.allplayoutnum);
		root.moves[i].nextindex =-1;
		root.nodeindex = 0;
		std::cout << root.moves[i] << endl;

		pos.undo_move();
	}
	//すべての差し手について評価値が計算された
	root.sort_resize_moves();
	std::cout << "sorted" << endl;
	for (ucbMove m : root.moves) { std::cout << m << endl; }

	Nodes.emplace_back(root);

	double bestucb;
	while (true) {
		//末端まで移動
		movebestUcbPos(pos,0);
		Node* node = &Nodes[Nodes.size() - 1];
		bestucb = -114514810;
		//ここでrootmoveの処理
		node->allplayoutnum++;
		pos.set(node->sfen_);
		std::cout << pos << endl;

		for (int i = 0; i < node->moves.size(); i++) {

			si.clear();
			cout << node->moves[i].move << endl;
			pos.do_move(node->moves[i].move, &si);//この手を指した時の評価値を計算

			th.set(pos);
			th.l_depth = 11;
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			v = -th.think();//相手から見た評価値になってる！
			if (v == -Value_Infinite) { continue; }
			node->moves[i].score = v;
			node->moves[i].numThisArmTried += 1;
			node->moves[i].ucb = calc_ucb(win_sig(v), node->moves[i].numThisArmTried, node->allplayoutnum);



			//std::cout << node->moves[i] << endl;

			pos.undo_move();
		}
		node->sort_resize_moves();
		for (ucbMove m : node->moves) { std::cout << m << endl; }

		//ここで木の根に向かってデータをを更新していく




		std::cout << "a";

	}






	std::cout << "test" << endl;
}

//評価値の値を逆伝搬する
void BackPropagationScore(Node* nownode, Value score,Move m) {

}



Value makemoveloop(Position& pos, Node& node,mt19937& mt,Thread& th) {

	StateInfo si;
	ucbMove* bestucbmove=nullptr;
	Value v;
	double maxucb = -1145148101919, ucb;
	double ln_allnumplayout = std::log(node.allplayoutnum);

	//ここでucbが最大となる差し手を探す
	for (ucbMove& move : node.moves) {

		//まだ試されていない差し手
		if (move.numThisArmTried == 0) {
			ucb = 1000 + mt() % 100;

		}
		else {
			//ucbの計算
			ucb = calc_ucb(move.winrate(), move.numThisArmTried, node.allplayoutnum);
		}

		if (ucb > maxucb) {
			maxucb = ucb;
			bestucbmove = &move;
		}
	}
	ASSERT(bestucbmove != nullptr);
	//bestな手が見つかった


	//bestな差し手が末端であれば次のノードを作成する
	if (bestucbmove->nextindex==-1) {
		Node nextnode;
		pos.set(node.sfen_);
		pos.do_move(bestucbmove->move, &si);
		nextnode.depth = node.depth + 1;
		nextnode.make_movesvector(pos);
		nextnode.sfen_ = pos.make_sfen();
	}

}

#endif