#include "book.h"

#include <sstream>
#include <fstream>

//datastream����book�̍쐬���s���B

//��̋ǖʂɑ΂���bookentry�͑������肤��̂�vector�ɂ��Ă����B
std::map<std::string, std::vector<BookEntry>> book;

bool BookDataStream::preparebook() {

	Position pos;

	std::string line;
	std::string sfen;

	vector<BookEntry> bookentrys;

	while (true) {
		//1�s�ǂݍ���
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

	}//���[�܂Ńt�@�C����ǂݍ���


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
��Ղ��쐬���邽�߂̋@�\

���̃\�t�g�̊��͂ł̓\�t�g�ɍl�������Ē�Ղ��쐬����͍̂���ł���ƍl�����邽�߁Afloodgate�̊�������쐬����B

���̑O�̑��ł͒�Ղ��g��Ȃ��̂����s�ł�������
�܂Ӓ�Ղ̌����������肵�Ē�Ս쐬�͂����ق��������Ǝv����B
�㔼�Ɏ��Ԃ��������邽�߂ɂ�....


��Ղ̌`���͌��݈�ʓI�ɗp�����Ă����˂��牤�t�H�[�}�b�g�B
http://yaneuraou.yaneu.com/2016/02/05/%E5%B0%86%E6%A3%8B%E3%82%BD%E3%83%95%E3%83%88%E7%94%A8%E3%81%AE%E6%A8%99%E6%BA%96%E5%AE%9A%E8%B7%A1%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E3%83%95%E3%82%A9%E3%83%BC%E3%83%9E%E3%83%83%E3%83%88%E3%81%AE/
�i��̃����N�ɏ���Ă���̂͊g���O�̃t�H�[�}�b�g�j

�i�����������������獡��Squirrel��sfen()�ł́@������̏o�͂̏��Ԃ�����Ē�Ղ��g���Ȃ���������Ȃ��̂ŁA���������ł����sfen()���C�����Ă��K�v������j


��˂��牤�t�H�[�}�b�g

sfen ln1gk1snl/1r1s2gb1/p1pppppp1/1p6p/7P1/2P3P2/PP1PPP2P/1B5R1/LNSGKGSNL b - 9
6i7h 8d8e 0 32 62
3i4h 8d8e 0 32 17
3i3h 8d8e 0 32 2

(sfen������)
(���̋ǖʂł̎w����) (�\�z����鉞��i�Ȃ����"none"�j) (���̎w����Ői�߂��Ƃ��̕]���l) (�]���l���o�����Ƃ��̒T���[��)�@�i�o���p�x�j

*/
#if defined(MAKEBOOK)
#include "evaluate.h"
#include "learner.h"
#include "makemove.h"
bool BOOK::makebook() {


	std::vector<Game> games;


	//�ݒ�
	int readgames = 35000;//�ǂݍ��ފ����̐��@�ő�ł�31000��

	int maxply = 40;//�w�K�̏����ǖʗp��60��܂ł̂��p�ӂ��Ă���


	ifstream gamedata(gamedatabasefile);
	

	cout << "readgames ";
	//cin >> readgames;

	//������ǂݍ���
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
	//������
	//====================
	book.clear();
	Thread th;
	Position pos;
	//th.cleartable();
	StateInfo si[500];
	//��Ղ��쐬����B
	//����ɍ쐬���������o���񐔂����������Ȃ��Ă��܂��̂�h�����ߕ���ɂ��Ȃ��B
	
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

			//��������
			if (game.result == int(pos.sidetomove() + 1)) {

				//pos.do_move(m, &si[ply]);
				////book�g�p����ponder���悤�Ƃ���Ə�������ςȂ̂ł��Ȃ�
				////if (!pos.is_legal(ponderm) || !pos.pseudo_legal(ponderm)) { goto NEXTGAME; }
				//pos.undo_move();

				//winside����݂̂ɂ��ׂ�OR��Ս쐬�ɗ͂�������Ă���2017�N�̊����݂̂��璊�o���ׂ��H�H

				//��΂�����������Ă��̂ł���͕K�v����
				//��������������o�^����ꍇ�͂������Ďז��ȉ\��������
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
				//�����Uvector���Ȃ񂩂Ɋi�[���Ă����čŌ�Ɉ�C�ɒǉ����ׂ������i�r���ň������ꂾ�������Ƃ��������邱�Ƃ�����͂��Ȃ̂�)
				BookEntry be(m, MOVE_NONE, Value(0), Depth(0), 1);

				auto bookentry = book.find(sfen);

				if (bookentry != book.end()) {
					bool find = false;

					std::vector<BookEntry>& entrys = bookentry->second;
					//���łɂ��̋ǖʂ��̎w���肪�o�^����Ă���Ώo���񐔂�ǉ�����B
					for (int i = 0; i < entrys.size(); i++) {

						if (entrys[i].move == m
							//&&entrys[i].counter == ponderm
							) {
							entrys[i].frequency++;
							find = true;
						}

					}
					//�����łȂ���ΐV����������Ƃ��Ēǉ�
					if (find != true) { book[sfen].push_back(be); }
					//�o�鏇�Ń\�[�g
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



	//������book�������o��
	write_book("C:/book2/newbook.db");

	cout << "finish" << endl;
	return true;
}

bool BOOK::write_book(string filename)
{
	ofstream of(filename);

	auto i = book.begin();

	//����i++�ł����邩��????
	for (; i != book.end(); i++) {

		/*
sfen ln1gk1snl/1r1s2gb1/p1pppppp1/1p6p/7P1/2P3P2/PP1PPP2P/1B5R1/LNSGKGSNL b - 9
6i7h 8d8e 0 32 62
3i4h 8d8e 0 32 17
3i3h 8d8e 0 32 2

(sfen������)
(���̋ǖʂł̎w����) (�\�z����鉞��i�Ȃ����"none"�j) (���̎w����Ői�߂��Ƃ��̕]���l) (�]���l���o�����Ƃ��̒T���[��)�@�i�o���p�x�j
		book��ponder������͖̂��Ȃ̂ł��������none�ɂ��Ă���

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
	//Node* p_nextnode = nullptr;//vector���|�C���^�ŊǗ����悤�Ƃ���Ƃ��������Ȃ�I������randomforest�ł�index�ŊǗ����Ă��̂�...
	int nextindex = -1;
	Value score=Value_Zero;//���̎���w������̕]���l
	int numThisArmTried = 0;//���̎肪�W�J���ꂽ��

	double winrate() {
		return win_sig(score);
	}

	ucbMove(double ucb_, Move m_) :ucb(ucb_), move(m_) {};
	ucbMove() {};
	ucbMove(Move m_) :move(m_) {};
};

// ucbMove�̕��בւ����s�Ȃ��̂Ŕ�r�I�y���[�^�[���`���Ă����B
inline bool operator<(const ucbMove& first, const ucbMove& second) {
	return first.ucb > second.ucb;
}
inline bool operator>(const ucbMove& first, const ucbMove& second) {
	return first.ucb < second.ucb;
}

inline std::ostream& operator<<(std::ostream& os, ucbMove m) { os << m.move << " ucb(" << m.ucb << ") score("<<m.score<<")"; return os; }


struct Node
{
	vector<ucbMove> moves;//���̋ǖʂɂ����鍇�@��
	bool isleaf = true;//���̋ǖʂ�leaf node��
	//ucbMove* lastMove = nullptr;
	Move lastmove;
	//Node* previous = nullptr;//�ȑO�̋ǖ�
	int previousindex = -1;
	string sfen_;//���̋ǖʂ�sfen������
	int depth=0;//���̋ǖʂ̐[��
	int allplayoutnum=0;//���̃m�[�h�̎�𒲂ׂ���
	int nodeindex = -1;


	//moves���X�g�̏�����
	void make_movesvector(const Position& pos) {
		//move�͂������񐶐�����邪�����͈����肪�����̂ŒT�����sort���ď��5�肮�炢�ɂ܂ōi���Ă��������낤
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

	//moves���X�g�̃\�[�g�ƃ��T�C�Y
	void sort_resize_moves() {
		//�����ő傫���ق��ɕ��ёւ�����B
		std::stable_sort(moves.begin(), moves.end(), [](const ucbMove& m1, const ucbMove& m2) {return m1.score > m2.score; });
		//�傫���ق�����6�肾���c��
		if (moves.size() < 6) return;

		moves.resize(6);


		//�񕪒T���ŕ]���l��臒l�ȉ��ɂȂ��Ă��܂��ꏊ��T��
		//int lb = 0,ub=moves.size();
		//int mid;
		//int threshold = -200;
		//while (ub - lb > 1) {
		//	mid = (ub + lb) / 2;
		//	if (moves[mid].score> threshold) {
		//		//���͈̔͂�[mid,ub]�ɍi����
		//		lb = mid;
		//	}
		//	else {
		//		ub = mid;
		//	}
		//}
		////�]���l��臒l�ȉ��̒l�����X�g�������
		//moves.resize(lb);

	}


};

inline std::ostream& operator<<(std::ostream& os, Node n) { os <<"node:move[0]"<< n.moves[0]; return os; }


#include <random>



//ucb���ő�ƂȂ閖�[�ǖʂ܂ňړ�����
void movebestUcbPos(Position& pos, int index) {

	ucbMove* bestucbmove = nullptr;
	Value v;
	double maxucb = -1145148101919, ucb;
	StateInfo si;

	//ucb���ő�ƂȂ鍷�����T��
	Node* node = &Nodes[index];
	for (ucbMove& move : node->moves) {
		//ucb = move.ucb;
		calc_ucb(win_sig(v), node->moves[i].numThisArmTried, node->allplayoutnum);

		if (ucb > maxucb) {
			maxucb = ucb;
			bestucbmove = &move;
		}
	}
	//���̂悤�Ȃ��Ƃ������Ƃ�ucb�̒l���o�邽�тɂ��̒l�Ń\�[�g����΂�낵��
	//sort���Ă��܂���pointer�ł����̂ڂ�Ȃ��I
	//bestucbmove = &node->moves[0];
	ASSERT(bestucbmove != nullptr);

	pos.set(node->sfen_);


	//if (bestucbmove->p_nextnode == nullptr) {
	if (bestucbmove->nextindex == -1) {
		//�����Ŗ��[�ǖʂɈړ�����
		pos.do_move(bestucbmove->move, &si);

		//������Node������Ă���
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

	//root�ɂ��Ă̏���

	Node root;
	Value v;
	Move m;
	StateInfo si;


	root.depth = 1;
	root.sfen_ = pos.make_sfen();
	root.make_movesvector(pos);
	root.allplayoutnum=1;
	//������rootmove�̏���
	for (int i = 0; i < root.moves.size(); i++) {

		si.clear();
		pos.do_move(root.moves[i].move, &si);//���̎���w�������̕]���l���v�Z

		th.set(pos);
		th.l_depth = 11;
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		v = -th.think();//���肩�猩���]���l�ɂȂ��Ă�I
		if (v == -Value_Infinite) { continue; }
		root.moves[i].score = v;
		root.moves[i].numThisArmTried += 1;
		root.moves[i].ucb = calc_ucb(win_sig(v), root.moves[i].numThisArmTried, root.allplayoutnum);
		root.moves[i].nextindex =-1;
		root.nodeindex = 0;
		std::cout << root.moves[i] << endl;

		pos.undo_move();
	}
	//���ׂĂ̍�����ɂ��ĕ]���l���v�Z���ꂽ
	root.sort_resize_moves();
	std::cout << "sorted" << endl;
	for (ucbMove m : root.moves) { std::cout << m << endl; }

	Nodes.emplace_back(root);

	double bestucb;
	while (true) {
		//���[�܂ňړ�
		movebestUcbPos(pos,0);
		Node* node = &Nodes[Nodes.size() - 1];
		bestucb = -114514810;
		//������rootmove�̏���
		node->allplayoutnum++;
		pos.set(node->sfen_);
		std::cout << pos << endl;

		for (int i = 0; i < node->moves.size(); i++) {

			si.clear();
			cout << node->moves[i].move << endl;
			pos.do_move(node->moves[i].move, &si);//���̎���w�������̕]���l���v�Z

			th.set(pos);
			th.l_depth = 11;
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			v = -th.think();//���肩�猩���]���l�ɂȂ��Ă�I
			if (v == -Value_Infinite) { continue; }
			node->moves[i].score = v;
			node->moves[i].numThisArmTried += 1;
			node->moves[i].ucb = calc_ucb(win_sig(v), node->moves[i].numThisArmTried, node->allplayoutnum);



			//std::cout << node->moves[i] << endl;

			pos.undo_move();
		}
		node->sort_resize_moves();
		for (ucbMove m : node->moves) { std::cout << m << endl; }

		//�����Ŗ؂̍��Ɍ������ăf�[�^�����X�V���Ă���




		std::cout << "a";

	}






	std::cout << "test" << endl;
}

//�]���l�̒l���t�`������
void BackPropagationScore(Node* nownode, Value score,Move m) {

}



Value makemoveloop(Position& pos, Node& node,mt19937& mt,Thread& th) {

	StateInfo si;
	ucbMove* bestucbmove=nullptr;
	Value v;
	double maxucb = -1145148101919, ucb;
	double ln_allnumplayout = std::log(node.allplayoutnum);

	//������ucb���ő�ƂȂ鍷�����T��
	for (ucbMove& move : node.moves) {

		//�܂�������Ă��Ȃ�������
		if (move.numThisArmTried == 0) {
			ucb = 1000 + mt() % 100;

		}
		else {
			//ucb�̌v�Z
			ucb = calc_ucb(move.winrate(), move.numThisArmTried, node.allplayoutnum);
		}

		if (ucb > maxucb) {
			maxucb = ucb;
			bestucbmove = &move;
		}
	}
	ASSERT(bestucbmove != nullptr);
	//best�Ȏ肪��������


	//best�ȍ����肪���[�ł���Ύ��̃m�[�h���쐬����
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