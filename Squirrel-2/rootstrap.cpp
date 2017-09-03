#define _CRT_SECURE_NO_WARNINGS


//#define USEPACKEDSFEN
#include "reinforce_learner.h"
#include "position.h"
#include "Thread.h"
#include "makemove.h"
#include <random>
#include <time.h>
#include <omp.h>
#include <fstream>

#if defined(_MSC_VER)
#include <filesystem>
namespace sys = std::tr2::sys;
#endif

#define MAKETEST


#if defined(WINDOWS)

#ifdef MAKETEST
string TEACHERPATH = "C:/teacher";
#else
#ifdef  USEPACKEDSFEN
string TEACHERPATH = "G:/201705260520D8AperyWCSC26";
#else
string TEACHERPATH = "G:/teacher";
#endif
#endif//maketest

#elif defined(LINUX)
string TEACHERPATH = "/home/suganuma/Teacher";
#endif//windows




#define DEPTH 6




#if defined(REIN) || defined(MAKETEACHER)



//==========================================================================


void renewal_PP_rein(dJValue &data);
void renewal_PP_nozomi(dJValue &data);//nozomiさんに教わった方法
//ランダム開始局面から1手ランダムにささせるために使う
void do_randommove(Position& pos, StateInfo* s, std::mt19937& mt);
#endif

#if defined(LEARN) && defined(MAKETEACHER)
/*---------------------------------------------------------------------------
数手先の評価値と局面の組をセットにした教師データを作成するための関数
これは初期化とthreadの取りまとめのための関数であるので
----------------------------------------------------------------------------*/
void Make_Teacher::make_teacher()
{
	int64_t maxnum;
	cout << "maxnum:";
	cin >> maxnum;

#ifdef WINDOWS
	cout << "have G volume?[Y/N]" << endl;
	string haveg;
	cin >> haveg;
	if (haveg == "Y" || haveg == "y") {
		TEACHERPATH[0] = 'G';
	}
#endif
	cout << TEACHERPATH << endl;

	/*
		std::string day;
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		day = asctime(timeinfo);

		size_t hoge;
		while ((hoge = day.find_first_of(" ")) != string::npos) {
			day.erase(hoge, 1);
		}
		while ((hoge = day.find_first_of("\n")) != string::npos) {
			day.erase(hoge, 1);
		}
		while ((hoge = day.find_first_of(":")) != string::npos) {
			day.erase(hoge, 1);
		}
	*/

	//開始局面データベース読み込み
	/*
	出てこないきょくめんばかりだった可能性が大なので定跡から読み込ませる
	*/
	//ifstream f("C:/sp_db/startpos.db");
#if defined(WINDOWS)
	ifstream f("C:/book2/standard_book.db");
#elif defined(LINUX)
	ifstream f("/home/suganuma/FV/BOOK/standard_book.db");
#endif
	string s;
	while (!f.eof()) {
		getline(f, s);
		if (s.find("sfen") != string::npos) { startpos_db.push_back(s); }

	}
	f.close();

	cout << "startposdb_size:" << startpos_db.size() << endl;
	//教師データ格納庫用意

	//GCCで全然うまくいかない..................(´･ω･｀)
	maxthreadnum__ = omp_get_max_threads();
	//maxthreadnum__ = 4;
	for (size_t i = 0; i < maxthreadnum__; i++) {
		teachers.emplace_back();
	}
	cout << "teachers" << teachers.size() << endl;
	//開始局面データベースシャッフル 
	std::random_device rd;
	std::mt19937 g_mt(rd());
	//スレッド作成
	vector<std::thread> threads(maxthreadnum__ - 1);
	cout << "threads" << threads.size() << endl;
	int i = 0;

	for (int i = 0; i < (maxnum / startpos_db.size()) + 2; i++)
		//for (int i = 0; i < 10; i++)
		//while(true)
	{
		//i++;
		cout << "iter:" << i << endl;

		sum_teachers.clear();
		for (size_t h = 0; h < maxthreadnum__; h++) {
			teachers[h].clear();
		}


		std::shuffle(startpos_db.begin(), startpos_db.end(), g_mt);

		cout << "shuffled startpos" << endl;
		//teacherの作成
		index__ = 0;
#define MALTI
#ifdef  MALTI


		//https://msdn.microsoft.com/ja-jp/library/dd293608.aspx
		for (int k = 0; k < maxthreadnum__ - 1; ++k) {
			threads[k] = std::thread([=] {make_teacher_body(k); });
		}
		make_teacher_body(maxthreadnum__ - 1);
#else
		make_teacher_body(0);
#endif 

#ifdef  MALTI
		for (auto& th : threads) { th.join(); }
#endif
		//thread毎のteacherをmerge
		int h = 0;

#ifdef MSVC
		for each (vector<teacher_data> tdv in teachers)
		{
			std::copy(tdv.begin(), tdv.end(), std::back_inserter(sum_teachers));
			teachers[h++].clear();
		}
#else
		for (vector<teacher_data> tdv : teachers)
		{
			std::copy(tdv.begin(), tdv.end(), std::back_inserter(sum_teachers));
			teachers[h++].clear();
		}
#endif
		//なんかシャッフルしないほうがいいとかいう情報もあるのでシャッフルしないほうがいいのか？まあ普通に考えたらminbatchに入っている局面のランダム性が多いほうがいいのでシャッフルすべきだと思うのだが...
		std::printf("shuffle teacherdata\n");
		std::shuffle(sum_teachers.begin(), sum_teachers.end(), g_mt);


		std::printf("write teacher_data\n");
		sumteachersize += sum_teachers.size();
		cout << "writed teacher num:" << sumteachersize << endl;
		//ふぁいるに書き出し（上書き）
		/*
		読み込むときはvector一つ分とってきて、それをpushbackしていけばいいと考えられるのだが
		*/
		string teacher_ = TEACHERPATH + "/" + "iteration5" + "depth" + itos(DEPTH - 1) + ".txt";
		ofstream of(teacher_, ios::app);
		if (!of) { UNREACHABLE; }
		//of.write(reinterpret_cast<const char*>(&sum_teachers[0]), sum_teachers.size() * sizeof(teacher_data));
		for (auto& td : sum_teachers) {
			of << td;
		}
		of.close();
		cout << i << endl;
		//なぜか落ちるので明日の朝チェックをする
	}


	cout << "finish!" << endl;
}

void Make_Teacher::make_teacher_body(const int number) {

	cout << number << endl;
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> random_move_probability(0.0, 1.0);//doubleのほうが扱いやすいか
	double random_probability = 1.0;
	Position pos;
	StateInfo si[500];
	StateInfo s_start;
	ExtMove moves[600], *end;
	Thread th;
	th.cleartable();
	end = moves;

	for (int g = lock_index_inclement__();
#ifdef MAKETEST
		g < 10;
#else
		g < startpos_db.size();
#endif
		g = lock_index_inclement__()) {

		for (int i = 0; i < 500; i++) si[i].clear();
		string startposdb_string = startpos_db[g];

		cout << "index:" << g << endl;

		if (startposdb_string.size() < 10) { continue; }//文字列の長さがありえないほど短いのはエラーであるので使わない
		pos.set(startposdb_string);//random開始局面集から一つ取り出してsetする。（このランダム開始局面は何回も出てくるのでここから一手動かしたほうがいい）
		do_randommove(pos, &s_start, mt);//random初期局面から1手ランダムに進めておく
		th.cleartable();

		random_probability = 1.0;

		for (int i = 0; i < 300; i++) {

			//探索前にthreadを初期化しておく
			Eval::eval_PP(pos);//差分計算でバグを出さないために計算しておく



			th.set(pos);
			//th.cleartable();//毎回やるとおそいがやった方がいいのだろうか？？

			//技巧NDFは深さ固定ではなく秒数固定。
			//秒数固定のほうが同じ差し手にならないのでいいらしい。
			th.l_depth = DEPTH;
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			Value v = th.think();//探索を実行する これは手番側から見た評価値である 読み抜けが激しいかもしれないので枝刈りを抑えた探索を行うべきか？
			if (abs(v) > 3000) { goto NEXT_STARTPOS; }//評価値が3000を超えてしまった場合は次の局面へ移る
			/*if (th.pv.size() > 6) {
				cout << th.pv.size() << endl;
			}*/
			//pos.pack_haffman_sfen();
			//root局面のhaffman符号を用意しておく
		/*	bool HaffmanrootPos[256];
			memcpy(HaffmanrootPos, pos.packed_sfen, sizeof(HaffmanrootPos));*/
			string sfen_rootpos = pos.make_sfen();//root局面のsfen
			//pos.pack_haffman_sfen();
			//------------------------------PVの末端のノードに移ってそこでの静止探索の値を求めteacher_dataに格納
			/*--------------------------------------------------------------------------------------------------------
			pvの末端に移動をしなかった場合deepvalueが探索の値とかけ離れてしまうということが起こった！これで少しはましになる
			--------------------------------------------------------------------------------------------------------*/
			//#define GOLEAF//USETTしてるのでGLEAFはできない
			Value deepvalue = v;
#if defined(GOLEAF)
			StateInfo si2[64];
			for (int i = 0; i < 64; i++) si2[i].clear();
			int pv_depth = 0;
			const Color rootColor = pos.sidetomove();//HaffmanrootPosの手番

			//pos.do_move(th.RootMoves[0].move, &si2[pv_depth++]);
			for (Move m : th.pv) {
				if (pos.is_legal(m) == false || pos.pseudo_legal(m) == false) { ASSERT(0); }
				pos.do_move(m, &si2[pv_depth++]);
			}
			//rootから見た評価値を格納する
#ifdef EVAL_KPP
			pos.state()->sumBKPP = Value_error; pos.state()->previous->sumBKPP = Value_error;
#elif defined(EVAL_PP)
			pos.state()->bpp = pos.state()->wpp = Value_error;//差分計算を無効にしてみる
			pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error;
#elif defined(EVAL_PPT)
			pos.state()->bpp = pos.state()->wpp = Value_error;//差分計算を無効にしてみる
			if (pos.state()->previous != nullptr) { pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error; }
#endif
			deepvalue = (rootColor == pos.sidetomove()) ? Eval::eval(pos) : -Eval::eval(pos);
#endif
			//teacher_data td(/*HaffmanrootPos,*/sfen_rootpos, deepvalue);
#ifdef  USEPACKEDSFEN
			teacher_data td(&pos.data, deepvalue/*,th.RootMoves[0].move*/);
#else
			teacher_data td(sfen_rootpos, deepvalue);
#endif

			teachers[number].push_back(td);
#if  defined(GOLEAF)
			for (int jj = 0; jj < pv_depth; jj++) {
				pos.undo_move();
			}
#endif
			//---------------------------------------------------------------------------
			//次の差し手を選別。(合法手が選ばれるまでなんども繰り返す必要がある)
			memset(moves, 0, sizeof(moves));//初期化
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;
			Move m;



			while (true) {
				//次の局面に進めるための指し手の選択
				//合法手が選択されるまでwhileで回る

				//王手がかかっていない場合、40パーセントでランダムの指し手で次局面に
				if (!pos.is_incheck()
					&& random_move_probability(rd) < random_probability)
				{
					random_probability /= 1.5; m = moves[mt() % num_moves];
				}
				else {
					//60パーセントでpv
					m = th.RootMoves[0].move;
				}

				if (pos.is_legal(m) && pos.pseudo_legal(m)) { break; }
			}
			//差し手で次の局面に移動する。
			pos.do_move(m, &si[i]);

		}
	NEXT_STARTPOS:;
	}

}

//メルセンヌついスタのコンストラクタは遅いので参照で渡す
void do_randommove(Position& pos, StateInfo* s, std::mt19937& mt) {


	if (pos.is_incheck()) { return; }

	ExtMove moves[600], *end;

	memset(moves, 0, sizeof(moves));//指し手の初期化
	end = test_move_generation(pos, moves);//指し手生成
	ptrdiff_t num_moves = end - moves;

	Move m;

	while (true) {
		//次の局面に進めるための指し手の選択
		//合法手が選択されるまでwhileで回る

		m = moves[mt() % num_moves];


		if (pos.is_legal(m) && pos.pseudo_legal(m)) { break; }
	}
	pos.do_move(m, s);

}


#endif

#if defined(REIN) 
//定数
int read_teacher_counter = 0;
int batchsize = 1000000;//nozomiさんはminbatch100万でいいって言ってた
double loss = 0;

//ok
//参考　http://gurigumi.s349.xrea.com/programming/binary.html
/*
この方法で文字列を読もうとすると最初の文字列を何回も読み直してしまう！！
ifstreamを外部から参照渡しすることにする

*/
bool Rein_Learner::read_teacherdata(fstream& f) {
	sum_teachers.clear();
	//ifstream f(TEACHERPATH);
	if (!f) { cout << "cantopen" << TEACHERPATH << endl; UNREACHABLE; }
	teacher_data t;
	int i = 0;

	while (/*!f.eof() &&*/ sum_teachers.size() < batchsize) {
		/*f.seekg(read_teacher_counter * sizeof(teacher_data));
		f.read((char*)&t, sizeof(teacher_data));

		sum_teachers.push_back(t);
		read_teacher_counter++;
		i++;*/

		if (f.eof()) {

			f.close();
			listcounter++;
			if (listcounter >= teacher_list.size()) { return false; }
			f.open(teacher_list[listcounter], ios::in);
			cout << "teacher " << teacher_list[listcounter] << endl;
		}

		std::string line, sfen;
		Value v;
		Move m;
		if (!getline(f, line)) { break; };
		sfen = line;
		if (!getline(f, line)) { break; };
		v = (Value)stoi(line);


		teacher_data td(sfen, v);
		sum_teachers.push_back(td);
		i++;
		read_teacher_counter++;
	}
	cout << "read teacher counter>>" << read_teacher_counter << endl;
	bool is_eof = f.eof();
	//f.close();
	return (is_eof == false);
	/*Position pos;
	pos.unpack_haffman_sfen(sum_teachers[0].haffman);
	cout << pos << endl;
	cout << pos.occ_all() << endl;*/
	//return true;
}

#endif
#if defined(LEARN) && defined(REIN)



//lowerDimPP lowdim_;




void Rein_Learner::reinforce_learn() {

	/*int num_iteration=100;

	cout << "num_iteration?:"; cin >> num_iteration;*/


	//read_teacherdata();//ここで教師データを読み込む

	cout << "have G volume?[Y/N]" << endl;
	string haveg;
	cin >> haveg;
	if (haveg == "Y" || haveg == "y") {
		TEACHERPATH[0] = 'G';
	}
	cout << TEACHERPATH << endl;
#ifdef LOG
	std::string str, filepath;
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	str = asctime(timeinfo);

	size_t hoge;
	while ((hoge = str.find_first_of(" ")) != string::npos) {
		str.erase(hoge, 1);
	}
	while ((hoge = str.find_first_of("\n")) != string::npos) {
		str.erase(hoge, 1);
	}
	while ((hoge = str.find_first_of(":")) != string::npos) {
		str.erase(hoge, 1);
	}

#if defined(_MSC_VER)
	filepath = "c:/book2/log/" + str + "rein.txt";
#endif
#if defined(__unix__) 
	filepath = "/home/daruma/fvPP/" + str + ".txt";
#endif
	//filepath = "c:/book2/log/" + day + ".txt";
	ofstream ofs(filepath);
	if (ofs.fail()) { cout << "log fileError"; }

#endif



	sys::path p(TEACHERPATH);
	std::for_each(sys::recursive_directory_iterator(p), sys::recursive_directory_iterator(),
		[&](const sys::path& p) {
		if (sys::is_regular_file(p)) { // ファイルなら...
#ifdef  USEPACKEDSFEN
			if (p.filename().string().find("bin") != string::npos) {
				teacher_list.push_back(p.string());
			}
#else
			if (p.filename().string().find("txt") != string::npos) {
				teacher_list.push_back(p.string());
			}
#endif
		}
	});
	for (string l : teacher_list) {
		cout << l << endl;
	}


	fstream f;
#if defined(USEPACKEDSFEN)
	f.open(teacher_list[0], ios::in | ios::binary);
#else
	f.open(teacher_list[0], ios::in);
#endif
	//if (f) { cout << "cantopen" << TEACHERPATH << endl; UNREACHABLE; }


	std::random_device rd;
	std::mt19937 g_mt(rd());






	//gradJ格納庫用意
	maxthreadnum__ = omp_get_max_threads();
	for (size_t i = 0; i < maxthreadnum__; i++) {
		gradJs.emplace_back();
	}
	//スレッド作成
	vector<std::thread> threads(maxthreadnum__ - 1);

	//何回もiterationを回してもいい代物ではないだろう
	read_teacher_counter = 0;
	//初期化
	sum_gradJ.clear();
	for (dJValue& dJ : gradJs) { dJ.clear(); }

	//学習開始(readteacherdataでバッチサイズだけ棋譜を読み込んでミニバッチ学習を行う)
	while (read_teacherdata(f)) {

		loss = 0;

		sum_gradJ.clear();
		for (dJValue& dJ : gradJs) { dJ.clear(); }

		index__ = 0;
		for (int i = 0; i < maxthreadnum__ - 1; ++i) {
			threads[i] = std::thread([=] {reinforce_learn_pharse1(i); });
		}
		reinforce_learn_pharse1(maxthreadnum__ - 1);

		for (dJValue& dJ : gradJs) { sum_gradJ.add(dJ); }
		for (auto& th : threads) { th.join(); }

		//様々な開始局面からの大量のデータがあるので次元下げは必要ないと考えられる
		/*
		値の更新の方法はbonanza methodと同じではダメ！！！
		記録された深い探索の値もパラメーターが変わってくるとそれは深い探索の値ではなくなってくるはず
		となると全教師データを用いて一回しか値を更新できない？？？？？う～～ん....それはさすがにないような気がするのだけれど....
		値の更新の方法を勉強しないといけない...手元にadadeltaの論文があるしこれをつかうか？？
		*/

		//nozomiさん曰く次元下げはしないほうがいいとのこと。
		//#ifdef JIGENSAGE
		//		lowdim_.clear();
		//		lower__dimPP(lowdim_, sum_gradJ);
		//		sum_gradJ.clear();
		//		weave_lowdim_to_gradj(sum_gradJ, lowdim_);
		//#endif




				//renewal_PP_rein(sum_gradJ);//nanがいっぱい入っていてちゃんと値の更新ができていなかったので強さが変わらなかった疑惑。
		renewal_PP_nozomi(sum_gradJ);//こっちは確実に値がついていると考えられる。


		Eval::param_sym_ij();
		write_FV();
		read_FV();
		cout << "loss:" << loss << endl;
		ofs << "loss " << loss << endl;
	}
	ofs.close();
	f.close();
	cout << "finish rein" << endl;
}


/*
それ以前にunpacksfenは通常に動作している？？
あの辺バグってたし、今後のためにsfenで出力することにした
*/
void Rein_Learner::reinforce_learn_pharse1(const int index) {

	Position pos;
	Thread th;
	th.cleartable();
	StateInfo si[100];
	for (int i = 0; i < 100; i++) si[i].clear();
	/*ExtMove moves[600], *end;
	end = moves;*/

	for (int g = lock_index_inclement__(); g < sum_teachers.size(); g = lock_index_inclement__()) {

		auto teacher_data = sum_teachers[g];
		const Value teacher = (Value)teacher_data.teacher_value;


		pos.set(teacher_data.sfen);

		const Color rootColor = pos.sidetomove();//rootcolorをpos setをする前に用意してしまっていた！
		/*
		http://www2.computer-shogi.org/wcsc27/appeal/Apery/appeal_wcsc27.html
		基本的には6手読みで数十億局面に点数を付け、それらを0~1手読み(静止探索は完全に行う)で点数を近付けるようにオンライン学習を行います

		Aperyはこういってるし、精子探索はしたほうがいいのかも...??まあ確かに動的な局面の静的評価値なんて信頼ならんし必要か？
		よく考えたらうちのソフトの静止探索は駒の取り合いしか見ていないので静止探索入れても意味ないか...
		（静止探索にrecapture以外を入れたら弱くなったがあれはバグがあったからなのかもしれないな...もう一回試してみたほうがいいか）

		1手探索ぐらいはしたほうがいいのかもしれない


		静止探索の末端の値を更新するのには疑問を覚えるのであとで変えてみる


		*/
		//const Value shallow_v = Eval::eval(pos);
#if 0
		th.set(pos);
		//th.cleartable();//毎回は死ぬほど遅い
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		th.l_depth = 2;
		//Eval::eval(pos);
		Value shallow_serch_value = th.think();
		if (abs(shallow_serch_value) > 3000 || shallow_serch_value == 0) { continue; }

		int ii = 0;
		for (Move m : th.pv) { pos.do_move(m, &si[ii]); ii++; }//pvの末端へ移動
#elif 0
		th.set(pos);
		//th.cleartable();//毎回は死ぬほど遅い
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		Value shallow_serch_value = th.Qsearch();
		//if (abs(shallow_serch_value) > 3000 ) { continue; }

		int ii = 0;
		//ASSERT(th.pv[0]==th.RootMoves[0].move) //う～～んちゃんとPVを作れてるか怪しいな...


		for (Move m : th.pv) {
			if (!pos.pseudo_legal(m) || !pos.is_legal(m)) {
				cout << pos << endl;
				th.print_pv(0, shallow_serch_value);
				ASSERT(0);
			}
			pos.do_move(m, &si[ii]); ii++;
		}//pvの末端へ移動



#endif

#ifdef EVAL_KPP
		pos.state()->sumBKPP = Value_error; pos.state()->previous->sumBKPP = Value_error;
#elif defined(EVAL_PP)
		pos.state()->bpp = pos.state()->wpp = Value_error;//差分計算を無効にしてみる
		if (pos.state()->previous != nullptr) { pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error; }
		//previousをvalueerrorにするのを忘れていた
#elif defined(EVAL_PPT)
		pos.state()->bpp = pos.state()->wpp = Value_error;//差分計算を無効にしてみる
		if (pos.state()->previous != nullptr) { pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error; }
#endif
		//rootから見た点数に変換する（teacherもrootから見た評価値のはず）]


		Value shallow_v = (rootColor == pos.sidetomove()) ? Eval::eval(pos) : -Eval::eval(pos);//ここ探索で帰ってきた値にすべき？（よくなかった）
		//Value shallow_v = shallow_serch_value;

		double win_teacher = win_sig(teacher);
		double win_shallow = win_sig(shallow_v);
		double diffsig_ = win_shallow - win_teacher;//交差エントロピー これのほうがいいってnozomiさんが言ってた

		/*
		PPの評価値の傾き方からしてponanzaの勝率の式は使えないと思って評価値の差だけにしていたが、
		そのままの評価値だと差が大きすぎるところの値が支配的になってしまうのでやはり勝率に変換すべきか？？
		*/
		//double diffsig_ = (shallow_v - teacher);

		//loss += diffsig_*diffsig_;
		loss += pow(int(shallow_v - teacher), 2);



#ifdef EVAL_KPPT
		double diffsig_withoutturn = (rootColor == BLACK ? diffsig_ : -diffsig_);//+bpp-wppの関係
		double diffsig_turn = diffsig_;
		std::array<float, 2> diffsig = { -diffsig_withoutturn,(rootColor == pos.sidetomove() ? -diffsig_turn : diffsig_turn) };
		gradJs[index].update_dJ(pos, diffsig);
#else
		double diffsig = (rootColor == BLACK ? diffsig_ : -diffsig_);
		double diffsig_turn = (rootColor == pos.sidetomove() ? -diffsig_ : diffsig_);//rootcolorつまり手番を握っている側に対してボーナスを与える
#ifdef EVAL_PP
		gradJs[index].update_dJ(pos, -diffsig);
#elif defined(EVAL_PPT)
		gradJs[index].update_dJ(pos, -diffsig, diffsig_turn);
#endif
#endif
	}


}
#if 0
double PP_double[fe_end2][fe_end2] = { 0,0 };

constexpr double row = 0.95, epsiron = 0.0001;
double lastEg[fe_end2][fe_end2] = { 0.0 }, last_Edeltax[fe_end2][fe_end2] = { 0.0 };
double RMS(const double a) { return sqrt(a + epsiron); }


void PP_to_doublePP() {

	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			PP_double[i][j] = double(PP[i][j]);
		}
	}


}


void doublePP_to_PP() {

	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			PP[i][j] = int32_t(PP_double[i][j]);
		}
	}
}

//Adadeltaを試してみる

void renewal_PP_rein(dJValue &data) {


	//PP_to_doublePP();
	//対称性はdJの中に含まれているのでここでは考えなくていい
	/*
	目的関数は小さくなったのだが、くっそ弱くなった。
	目的関数が小さくなったということは学習には成功しているはずである。
	教師データ作成、開始局面作成が雑すぎた、ハフマン符号化がうまくいってなかったということもあるかもしれないが、いったん強化学習部から離れてほかのところを弄ろうと思う。
	*/
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {



			PP_double[i][j] = double(PP[i][j]);

			double gt = data.absolute_PP[i][j];
			double gt2 = gt*gt;
			double Egt = lastEg[i][j] = (row)*lastEg[i][j] + (1 - row)*gt2;
			double delta_x = gt*RMS(last_Edeltax[i][j]) / RMS(Egt);
			last_Edeltax[i][j] = (row)*last_Edeltax[i][j] + (1 - row)*delta_x;

			//FV_SCALEでいいのか？
			/*
			こっちのほうが強くなったと思ったが配列にnanが入っていて値が動いていないだけだった

			*/
			if (delta_x == NAN) { continue; }
			int add = int(delta_x)*FV_SCALE;//clampか何かしたほうがいいか？(というかPPをいったんdoubleに変換してそれをintに戻すほうがいいか？？)
			if (abs(PP[i][j] + add) < INT16_MAX) {
				PP[i][j] += add;
			}

			////この方法ではパラメータが全然動いていなかったう～～～～んオンライン学習でも始めるか？？
			//if (abs(PP_double[i][j] + delta_x) < INT16_MAX) {
			//	PP_double[i][j] += delta_x;
			//}


			PP[i][j] = int32_t(PP_double[i][j]);

		}
	}
	//doublePP_to_PP();

}
#endif
/*
wcsc27でnozomiさんに教えていただいた方法。
勾配の方向に1だけ動かす！！！！！！！！
kppなどに局所解はあまりないというのが見解としてわかったらしいので0~2までの範囲で動かす値を変えるなんてしなくていいみたい。
なんか昔はハイパーパラメータいろいろ弄ったらしいがここに落ち着いたようだ...

https://twitter.com/hillbig/status/868053156223004672
https://twitter.com/hillbig/status/869122361001246720
https://twitter.com/hillbig/status/869348563259490304

これをみるとたしかにこの方法でなんだかんだいけるような気がする
ただし動かす値が1だけなので学習データはいっぱいほしい
（1だけではあるが場合によっては1が大きいということもあるかもしれないが）
*/
void renewal_PP_nozomi(dJValue &data) {



	int h;

	h = 1;
#ifdef EVAL_PP
	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			int inc = h*sign(data.absolute_PP[i][j]);
			if (abs(PP[i][j] + inc) < INT16_MAX) {
				PP[i][j] += inc;
			}

		}
	}
#elif defined(EVAL_PPT)
	//対称性はdJの中に含まれているのでここでは考えなくていい
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			int inc = h*sign(data.absolute_PP[i][j]);
			if (abs(PP[i][j] + inc) < INT16_MAX) {
				PP[i][j] += inc;
			}
			inc = h*sign(data.absolute_PPt[i][j]);
			if (abs(PPT[i][j] + inc) < INT16_MAX) {
				PPT[i][j] += inc;
			}
		}
	}
#endif

};



#endif


#if defined(REIN)

/*
毎回clear tableするとほとんどの評価値の誤差は非常に小さい範囲に収まった.小さいやつは1も差がない
しかし毎回初期化して探索を行うと時間を馬鹿みたいに食う...まあ毎回初期化は教師データ作成時にはしなくていいだろう。



AperyWCSC26の教師データファイル読み込みに間違いはなかった
*/
void Rein_Learner::check_teacherdata() {

	//	Position pos;
	Position pos__;
	Thread th;
	th.cleartable();

	ofstream ofs("C:/Users/daruma/Desktop/SQcheckteacher.txt");


	fstream f;
	f.open(TEACHERPATH, ios::in | ios::binary);
	if (!f) { cout << "cantopen" << TEACHERPATH << endl; UNREACHABLE; }
	uint64_t count = 0;

	bool firsttime = true;

	while (read_teacherdata(f) || firsttime) {

		firsttime = false;
		for (int g = lock_index_inclement__(); g < sum_teachers.size(); g = lock_index_inclement__()) {
			count++;
			auto data = sum_teachers[g];


			pos__.set(data.sfen);

			//cout << pos__ << endl;
#if 0
			th.cleartable();
			th.set(pos__);
			th.l_depth = DEPTH;
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			Value v = th.think();//探索を実行する

								 //tableの値の付き方が変わってたりするかもしれないので100は許容する（それでも大きいが）
			if (abs(v - (Value)data.teacher_value) > 1) {
				//cout << "position " << data.sfen << endl;
				cout << "nowsearched:" << v << " data:" << data.teacher_value << endl;
				//ASSERT(0);
			}
			else {
				cout << "ok teacher" << endl;
			}
#else
			ofs << pos__.make_sfen() << " " << data.teacher_value << endl;
			if (count > 1000) { goto END; }
#endif
			//pos__.unpack_haffman_sfen(data.haffman);
			//ASSERT(pos == pos__);
		}
		cout << count << endl;
	}
END:;
	ofs.close();
};
#endif