#pragma once
/*
中規模棋譜学習用の関数をここで宣言する

数手先の評価値を教師にして学習を行うのでこれって強化学習？？？とは思うけれどほかの強豪ソフトが強化学習って言ってるので強化学習なんだろう。
（そもそも強化学習が何なのかわかっていない）


まずは教師データ生成。
教師データを生成するにはそのデータ構造を決めなければならない。
学習に必要なのは
その局面のsfenまたはハフマン化sfenと数手先を読んだ時の評価値
sfenはハフマン化して256bit 評価値は16bitであるので　272bitあればよい(paddingをなくさないといけないな...)
これをクラスとして用意してファイルにバイナリ形式で書き出していく。
これで何とかなると思われる。

そして棋譜に出てくる局面にランダム性を持たせるために、
チェス960みたいに初期局面ランダムデータベースを作る必要がある。

評価値は2000を超えれば試合を打ち切る

教師データ作成時には
次の手に進むための差し手にもランダム性を持たせる。
完全にランダムでもそれなりに良いかもしれないが、30％ぐらいにしておこうかな


学習部をもはや静的に持てなくなってしまったので学習部クラスを作って
そこに必要なデータ全部突っ込んでそれを動的に確保する設計にしなければならない！！！！！！！！


*/
#include <fstream>
#include <sstream>
#include "learner.h"


#if defined(REIN) || defined(MAKETEACHER)
struct teacher_data {

	//bool haffman[256];

	string sfen;//ハフマン失敗しまくったのでstringで行く。　大会終わって時間もできたしハフマン変換関数のデバッグするか...

				//
	int16_t teacher_value;
	uint16_t move;
	uint16_t gameply;
	bool isWin;
	uint8_t padding;
	//teacher_data() {};
	/*
	elmo式を使うならここに勝率
	qhapak式で教師手よりも価値の高い差し手を低くするためには差し手を含めなければならない

	勝敗を入れることで
	数手先だけの局所最適化が防げる（NDFの金沢さん）
	評価値と勝率がかけ離れている戦型も学習がちゃんとできるようになる（かパックの澤田さん）
	*/
	//Color winner;
	//Move move;

	//teacher_data(/*const bool *haff,*/const string sfen_,const Value teachervalue,const Move m) {
	//	//memcpy(haffman, haff, sizeof(haffman));
	//	sfen = sfen_;
	//	teacher_value = (int16_t)teachervalue;
	//	move = m;
	//}

	teacher_data(const string sfen_, const Value teachervalue) {

		sfen = sfen_;
		teacher_value = (int16_t)teachervalue;

	}

	teacher_data() {}
};
//static_assert(sizeof(teacher_data) == 40);

inline std::ostream& operator<<(std::ostream& os, const teacher_data& td) {
	os << td.sfen << endl;
	os << td.teacher_value << endl;
	//os << td.move << endl;
	return os;
}
#endif

#if defined(MAKETEACHER)

//学習用、データ作成用のクラスを動的確保させる
class Make_Teacher {
private:
	vector<string> startpos_db;//開始局面集
	vector<vector<teacher_data>> teachers;//threadごとの作成した教師局面データ。
	vector<teacher_data> sum_teachers;//最後にここにthreadごとに作成した教師局面データをまとめてあげる

	uint64_t sumteachersize = 0;
	int maxthreadnum__;


	//------------------------------------------------maltithread時のrandam開始局面databaseのindexとか教師データのindexとかに使う
	std::mutex mutex__;
	int index__ = 0;

	int lock_index_inclement__() {
		std::unique_lock<std::mutex> lock(mutex__);
		if (index__ > startpos_db.size()) { cout << "o" << endl; }

		else if (index__ % 1000 == 0) { cout << "."; }

		return index__++;
	}
	//==========================================================================

public:
	void make_teacher();
	void make_teacher_body(const int number);

};
#elif defined(REIN)
class Rein_Learner {


	dJValue sum_gradJ;
	vector<dJValue> gradJs;

	vector<teacher_data> sum_teachers;//最後にここにthreadごとに作成した教師局面データをまとめてあげる
	int maxthreadnum__;

	vector<string> teacher_list;
	int listcounter = 0;



	//------------------------------------------------maltithread時のrandam開始局面databaseのindexとか教師データのindexとかに使う
	std::mutex mutex__;
	int index__ = 0;

	int lock_index_inclement__() {
		std::unique_lock<std::mutex> lock(mutex__);
#ifdef MAKETEACHER
		if (index__ > startpos_db.size()) { cout << "o" << endl; }
#else
		if (index__ > sum_teachers.size()) { cout << "o" << endl; }
#endif //  MAKETEACHER
		else if (index__ % 1000 == 0) { cout << "."; }

		return index__++;
	}
	//==========================================================================

public:
	void reinforce_learn_pharse1(const int index);
	void reinforce_learn();
	void check_teacherdata();
	bool read_teacherdata(fstream& f);
};

#endif // defined(MAKETEACHER)