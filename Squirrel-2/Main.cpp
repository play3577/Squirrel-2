#include "fundation.h"
#include "Bitboard.h"
#include "position.h"
#include "makemove.h"
#include "benchmark.h"
#include "usi.h"
#include "learner.h"
#include "evaluate.h"
#include "Hash.h"
#include "tpt.h"
#include "book.h"
#include "occupied_m256.h"
#include "progress.h"

#include <iostream>
#include <string>


using namespace std;

//#define TEST


int main() {

	//‚±‚Ì‹L–@‚ª‹–‚³‚ê‚é‚Ì‚ÍVC++‚¾‚¯‚ç‚µ‚¢‚Ì‚Ålinux‚ÉˆÚA‚·‚éê‡‚Í•Ê‚Ì•û–@‚ğg‚í‚È‚¢‚Æ‚¢‚¯‚È‚¢
	//cout << __argv[0] << endl;

#if defined(_MSC_VER)
	string ename = __argv[0];
	string::size_type hoge = ename.find_last_of('\\');
	if (hoge == string::npos) { hoge = ename.find_last_of('/'); }
	string ename2 = ename.substr(hoge + 1);
#endif
#if defined(__GNUC__) 
	string ename2 = "Squirrel_linux";
#endif

	
	//cout << ename2 << endl;
	//Progress::initialize_KP();


	Progress::read_KP();
	USI::init_option(Options,ename2);
	bitboard_init();
	init_occ256();
	Eval::init();
	Zoblist::init();
	search_init();
	//Eval::initialize_PP();
	/*if (Options["usebook"] == true) {
		BOOK::init();
	}*/
#ifdef USETT
	TT.resize(30);//30MB‚Å‰Šú‰»‚·‚éi‚â‚Ë‚¤‚ç‰¤‚É•í‚Á‚½j
#endif
	//bitboard_debug();

#ifdef TEST
	/*Position pos;*/

	
	/*
	for (Square a = SQ1A; a < SQ_NUM; a++) {

		cout << a << endl;
	}*/
	/*for (Piece pc = NO_PIECE; pc < PC_ALL; pc++) {
		cout << pc << endl;
	}*/
	/*Move normal = make_move(SQ9I, SQ1D, ROOK);
	Move promote = make_movepropawn(SQ9I, SQ1D, ROOK);
	Move drop = make_drop(SQ9I, ROOK);
	check_move(normal);
	check_move(promote);
	check_move(drop);*/

	Hand h = Hand(0b00011);//•à‚ª‚R–‡
	h += Hand(0b100<<6);//‚ª‚S–‡
	h += Hand(0b100 << 9);//Œj”n‚ª4–‡
	h += Hand(0b100 << 12);//‹â‰Í4–‡
	h += Hand(0b10 << 15);//Šp‚ª‚Q–‡
	h += Hand(0b10 << 17);//”òÔ‚ª2–‡
	h += Hand(0b100 << 19);
	cout << h << endl;
	for (Piece pt = PAWN; pt <= GOLD; pt++) {
		makehand(h, pt, 1);
		cout << h << endl;
	}
	
	

	pos.set(sfen_max);
	pos.set(sfen_maturi);

	/*pos.remove_piece(BLACK, PAWN, SQ1G);
	cout << pos << endl;
	pos.put_piece(BLACK, PAWN, SQ1G);
	cout << pos << endl;*/

	"sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
	pos.set("sfen lnsgkgsnl/1r5b1/ppppppppp/P8/9/9/1PPPPPPPP/1B5R1/LNSGKGSNL b - 1");

	

	Move m;
	m = make_move(SQ2G, SQ2F,PAWN);
	//m = make_movepropawn(SQ9D, SQ9C, PAWN);
	pos.do_move(m, &si);
	//cout << pos << endl;
	pos.check_occbitboard();
	pos.undo_move();
	//cout << pos << endl;
	pos.check_occbitboard();

	pos.check_effect();

	pos.set("sfen 1nsgkgsnl/1r5b1/ppppppppp/9/9/9/PP1P+P+PP2/1+B5R1/LNSGKGS1L b P 1");
	pos.set("sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b PLNGSBR 1");

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

	pos.check_occbitboard();
	cout << "movenum " << ptrdiff_t(end - moves_) << endl;


	pos.set("sfen l6nl/5+P1gk/2np1S3/p1p4Pp/3P2Sp1/1PPb2P1P/P5GS1/R8/LN4bKL w GR5pnsg 1");//w‚µè¶¬Õ‚è‚Ì‹Ç–Ê
	cout << pos << endl;
	speed_genmove(pos);

	Eval::initialize_PP();
	Position pos;
	//StateInfo si;

	pos.check_effecttoBB();


#endif
	

	USI::loop();
	return 0;
}