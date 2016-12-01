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

//position�R�}���h�ŋǖʂ��ړ������邽�߂�stateinfo
//�R���Ȃ�position�R�}���h���甲���Ă��܂��Ă�stateinfo�������邱�Ƃ͂Ȃ��͂��ł���
static StateInfo g_st[257];

USI::OptionMap Options;

/*
error position
position startpos moves 2g2f 3c3d 2f2e 2b3c 7g7f 4c4d 5g5f 5c5d 3g3f 3a2b 2i3g 7c7d 5f5e 5d5e 8h5e 8b5b

*/
//2�����w�����IPawnBB�̍����Ƀo�O���L��H�H�H

//�ł����l�߂̋ǖ�
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

//usi�Ŏ��ȏЉ�����邽�߂̊֐�
const string self_introduction() {

	stringstream ss;

	ss << "id name Squirrel_";
#ifdef _DEBUG
	ss << "debug ";
#endif
#ifndef _DEBUG
	ss << "releaseTT ";
#endif
	ss << endl;
	ss << "id author Kotaro Suganuma";
	return ss.str();
}


void USI::init_option(OptionMap &o)
{
	o["USI_Ponder"] << USIOption(false);
	o["Threads"] << USIOption(1, 1, 128);
	o["eval"] << USIOption("c:/book2/fv_PP.bin");
	o["USI_Hash"] << USIOption(1, 1, 256);
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

//isready�ŃZ�b�e�B���O���邽�߂̊֐�
void is_ready() {
	//eval�̃p�X���ύX����Ă��邩������Ȃ����߂������ǂݒ����B
	//�ƌ��������usi�ɕԎ����Ă���read�������ق����������H
	Eval::read_PP();
}

void go(Position& pos, istringstream& is, Thread& th) {

	limit.starttime = now();
	Value v;
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
	}

#ifdef TEST
	cout << limit << endl;
#endif


	th.set(pos);
	v = th.think();
	cout << " �]���l " << v << endl;

}


void position(Position& pos, istringstream& is) {

	/*
	����Ȃ񂪓����Ă���

	position startpos moves 7g7f 8c8d 7i6h 3c3d 6h7g 7a6b 2g2f 3a4b 3i4h 4a3b 6i7h 5a4a 5i6i 5c5d 5g5f 6a5b 3g3f 7c7d 4i5h 4b3c 8h7i 2b3a 1g1f 1c1d
	*/

	string token,sfen="sfen ";
	is >> token;

	//�����܂ł͏����ǖʐݒ�

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
	//��������͏����ǖʂ���̈ړ�
	while (is >> token) {

		if (token == "moves") { continue; }
		//�܂��ςȎw��������Ă��邱�ƂȂ����
		m = Sfen2Move(token, pos);
		pos.do_move(m, &g_st[ply]);
		ply++;
	}


	cout << pos << endl;

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

		istringstream is(cmd);//parse���邽�߂�istringstream
		token.clear();
		is >> skipws >> token;//�Ԃ̋󔒕����͓ǂݔ�΂���token�ɓ����B

		if (token == "usi") {
			cout << self_introduction() << endl;
			cout << Options << endl;
			cout << "usiok" << endl;
		}
		else if (token == "isready") { is_ready(); cout << "readyok" << endl; }
		else if (token == "position") { position(pos, is); }
		else if (token == "go") { go(pos, is, th); }
		else if (token == "setoption") {
/*
>C:setoption name Threads value 2
>C:setoption name Hash value 32
>C:setoption name Ponder value true
>C:setoption name WriteDebugLog value false
>C:setoption name NetworkDelay value 41
>C:quit

����Ȋ�����option�������Ă���I�I�I
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
		//�w�K�p�R�}���h�h�A�C�h�ł͂Ȃ�"�G��"
		else if (token == "l") {

#ifdef LEARN
			string yn;
			cout << "do you really wanna learning fv? [y/n]  " ;
			cin >> yn;
			if (yn != "y") { cout << "OK I do not  learning"; break; }
			Eval::learner();
#endif
#ifndef LEARN
			cout << "not learning mode" << endl;
#endif
		}
		//====================
		//�������牺�̓f�o�b�O�p�R�}���h
		//====================
		else if (token == "gm") {
			//�w���萶�����x�v��
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
			//�ł����l�߂̋ǖ�
			pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL b P 1");
			bool uchihu= pos.is_uchihu(BLACK, SQ5E);
			cout << uchihu << endl;
		}
		else if (token=="suicide") {
			pos.set("sfen ln6n/s2RSR3/ppp1p1ppp/gb2k2bl/g2p1p2s/4G4/PPPP1PPPP/9/LN1GK1SNL w P 1");
			Move m = make_move(SQ5D, SQ5E, W_KING);
			bool is_suicide = pos.is_king_suiside(WHITE, move_to(m),move_from(m));
			cout << (is_suicide)<< endl;
		}
		else if(token=="legal1")
		{
			//false�ɂȂ�͂�
			pos.set("sfen lns1kgsnl/1r1p3b1/ppp1gpppp/9/9/4R4/PPPPPPPPP/1B7/LNSGKGSNL w p 1");
			Move m = make_move(SQ5C, SQ6C,W_GOLD);
			bool legal = pos.is_legal(m);
			cout << legal << endl;
		}
		else if (token == "legal2")
		{
			//true�ɂȂ�͂�
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
		//sfen�̐���
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
				std::cout << moves_[i].move << ", ";
			}
			std::cout << std::endl;

		}
	} while (token != "quit");




}
