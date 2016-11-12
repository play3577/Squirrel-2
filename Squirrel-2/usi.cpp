#include "usi.h"
#include "position.h"
#include "benchmark.h"
#include "search.h"
#include "misc.h"
#include "fundation.h"
//#include "makemove.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace USI;

const string maturi = "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1";
const string max = "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g3n17p 1";

//打ち歩詰めの局面
//"sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1"  



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
		else if (token == "testpbb") {
			StateInfo si;
			pos.set("sfen lnsgkgsnl/1r5b1/ppppppppp/P8/9/9/1PPPPPPPP/1B5R1/LNSGKGSNL b - 1");
			Move m = make_movepromote(SQ9D, SQ9C,PAWN);
			cout << pos << endl;
			pos.do_move(m, &si);
			cout << pos << endl;
			pos.print_existpawnBB();
			pos.undo_move();
			cout << pos << endl;
			pos.print_existpawnBB();
		}
		else if (token == "uchihu") {
			//打ち歩詰めの局面
			pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1");
			bool uchihu= pos.is_uchihu(BLACK, SQ5E);
			cout << uchihu << endl;
		}
		else if (token=="suicide") {
			pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL w P 1");
			Move m = make_move(SQ5D, SQ5E, W_KING);
			bool suicide = pos.is_king_suiside(WHITE, move_to(m));
			cout << (suicide)<< endl;
		}
		else if(token=="legal1")
		{
			//falseになるはず
			pos.set("sfen lns1kgsnl/1r1p3b1/ppp1gpppp/9/9/4R4/PPPPPPPPP/1B7/LNSGKGSNL w p 1");
			Move m = make_move(SQ5C, SQ6C,W_GOLD);
			bool legal = pos.is_legal(m);
			cout << legal << endl;
		}
		else if (token == "legal2")
		{
			//trueになるはず
			pos.set("sfen lns1kgsnl/1r2p2b1/ppp1gpppp/9/9/4R4/PPPPPPPPP/1B7/LNSGKGSNL w p 1");
			Move m = make_move(SQ5C, SQ6C, W_GOLD);
			bool legal = pos.is_legal(m);
			cout << legal << endl;
		}

	} while (token != "quit");




}
