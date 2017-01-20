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
