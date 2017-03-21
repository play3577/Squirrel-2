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
#include "game_database.h"

bool BOOK::makebook() {


	std::vector<Game> games;


	//設定
	int readgames = 35000;//読み込む棋譜の数　最大でも31000位

	int maxply = 30;//30手までの定跡を作成する


	ifstream gamedata(gamedatabasefile);
	
	GameDataStream gamedatastream(gamedata);

	cout << "readgames ";
	cin >> readgames;

	//棋譜を読み込む
	Game g;

	for (int i = 0; i < readgames; ++i) {

		
		if (gamedatastream.read_onegame(&g)
			&& g.ply >= 60
			&& g.result != draw)
		{
			games.push_back(g);
		}
	}
	cout << endl << "finish readgames" << endl;

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

		auto game = games[num];
		pos.set_hirate();
		th.cleartable();

		for (int ply = 0; ply <maxply; ply++) {

			si[ply].clear();
			const Move m = game.moves[ply];
			const Move ponderm = game.moves[ply + 1];
			if (!pos.is_legal(m) || !pos.pseudo_legal(m)) { goto NEXTGAME; }
			pos.do_move(m, &si[ply]);
			if (!pos.is_legal(ponderm) || !pos.pseudo_legal(ponderm)) { goto NEXTGAME; }
			pos.undo_move();


			//th.set(pos);
			//これでSquirrelに向いてない定跡は省きたいのだけれどSquirrelの評価関数は安定してないから厳しいだろうなぁ...
			//しゃーないので合法手は全部定跡に入れるか....
			//const Value score = th.think();
			//if (score < -100) { goto NEXTGAME; }

			pos.ply_from_startpos = ply + 1;
			const string sfen = pos.make_sfen();
			//move ponder value depth frequency
			BookEntry be(m, ponderm, Value(0), Depth(0), 1);
			
			auto bookentry = book.find(sfen);

			if (bookentry != book.end()) {
				bool find = false;

				std::vector<BookEntry>& entrys = bookentry->second;
				//すでにこの局面この指し手が登録されていれば出現回数を追加する。
				for (int i = 0; i < entrys.size(); i++) {

					if (entrys[i].move == m
						&&entrys[i].counter == ponderm) {
						entrys[i].frequency++;
						find = true;
					}

				}
				//そうでなければ新しい差し手として追加
				if (find != true) {book[sfen].push_back(be);}
				//出る順でソート
				sort(bookentry->second.begin(), bookentry->second.end());
			}
			else {
				book[sfen].push_back(be);
			}


			pos.do_move(m, &si[ply]);
		}//while ply
NEXTGAME:;

	}//while games



	//ここでbookを書き出す
	write_book("C:/book2/newbook.db");


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
		*/
		of << i->first << endl;
		for (int m = 0; m < i->second.size(); m++) {
			of << i->second[m].move << " " << i->second[m].counter << " " << i->second[m].value << " " << i->second[m].depth << " " << i->second[m].frequency << endl;
		}
	}
	of.close();
	return true;
}
