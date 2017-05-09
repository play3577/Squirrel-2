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
#include "book.h"
#include "progress.h"
//#include "makemove.h"
#include "reinforce_learner.h"
#include "tpt.h"

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace USI;

//positionコマンドで局面を移動させるためのstateinfo
//コレならpositionコマンドから抜けてしまってもstateinfoが消えることはないはずである
static StateInfo g_st[257];

USI::OptionMap Options;

//最初のisreadyか（何度も読みコマせるのは時間のむだなので）
bool first_ready = true;
/*
error position
position startpos moves 2g2f 3c3d 2f2e 2b3c 7g7f 4c4d 5g5f 5c5d 3g3f 3a2b 2i3g 7c7d 5f5e 5d5e 8h5e 8b5b
*/
//2歩を指した！PawnBBの差分にバグが有る？？？

//打ち歩詰めの局面
//"sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1"  
const string maturi = "sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1";
const string max_pos = "sfen R8/2K1S1SSk/4B4/9/9/9/9/9/1L1L1L3 b RBGSNLP3g3n17p 1";
const string nijyuuoute = "sfen lnsgk1snl/7b1/ppppppppp/9/4r4/9/PPP2PPPP/1B1g3R1/LNSGKGSNL b 2P 1";
const string oute = "sfen lnsgk1snl/7b1/ppppppppp/9/4r4/9/PPPg1PPPP/1B5R1/LNSGKGSNL b 2P 1";
const string suicide = "sfen lnsgkgsnl/1r7/pppppp1pp/6p2/8P/6P2/PP1PPP1P1/1B3K1R1/LNSG+bGSNL b P 1";
const string capturepromote = "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/2P6/PP1PPPPPP/1B5R1/LNSGKGSNL b - 1";
const string drop = "sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PP1PPPPPP/1B5R1/LNSGKGSNL b P 1";
const string debug1 = "sfen 5k2l/1r7/S2bp1gs1/1N2sp2p/2P7/4SP3/LP2PGN1P/3GN2r1/PK6L w 1";
const string debug2 = "sfen +S3+P3l/9/p1ppn1b2/4+R4/4Pnpk1/1P1KNr1n1/P1LP1P1+s+p/1G7/L1S1+bG2L w 1";
const string nuhu = "sfen l6nl/1r3kgs1/p1+Bspp2p/6pp1/1p7/9/PPG1PPPPP/6SK1/+r4G1NL 2PNGB2plns w 1";

/*
position startpos moves 2g2f 3c3d 2f2e 2b3c 5i6h 4a3b 5g5f 3a2b 7g7f 3c8h+ 7i8h B*6e 3g3f 6e5f 3f3e 5f4g+ 3e3d 4g4f 6h7g 4f2h 3i2h R*4g B*1f 4g4e+ 2h3g P*3f 3g2f 4e3d 6g6f 3d4e 7g7h 4e5f 8h7g 2b3c 2f3e 5f4e B*4f 3c3d 3e3d 4e4f 1f3h 3f3g+ 2i3g 4f3g 3d2c+ 3b2c 2e2d 2c2d 3h6e B*6g 7h8h 6g4i+ 6e4c+ 3g4h 6i6h 4h4c 6h7h G*2h P*2b 8b2b P*3b 2b3b P*3d 4c3d P*2b 3b2b 9g9f 3d3g S*4d 5a6b
*/

//sfen ln2kg1nl/3rg1s2/p3p1bpp/1pp6/3S1pSP1/2P6/PP1P1P2P/1BG4R1/LN1K1G1NL w PS3p 1
//P*6h

//usiで自己紹介をするための関数
const string self_introduction() {

	stringstream ss;
//
//	ss << "id name Squirrel_";
//#ifdef _DEBUG
//	ss << "debug ";
//#endif
//#ifndef _DEBUG
//	ss << "releaseTT ";
//#endif
	ss <<"id name "<< Options["EngineName"].str();
	ss << endl;
	ss << "id author Kotaro Suganuma";
	return ss.str();
}


void USI::init_option(OptionMap &o,string engine_name)
{


	string name;
	if (engine_name.size() == 0) { 
		name = "Squirrel_";
	}
	else {
		name = engine_name;
	}
	//optionが来る前に名前を設定されてしまうのでコマンドから名前を変えようとしても無駄か...
//
//	string name;
//	name += "Squirrel_";
//#ifdef _DEBUG
//	name+= "debug ";
//#endif
//#ifndef _DEBUG
//	name+="releaseTT ";
//#endif

	o["USI_Ponder"] << USIOption(false);
	o["Threads"] << USIOption(1, 1, 128);
	o["USI_Hash"] << USIOption(64, 1, 100000000);
	o["EngineName"] << USIOption(name.c_str());
	//o["is_0.1s"] << USIOption(false);
	//o["bookpath"] << USIOption("c:/book2/book2016928fg2800_40.db");
	o["Move Overhead"] << USIOption(100, 0, 3000);
	o["Slow Mover"] << USIOption(89, 10, 1000);
	o["Minimum Thinking Time"] << USIOption(100, 50, 10000);
	//o["bookpath"] << USIOption("c:/book2/standard_book.db");
	o["bookpath"] << USIOption("c:/book2/wcsc27.db");
	o["usebook"] << USIOption(true);
	o["randombook"] << USIOption(true);
	o["use_defined_time"] << USIOption(true);
	o["defined_time"] << USIOption(1000,100,100000);
#ifdef  EVAL_KPP
	o["KPP"] << USIOption("c:/yaneeval/kpp16ap.bin");
	o["KKP"] << USIOption("c:/yaneeval/kkp32ap.bin");
#endif //  EVAL_KPP
#ifdef EVAL_PROG
	o["eval_o"] << USIOption("c:/book2/Eval_p/fv_PPo.bin");
	o["eval_f"] << USIOption("c:/book2/Eval_p/fv_PPf.bin");
#else
	o["eval"] << USIOption("c:/book2/fv_PP.bin");
#endif

}


// operator<<() is used to print all the options default values in chronological
// insertion order (the idx field) and in the format defined by the UCI protocol.
std::ostream& USI::operator<<(std::ostream& os, const OptionMap& om) {

	for (size_t idx = 0; idx < om.size(); idx++) {

		for (const auto& it : om) {
			if (it.second.idx == idx)
			{
				const USIOption& o = it.second;
				os << "\noption name " << it.first << " type " << o.type;

				if (o.type != "check") {
					os << " default " << o.value;
				}

				if (o.type == "spin") {
					os << " min " << o.min << " max " << o.max;
				}
				if (o.type == "check") {
					os << " default ";
					if (o.value == "true") { os << "true"; }
					else { os << "false"; }
				}
				break;
			}
		}
	}

	return os;
}

//isreadyでセッティングするための関数
void is_ready() {
	//evalのパスが変更されているかもしれないためもう一回読み直す。
	//と言うか先にusiに返事してからreadさせたほうがいいか？
	if (first_ready == true) {
#ifdef USETT
		cout << "usihash(MB):" << (int)Options["USI_Hash"] << endl;
		TT.resize((int)Options["USI_Hash"]);
#endif
#ifdef EVAL_PP
		Eval::read_PP();
#elif EVAL_KPP 1
		Eval::read_KPP();
#endif
		if (bool(Options["usebook"]) == true) {
			BOOK::init();
		}
	}

#ifndef LEARN
	//Threads.init();
	Threads.read_usi_options();
#endif // !LEARN

	limit.is_inponder = false;
	signals.stop = signals.stopOnPonderHit = false;
	first_ready = false;
}

void go(Position& pos, istringstream& is/*, Thread& th*/) {

	limit.starttime = now();
	/*Value v;*/
	string token,buffer;
	
	limit.byoyomi = 1000;

	while (is >> token) {

		if(token=="btime"){ 
			is >> buffer;
			limit.remain_time[BLACK] = stoi(buffer);
		}
		else if (token == "wtime") {
			is >> buffer;
			limit.remain_time[WHITE] = stoi(buffer);
		}
		else if (token == "byoyomi") {
			is >> buffer;
			limit.byoyomi= stoi(buffer);
		}
		else if (token == "binc") {
			is >> buffer;
			limit.inc_time = stoi(buffer);
		}
		else if (token == "winc") {
			is >> buffer;
			limit.inc_time = stoi(buffer);
		}
		else if (token == "ponder") { limit.is_inponder = true; }
	}

#ifdef TEST
	cout << limit << endl;
#endif


#ifdef  EVAL_KPP
	Eval::eval_KPP(pos);
#else



#ifdef HAVE_AVX2
	Eval::eval_allPP_AVX2(pos);
#else
	Eval::eval_PP(pos);
#endif




#endif //  EVAL_KPP




#ifndef LEARN
	Threads.start_thinking(pos);
#endif // !LEARN

	

#if 0
	th.set(pos);
	v = th.think();
#endif
}
/*
こんなんが入ってくる

position startpos moves 7g7f 8c8d 7i6h 3c3d 6h7g 7a6b 2g2f 3a4b 3i4h 4a3b 6i7h 5a4a 5i6i 5c5d 5g5f 6a5b 3g3f 7c7d 4i5h 4b3c 8h7i 2b3a 1g1f 1c1d
*/
void position(Position& pos, istringstream& is) {

#ifdef SENNICHI
	//千日手対策の情報vectorを初期化
	pos.reputaion_infos.clear();
#endif

	string token,sfen="sfen ";
	is >> token;

	//ここまでは初期局面設定

	if (token == "startpos") { pos.set_hirate(); }
	else if (token == "sfen") {

		while (is >> token) {
			if (token == "moves") { break; }
			sfen += token + " ";
		}
		pos.set(sfen);
	}

	
	Move m;
	int ply=0;
	//ここからは初期局面からの移動
	while (is >> token) {

		if (token == "moves") { continue; }
		//まあ変な指し手入ってくることないやろ
		m = Sfen2Move(token, pos);
		pos.do_move(m, &g_st[ply]);
		ply++;
		pos.ply_from_startpos++;
#ifdef SENNICHI
		//千日手対策
		/*uint8_t checkdside;
		if (pos.is_incheck()) { checkdside = (uint8_t)(pos.sidetomove()) + 1; }
		else { checkdside = 0; }*/
#if 0
		//毎回コンストラクタを呼ぶのは遅いと考えられるので没にする
		struct Matcher {
			Key v;
			Matcher(Key v_ = 0) : v(v_) {}
			bool operator()(const ReputationInfo &a) const { return v == a.key; }
		};
		const Key posKey = pos.key();
		auto fp = std::find_if(pos.reputaion_infos.begin(), pos.reputaion_infos.end(),Matcher(posKey));
		if (fp != pos.reputaion_infos.end()) {
			fp->count_up();
		}
		else {
			
			ReputationInfo ri(posKey, checkdside);
			pos.reputaion_infos.push_back(ri);
		}
#else
		bool find = false;
		for (ReputationInfo& a : pos.reputaion_infos) {
			if (a.key == pos.key()) { a.count_up(); find = true; break; }
		}
		if (find == false) {
			ReputationInfo ri(pos.key()/*, checkdside*/);
			pos.reputaion_infos.push_back(ri);
		}
#endif

#endif
	}


#ifdef SENNICHI

	/*for (auto a : pos.reputaion_infos) {
		cout << a << endl;
	}
	cout << "repsize:" << pos.reputaion_infos.size() << endl;*/
	//出てきた回数が少ないものは消す
	//サイズはどんどん変わっていくのでiで指定するのは良くない....
	//http://d.hatena.ne.jp/unk_pizza/20140426/p1　参照 この方法はかっこいいな
	if (pos.reputaion_infos.size() != 0) {
		auto tail_itr = std::remove_if(pos.reputaion_infos.begin(), pos.reputaion_infos.end(), [](ReputationInfo ri) {return (ri.count < 3); });
		pos.reputaion_infos.erase(tail_itr, pos.reputaion_infos.end());
	}
	//OK

	/*
	for (auto a : pos.reputaion_infos) {
		cout << a << endl;
	}
	cout << "repsize:" << pos.reputaion_infos.size() << endl;
	*/

#endif

	
}



void USI::loop()
{
	Position pos;
	/*Thread th;
	th.cleartable();*/
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
			cout << self_introduction() << endl;
			cout << Options << endl;
			cout << "usiok" << endl;
		}
		else if (token == "isready") { is_ready(); /*search_clear(th);*/  cout << "readyok" << endl; }
		else if (token == "position") { position(pos, is); }
		else if (token == "go") { go(pos, is/*, th*/); }
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
			if (type == "check") {
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
#ifndef LEARN
		else if (token == "ponderhit") {

			limit.is_inponder = false;
			if (signals.stopOnPonderHit) {
				signals.stop = true;
				Threads.main()->start_searching(true);
			}
		}
		else if (token == "stop"||token=="gameover") {
			signals.stop = true;
			limit.is_inponder = false;
			Threads.main()->start_searching(true);
		}
#endif
		//学習用コマンド”アイ”ではなく"エル"
		else if (token == "l") {

#ifdef LEARN
			string yn;
			cout << "do you really wanna learning fv? [y/n]  " ;
			cin >> yn;
			if (yn != "y") { cout << "OK I do not  learning"; break; }
			//Eval::learner();
			Eval::parallel_learner();
#endif
#ifndef LEARN
			cout << "not learning mode" << endl;
#endif
		}
		//====================
		//ここから下はデバッグ用コマンド
		//====================
#if 1
		//else if (token == "gm") {
		//	//指し手生成速度計測
		//	speed_genmove(pos);
		//}
		
		else if (token == "maturi") {
			pos.set(maturi);
		}
		else if (token == "max") {
			pos.set(max_pos);
		}
#ifdef  LEARN


#ifdef MAKESTARTPOS
		else if (token == "rsp") {
			//pos.random_startpos();
			make_startpos_detabase();
		}
#endif
#ifdef MAKETEACHER
		else if (token == "mt") {
			make_teacher();
		}
#endif
#ifdef REIN
		/*else if (token == "readt") {
			read_teacherdata();
		}*/
		else if (token == "rein") {
			string yn;
			cout << "do you really wanna learning fv? [y/n]  ";
			cin >> yn;
			if (yn != "y") { cout << "OK I do not  learning"; break; }
			reinforce_learn();
		}
#endif
#if defined(REIN) || defined(MAKETEACHER)
		else if (token == "hafft") {
			check_teacherdata();
		}
#endif
#endif //  LEARN
#ifdef MISC
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
			/*pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1");
			bool uchihu= pos.is_uchihu(BLACK, SQ5E);
			cout << uchihu << endl;*/

			pos.set("sfen lnG1kgRnl/7b1/pppp+Bpppp/9/9/9/PPPP1PPPP/7R1/LNSGKGSNL b 2S2P 1");
			bool buchihu = pos.is_uchihu(BLACK, SQ5B);
			cout << buchihu << endl;

			pos.set("sfen lnGskgRnl/7b1/pppp+Bpppp/9/9/9/PPPP1PPPP/7R1/LNSGKGSNL b S2P 1");
			 buchihu = pos.is_uchihu(BLACK, SQ5B);
			cout << buchihu << endl;

		}
		else if (token=="suicide") {
			pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL w P 1");
			Move m = make_move(SQ5D, SQ5E, W_KING);
			bool is_suicide = pos.is_king_suiside(WHITE, move_to(m),move_from(m));
			cout << (is_suicide)<< endl;
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
		else if (token == "random") {
			wrap_randomwalker();
		}
		else if (token=="nyugyoku") {
			/*
			position sfen l1sgkg2l/7K1/+S+NP1+B+Rppp/1pp6/p8/9/PP1PPPPPP/9/LNSG1GSNL b RBN3P 1
			position sfen K3kg2+L/1+S+N4+RG/+S+N+P3+Bp+L/1pp6/p8/9/PP1PPPPPP/9/LNSG1GSNL b RB5P 1
			position sfen K3k3+L/1+S+N4+RG/+S+N+P+B3p+L/1pp6/p8/9/PP1PPPPPP/9/LNSG1GSNL b RBG5P 201
			*/
			cout << pos.is_nyugyoku() << endl;
		}
		else if (token == "qc") {
			cout << pos << endl;
			ExtMove moves_[600], *end;
			end = moves_;
			end = test_quietcheck(pos, moves_);
			for (ExtMove* i = moves_; i < end; i++) {
				check_move(i->move);
				
			}
			
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
		else if (token == "mcheck") {
			Move m;
			string ms;
			is >> ms;
			m = Move(stoi(ms));
			check_move(m);

		}
		else if (token == "debug1") {
			pos.set(debug1);
			cout << pos << endl;
			pos.check_longeffect();
		}
		else if (token == "debug2") {
			pos.set(debug2);
			cout << pos << endl;
			pos.check_longeffect();
		}
		//sfenの生成
		else if (token == "sfen") {
			cout << pos.make_sfen() << endl;
		}
		else if(token=="xor"){
			int sq;
			is >> sq;
			cout << (pos.occ_all() ^= SquareBB[sq]) << endl;
			cout << (pos.occ_all() ^ SquareBB[hihumin_eye(Square(sq))]) << endl;
		}
		else if (token == "and") {
			int sq;
			is >> sq;
			cout << (pos.occ_all() & SquareBB[sq]) << endl;
			//cout << (pos.occ_all() &= SquareBB[hihumin_eye(Square(sq))]) << endl;
		}
		else if (token == "andnot") {
			int sq;
			is >> sq;
			//cout << (pos.occ_all().andnot(SquareBB[sq])) << endl;
			cout << (pos.occ_all()&~(SquareBB[sq])) << endl;
			//cout << (pos.occ_all() ^ SquareBB[hihumin_eye(Square(sq))]) << endl;
		}
		else if (token == "or") {
			int sq;
			is >> sq;
			cout << (pos.occ_all() |= SquareBB[sq]) << endl;
			cout << (pos.occ_all() | SquareBB[hihumin_eye(Square(sq))]) << endl;
		}
		else if(token=="not"){
			cout << (~pos.occ_all()) << endl;
			//cout << (pos.occ_all() ^ SquareBB[hihumin_eye(Square(sq))]) << endl;
		}
		else if (token == "islegal") {
			string smove;
			Move m;
			is >> smove;
			m = Sfen2Move(smove, pos);
			cout << pos.is_legal(m) << endl;
		}
		else if (token == "nihu") {
			pos.set(nuhu);
			cout << pos << endl;
			cout<<pos.pawnbb(BLACK)<<endl;
			cout<<pos.pawnbb(WHITE)<<endl;
		}
		else if (token == "m") {

			cout << pos << endl;

			ExtMove moves_[600];
			ExtMove *end = moves_;
			

			if (pos.is_incheck()) {
				end = moves_;
				end = move_eversion(pos, moves_);
			}
			else {
				end = moves_;
				end = move_generation<Cap_Propawn>(pos, moves_);
				end = move_generation<Quiet>(pos, end);
				end = move_generation<Drop>(pos, end);
			}
			
			const ptrdiff_t count = end - moves_;
			std::cout << "num of moves = " << count << std::endl;
			for (int i = 0; i < count; ++i) {
				std::cout << moves_[i].move << ", " << " islegal:" << pos.is_legal(moves_[i].move);
				check_move(moves_[i].move);
				//cout<< " islegal:"<<pos.is_legal(moves_[i].move);
			}
			std::cout << std::endl;

		}
		else if (token == "sym") {


			for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
				cout << sq << " sym " << sym_rl_sq(sq)<<endl;
			}

			auto elist = pos.evallist();

			for (int i = 0; i < 40; i++) {
				cout <<" elist"<<endl<< elist.bplist_fb[i] << endl;
				//cout << "sym " <<endl<< Eval::sym_rightleft(elist.bplist_fb[i]) << endl;
			}
		}
		else if (token == "null") {
			cout << pos << endl;
			StateInfo si;
			pos.do_nullmove(&si);
			cout << pos << endl;
			pos.undo_nullmove();
			cout << pos << endl;
		}
		else if (token == "ce") {
			pos.check_effectocc256();
		}
		else if (token == "ce2") {
			cout << pos << endl;
			pos.check_longeffect256();
		}
		else if(token=="see"){
			/*pos.set("sfen lnsgkg1nl/1r4sb1/ppppppppp/7P1/9/2P6/PP1PPPP1P/1B5R1/LNSGKGSNL b - 1");
			const Move m = make_move(SQ2D, SQ2C, PAWN);*/
			/*pos.set("sfen lnsgkg1nl/1r4sb1/ppppppppp/6P2/7N1/2P4P1/PP1PPP2P/1B4R2/LNSGKGS1L b - 1");
			const Move m = make_move(SQ3D, SQ3C, PAWN);*/
			/*pos.set("sfen 3k4r/2b3+N2/4p1Ss1/2PN4G/lP1p1Ppp1/L3GbPPP/p+pNPG1N2/2LRS4/PgSK2+p1L w 3p 1");
			Move m = make_move(SQ8I, SQ7I, W_GOLD);*/

			pos.set("sfen pn1r2+S+N1/2r+N1k1Gs/l1g4p1/Ppps1pP1l/6pPP/lP2P2bp/2P3+bG1/+p2P1P1+l1/+pN3K1g1 b s 1");
			Move m = make_movepromote(SQ3D, SQ3C, PAWN);
			cout << pos << endl;
			check_move(m);
			/*cout << "see sign" << pos.see_sign(m) << endl;
			cout << "see "<<pos.see(m) << endl;*/
			cout << "seege:" << pos.see_ge(m, Value(0)) << endl;
		}
		else if (token == "occ") {
			pos.check_occbitboard();
		}
#ifdef REIN
		else if (token == "psfen") {
			pos.pack_haffman_sfen();
		}
#endif
		else if (token=="mate") {
			cout << pos << endl << endl;
			cout << pos.mate1ply() << endl;;
		}
#ifdef HAVE_AVX2
		else if (token=="avx") {
			Value avx = Eval::eval_allPP_AVX2(pos);
			Value v = Eval::eval_PP(pos);
			cout << "avx:" << avx << " eval:" << v << endl;
		}
#endif
		else if (token == "cstm") {
			pos.change_stm();
		}
		else if (token == "bplist") {
			pos.evallist().print_bplist();
		}
		else if (token == "list") {
			pos.check_bplist();
		}
		else if (token == "prog") {
//			cout<<fixed<<Progress::prog_scale*Progress::calc_prog(pos)<<endl;
		}
		else if (token == "pl") {
#ifdef  Prog_LEARN
			string yn;
			cout << "do you really wanna learning prog? [y/n]  ";
			cin >> yn;
			if (yn != "y") { cout << "OK I do not  learning"; break; }
			//Eval::learner();
			Progress::learner();
#endif
#ifndef  Prog_LEARN
			cout << "not learning mode" << endl;
#endif
		}
#if defined(REIN) || defined(MAKETEACHER)
		else if (token == "haff") {
			bool haff[256];
			pos.pack_haffman_sfen();
			memcpy(haff, pos.packed_sfen, sizeof(haff));
			Position pos_haffman;
			pos_haffman.unpack_haffman_sfen(haff);
			ASSERT(pos_haffman == pos);

		}
#endif
#ifdef LEARN
		else if (token == "ij") {
			Eval::param_sym_ij();
		}
#endif
#if defined(MAKEBOOK)
		else if (token == "makebook") {

			string yn;
			cout << "do you really wanna makebook? [y/n]  ";
			cin >> yn;
			if (yn != "y") { cout << "OK I do not makebook"; break; }
			BOOK::makebook();
		}
#endif
#endif
#endif //MISC
	} while (token != "quit");




}
