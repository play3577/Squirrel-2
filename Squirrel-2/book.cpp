#include "book.h"





void BOOK::init()
{
}

//datastreamからbookの作成を行う。

bool BookDataStream::preparebook() {

	Position pos;

	std::string line;
	std::string sfen;

	vector<BookEntry> bookentrys;

	while (true) {
		//1行読み込み
		if (!getline(input_stream_, line)) {
			if (sfen.size() != 0 && bookentrys.size() != 0) {
				book[sfen] = bookentrys;
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
			Move counter;
			if (scounter != "none") {
				counter = Sfen2Move(scounter, pos);
			}
			else {
				counter = MOVE_NONE;
			}

			if (move == MOVE_NONE || !pos.is_legal(move) && pos.is_uchihu(pos.sidetomove(), move_to(move))) {
				ASSERT(0);
			}
			if (counter != MOVE_NONE) {
				if (!pos.is_legal(counter) && pos.is_uchihu(pos.sidetomove(), move_to(counter))) {
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
}

void BOOK::init() {

	ifstream datafile(Options["bookpath"].str().c_str());
	BookDataStream bookdatastream(datafile);

	bookdatastream.preparebook();
}
