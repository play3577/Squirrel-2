#include "usi.h"
#include "position.h"
#include "benchmark.h"
#include "search.h"
#include "misc.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace USI;

const string maturi = "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1";
const string max = "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g3n17p 1";

void USI::loop()
{
	Position pos;
	string token, cmd;

	//pos.set_hirate();
	//pos.set(check_drop);
	do {

		if (!getline(cin, cmd)) {
			cmd = "quit";
		}

		istringstream is(cmd);//parseするためのistringstream
		token.clear();
		is >> skipws >> token;//間の空白文字は読み飛ばしてtokenに入れる。

		if (token == "usi") {
			cout << "usiok" << endl;
		}
		else if (token == "go") {
			cout << "未実装" << endl;
		
		}
		else if (token == "gm") {
			//指し手生成速度計測
			speed_genmove(pos);
		}
		else if (token == "maturi") {
			pos.set(maturi);
		}
		else if (token == "max") {
			pos.set(max);
		}
		else if (token == "hirate") { pos.set_hirate(); }
		else if (token == "dp") { std::cout << pos << std::endl; }//debug position
		else if (token == "ebb") { pos.check_effecttoBB(); }
		else if (token == "pbb") { pos.print_existpawnBB(); }


	} while (token != "quit");




}
