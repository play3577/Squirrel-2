#include "usi.h"
#include "position.h"
#include "benchmark.h"
#include "search.h"
#include "misc.h"
#include "fundation.h"
#include "movepicker.h"
#include "Thread.h"
#include "makemove.h"
#include "learner.h"
//#include "makemove.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace USI;


USI::OptionMap Options;

//打ち歩詰めの局面
//"sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1"  
const string maturi = "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1";
const string max_pos = "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g3n17p 1";
const string nijyuuoute = "sfen lnsgk1snl/7b1/ppppppppp/9/4r4/9/PPP2PPPP/1B1g3R1/LNSGKGSNL b 2P 1";
const string oute = "sfen lnsgk1snl/7b1/ppppppppp/9/4r4/9/PPPg1PPPP/1B5R1/LNSGKGSNL b 2P 1";
const string suicide = "sfen lnsgkgsnl/1r7/pppppp1pp/6p2/8P/6P2/PP1PPP1P1/1B3K1R1/LNSG+bGSNL b P 1";
const string capturepromote = "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL b - 1";
const string drop = "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PP1PPPPPP/1B5R1/LNSGKGSNL b P 1";

void USI::init_option(OptionMap &o)
{
	o["Ponder"] << USIOption(false);
	o["Threads"] << USIOption(1, 1, 128);
	o["eval"] << USIOption("c:/book/eval/");
}


/// operator<<() is used to print all the options default values in chronological
/// insertion order (the idx field) and in the format defined by the UCI protocol.
std::ostream& USI::operator<<(std::ostream& os, const OptionMap& om) {

	for (size_t idx = 0; idx < om.size(); idx++) {

		for (const auto& it : om) {
			if (it.second.idx == idx)
			{
				const USIOption& o = it.second;
				os << "\noption name " << it.first << " type " << o.type;

				if (o.type != "button") {
					os << " default " << o.value;
				}

				if (o.type == "spin") {
					os << " min " << o.min << " max " << o.max;
				}
				if (o.type == "button") {
					os << " defalt ";
					if (o.value == "true") { os << "true"; }
					else { os << "false"; }
				}
				break;
			}
		}
	}

	return os;
}


void USI::loop()
{
	Position pos;
	Thread th;
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
			cout << Options << endl;
			cout << "usiok" << endl;
		}
		else if (token == "go") {
			//cout << "未実装" << endl;
			limit.starttime = now();
			Value v;
			th.set(pos);
			v = th.think();
			cout << " 評価値 " << v << endl;


		}
		else if (token == "setoption") {
/*
>C:setoption name Threads value 2
>C:setoption name Hash value 32
>C:setoption name Ponder value true
>C:setoption name WriteDebugLog value false
>C:setoption name NetworkDelay value 41
>C:quit

こんな感じでoptionが入ってくる！！！
>C:setoption name Progress value ./progress/
>C:setoption name eval value ./eval/
*/
			string buffer, name, value,type;
			is >> buffer;
			is >> name;
			is >> buffer;
			is >> value;

			USI::USIOption& option = Options[name];
			/*
			if (option == nullptr) {

			}*/

			type = option.return_type();
			if (type == "button") {
				value == "true" ? option.change(true) : option.change(false);
			}
			else if (type == "spin") {
				option.change(stoi(value));
			}
			else if (type == "string") {
				option.change(value.c_str());
			}
			else {
				UNREACHABLE;
			}

		}
		else if (token == "l") {
			string yn;
			cout << "do you really wanna learning fv? [y/n]  " ;
			cin >> yn;
			if (yn != "y") { cout << "OK I do not  learning"; break; }
			Eval::learner(th);
		}
		//====================
		//ここから下はデバッグ用コマンド
		//====================
		else if (token == "gm") {
			//指し手生成速度計測
			speed_genmove(pos);
		}
		else if (token == "maturi") {
			pos.set(maturi);
		}
		else if (token == "max") {
			pos.set(max_pos);
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
			bool suicide = pos.is_king_suiside(WHITE, move_to(m),move_from(m));
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
		else if (token == "mpick") {
			movepicker mp(pos);
			Move m;
			StateInfo si;
			while ((m = mp.return_nextmove()) != MOVE_NONE) {
				cout << m << endl;
				pos.do_move(m, &si);
				pos.undo_move();
			}
		}
		else if (token == "2jyuu") {
			pos.set(nijyuuoute);
			movepicker mp(pos);
			Move m;
			while ((m = mp.return_nextmove()) != MOVE_NONE) {
				cout << m << " " << pos.is_legal(m) << endl;
			}
		}
		else if (token == "oute") {
			pos.set(oute);
			movepicker mp(pos);
			Move m;
			while ((m = mp.return_nextmove()) != MOVE_NONE) {
				cout << m << " " << pos.is_legal(m) << endl;
			}
		}
		else if (token == "ks") {
			pos.set(suicide);
			movepicker mp(pos);
			Move m=make_move(SQ4H,SQ3G,B_KING);
			cout << m << " " << pos.is_legal(m) << endl;
			check_move(m);
			/*while ((m = mp.return_nextmove()) != MOVE_NONE) {
				cout << m << " " << pos.is_legal(m) << endl;
			}*/
		}
		else if (token == "random") {
			wrap_randomwalker();
		}
		else if (token == "smove") {

			string move;
			is >> move;
			Sfen2Move(move, pos);
		}
		else if (token == "test") {
			StateInfo si;
			ExtMove moves_[600],*end;
			end = moves_;

			cout << pos << endl;
			end = test_move_generation(pos, moves_);

			for (ExtMove* i = moves_; i < end; i++) {
				check_move(i->move);
				pos.do_move(i->move, &si);
				cout << pos << endl;
				pos.undo_move();
				cout << pos << endl;
			}
		}
		else if (token == "cappro") {
			StateInfo si;
			pos.set(capturepromote);
			const Move m = make_movepromote(SQ8H, SQ3C, BISHOP);
			cout << pos << endl;
			pos.do_move(m, &si);
			cout << pos << endl;
			pos.undo_move();
			cout << pos << endl;

		}
		else if (token == "drop") {
			StateInfo si;
			pos.set(drop);
			Move m = make_drop(SQ7G, PAWN);
			cout << pos << endl;
			pos.do_move(m, &si);
			cout << pos << endl;
			pos.undo_move();
			cout << pos << endl;
		}
		else if (token == "eval") {
			cout << Eval::eval(pos) << endl;;
		}
		else if (token == "csa") {
			string csa;
			is >> csa;
			cout << CSA2Move(csa, pos) << endl;
		}
		else if (token == "komaw") {
			Eval::komawari_check();

		}

	} while (token != "quit");




}
