#include "reinforce_learner.h"
#include "position.h"
#include "Thread.h"
#include "progress.h"
#include "makemove.h"
#include <random>
#include <sstream>
#include <fstream>
#include <omp.h>
/*
ここではランダムに初期局面を用意しsfen文字列に変換し、ファイルに書き出す。
packedsfenのほうがいいかもしれないがまずはsfenで作成する

ランダムといっても
ある程度のルールは必要だろう

まず王手がかかっていないこと
その局面の評価値が500を超えないこと（もしくは進行度を使って序盤であることを確認してもいいかも）



...まあこれぐらいか？？

やっぱり初期局面なので自陣から見て4段目までなどという制限が必要か...
初期局面に成りを用意すべきか..??
う～～ん初期局面になりを入れると初期局面の評価値が大きくなりすぎてしまうような気がするので
初期局面になりを入れることはしないでおく


そうか2歩も防がないといけないな....

持ち駒はないほうがいいか..


評価関数がよくないからか,あんまり質のいい開始局面は生成できなかった。
depth2では評価値100以内だが他では1000超えてしまうみたいな...
*/


#ifdef MAKETEACHER
string Position::random_startpos()
{
	clear();
	std::random_device seed_gen;
	std::default_random_engine engine(seed_gen());
	std::uniform_int_distribution<int> rand_color(BLACK, WHITE);
	std::uniform_int_distribution<int> is_hand_piece(0,1);//9で手ごま
	std::uniform_int_distribution<int> rand_file(0, 8);
	std::uniform_int_distribution<int> rand_rank(0, 3);
	std::uniform_int_distribution<int> rand_king(SQ_ZERO, SQ_NUM-1);
	string sfen;
	Thread th;

	int count = 0;
	//pawn
	for (int i = 0; i < 9; i++) {
		
		const Color c = BLACK;
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], PAWN, num_pt(hands[c], PAWN) + 1); count++; break; }
			else {

				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);

				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq] & ExistPawnBB[c]).isNot()) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(PAWN, c);
					put_piece(c, PAWN, sq);
					set_occ256(sq);
					ExistPawnBB[c] |= FileBB[sqtofile(sq)];
					count++;
					break;
				}
				
			}
		}
	}
	for (int i = 0; i < 9; i++) {

		const Color c = WHITE;
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], PAWN, num_pt(hands[c], PAWN) + 1); count++; break; }
			else {

				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);

				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq] & ExistPawnBB[c]).isNot()) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(PAWN, c);
					put_piece(c, PAWN, sq);
					set_occ256(sq);
					ExistPawnBB[c] |= FileBB[sqtofile(sq)];
					count++;
					break;
				}

			}
		}
	}
	//lance
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], LANCE, num_pt(hands[c], LANCE) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				if (c == BLACK ? sqtorank(sq) == RankA : sqtorank(sq) == RankI) { continue; }
				else {
					pcboard[sq] = add_color(LANCE, c);
					put_piece(c, LANCE, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//knight
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], KNIGHT, num_pt(hands[c], KNIGHT) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				if ((SquareBB[sq]&CantGo_KNIGHT[c]).isNot()) { continue; }
				else {
					pcboard[sq] = add_color(KNIGHT, c);
					put_piece(c, KNIGHT, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//silver
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], SILVER, num_pt(hands[c], SILVER) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(SILVER, c);
					put_piece(c, SILVER, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}

	//gold
	for (int i = 0; i < 4; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], GOLD, num_pt(hands[c], GOLD) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(GOLD, c);
					put_piece(c, GOLD, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//bishop
	for (int i = 0; i < 2; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], BISHOP, num_pt(hands[c], BISHOP) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(BISHOP, c);
					put_piece(c, BISHOP, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//rook
	for (int i = 0; i < 2; i++) {

		const Color c = (Color)rand_color(engine);
		while (true) {
			const int h = is_hand_piece(engine);
			if (h == 9) { makehand(hands[c], ROOK, num_pt(hands[c], ROOK) + 1); count++; break; }
			else {
				const File f = (File)rand_file(engine);
				const Rank r = (c == BLACK) ? (Rank)(8 - rand_rank(engine)) : (Rank)rand_rank(engine);
				const Square sq = make_square(r, f);
				if (piece_on(sq) != NO_PIECE) { continue; }
				else {
					pcboard[sq] = add_color(ROOK, c);
					put_piece(c, ROOK, sq);
					set_occ256(sq);
					count++;
					break;
				}

			}
		}
	}
	//先手玉
	while (true)
	{
		const File f = (File)rand_file(engine);
		const Rank r = (Rank)(8 - rand_rank(engine));
		const Square ksq = make_square(r, f);
		if(piece_on(ksq) != NO_PIECE) { continue; }
		if (attackers_to(WHITE, ksq, occ256).isNot()) { continue; }//相手の効きが効いていてはいけない
		pcboard[ksq] = add_color(KING, BLACK);
		put_piece(BLACK, KING, ksq);
		set_occ256(ksq);
		count++;
		break;
	}
	while (true)
	{
		const File f = (File)rand_file(engine);
		const Rank r = (Rank)rand_rank(engine);
		const Square ksq = make_square(r, f);
		if (piece_on(ksq) != NO_PIECE) { continue; }
		if (attackers_to(BLACK, ksq, occ256).isNot()) { continue; }//相手の効きが効いていてはいけない
		pcboard[ksq] = add_color(KING, WHITE);
		put_piece(WHITE, KING, ksq);
		set_occ256(ksq);
		count++;
		break;
	}
	ASSERT(count == 40);
	ASSERT(is_incheck() == false);
	/*cout << *this << endl;
	cout << count << endl;*/
	sfen = make_sfen();
	set(sfen);
	
	th.set(*this);
	th.l_depth = 10;
	th.l_beta = (Value)101;
	th.l_alpha = (Value)-101;
	Value v = th.think();
	//あまり評価値が離れていると初期局面として不適切と考えられる。
	if (abs(v) > 100/* Progress::prog_scale*Progress::calc_prog(*this)>2000*/) {
		return to_string(0);
	}
	return sfen;
}




void make_startpos_detabase()
{
	Position pos;
	vector<string> db;

	int64_t num;
	cout << "num:";
	cin >> num;
	int64_t i = 0;
	string str;
	
	//db作成。
	while (true) {
		str = pos.random_startpos();

		for each (string s in db){if (str == s) { goto NEXT; }}//同じ局面があれば再登録はしない
		if (str.size() < 3) { continue; }
		else {
			db.push_back(str);
			i++;
		}
		if (i % 1000 == 0) { cout << i << endl; }
		if (i % 2000 == 0) { 
			
			ofstream of1("C:/sp_db/startpos.db");
			for (int j = 0; j < db.size(); j++) {

				of1 << db[j] << endl;
			}
			of1.close();
		}
		if (i > num) { break; }

	NEXT:;
	}

	//ファイルに書き出し
	
	ofstream of("C:/sp_db/startpos.db");

	for (int j = 0; j < db.size(); j++) {
		
		of << db[j] << endl;
	}
	of.close();
	cout << "finish make database!" << endl;
}

#endif
#if defined(LEARN) && defined(MAKETEACHER)

struct teacher_data {

	bool haffman[256];
	Value teacher_value;
	//teacher_data() {};
	teacher_data(const bool *haff, Value teachervalue) {
		memcpy(haffman, haff, sizeof(haffman));
		teacher_value = teachervalue;
	}
	teacher_data(){}
};
vector<string> startpos_db;
vector<vector<teacher_data>> teachers;
vector<teacher_data> sum_teachers;


std::mutex mutex__;
int index__ = 0;

int lock_index_inclement__() {
	std::unique_lock<std::mutex> lock(mutex__);
	if (index__ > startpos_db.size()) { cout << "o" << endl; }
	else { printf("."); }

	return index__++;
}


int maxthreadnum__;
/*
3手先の評価値と局面の組をセットにした教師データを作成する。
*/
void make_teacher()
{
	int maxnum;
	cout << "maxnum:" << endl;
	cin >> maxnum;

	//開始局面データベース読み込み
	ifstream f("C:/sp_db/startpos.db");
	string s;
	while (!f.eof()) {
		getline(f, s);
		startpos_db.push_back(s);
	}
	f.close();


	//教師データ格納庫用意
	maxthreadnum__ = omp_get_max_threads();
	for (size_t i = 0; i < maxthreadnum__; i++) {
		teachers.emplace_back();
	}

	//開始局面データベースシャッフル 
	std::random_device rd;
	std::mt19937 g_mt(rd());
	//スレッド作成
	vector<std::thread> threads(maxthreadnum__ - 1);
	
	for (int i = 0; i < (maxnum / startpos_db.size())+1; i++) {



		sum_teachers.clear();
		for (size_t i = 0; i < maxthreadnum__; i++) {
			teachers[i].clear();
		}


		std::shuffle(startpos_db.begin(), startpos_db.end(), g_mt);


		//teacherの作成
		index__ = 0;
		for (int i = 0; i < maxthreadnum__ - 1; ++i) {
			threads[i] = std::thread([i] {make_teacher_body(i); });
		}
		make_teacher_body(maxthreadnum__ - 1);


		//thread毎のteacherをmerge
		for each (vector<teacher_data> tdv in teachers)
		{
			std::copy(tdv.begin(), tdv.end(), std::back_inserter(sum_teachers));
		}


		//ふぁいるに書き出し（上書き）
		/*
		読み込むときはvector一つ分とってきて、それをpushbackしていけばいいと考えられるのだが
		*/
		ofstream of("C:/teacher/teacherd2.bin", ios::out | ios::binary|ios::app);
		if (!of) { UNREACHABLE; }
		of.write(reinterpret_cast<const char*>(&sum_teachers[0]), sum_teachers.size() * sizeof(teacher_data));

		of.close();
	}

	for (auto& th : threads) { th.join(); }
	cout << "finish!" << endl;
}

void make_teacher_body(const int number) {

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> isrand(0, 9);
	Position pos;
	StateInfo si[500];
	ExtMove moves[600], *end;
	Thread th;
	end = moves;

	for (int g = lock_index_inclement__(); g < startpos_db.size(); g = lock_index_inclement__()) {
		string startpos = startpos_db[g];
		if (startpos.size() < 10) { continue; }//10は適当
		pos.set(startpos);
		th.cleartable();

		for (int i = 0; i < 256; i++) {


			th.set(pos);
			th.l_depth = 3;
			th.l_beta = (Value)2001;
			th.l_alpha = (Value)-2001;
			Value v = th.think();
			if (abs(v) > 2000) { goto NEXT_STARTPOS; }
			pos.pack_haffman_sfen();
			teacher_data td(pos.packed_sfen, v);
			teachers[number].push_back(td);

			//次の差し手を選別。(合法手が選ばれるまでなんども繰り返す必要がある)
			memset(moves, 0, sizeof(moves));//初期化
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;
			Move m;
			while (true) {
				int a = isrand(rd);

				//40パーセントでpvの差し手。
				if (a < 4) { m = th.pv[0]; }
				//60パーセントでランダム
				else { m= moves[mt() % num_moves];}

				if (pos.is_legal(m) && pos.pseudo_legal(m)) {break;}
			}
			//差し手で次の局面に移動する。
			pos.do_move(m, &si[i]);
			Eval::eval(pos);//差分計算でバグを出さないために計算しておく
		}
	NEXT_STARTPOS:;
	}

}

//ok
//参考　http://gurigumi.s349.xrea.com/programming/binary.html
void read_teacherdata() {

	ifstream f("C:/teacher/teacherd2.bin", ios::in | ios::binary);
	teacher_data t;
	int i = 0;
	while (!f.eof()) {
		f.seekg(i * sizeof(teacher_data));
		f.read((char*)&t, sizeof(teacher_data));

		sum_teachers.push_back(t);
		i++;
	}
	f.close();

	/*Position pos;

	pos.unpack_haffman_sfen(sum_teachers[0].haffman);
	cout << pos << endl;
	cout << pos.occ_all() << endl;*/



}


#endif //  LEARN


