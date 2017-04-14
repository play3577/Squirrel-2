#pragma once
/*
中規模棋譜学習用の関数をここで宣言する

数手先の評価値を教師にして学習を行うのでこれって強化学習？？？とは思うけれどほかの強豪ソフトが強化学習って言ってるので強化学習なんだろう。
（そもそも強化学習が何なのかわかっていない）


まずは教師データ生成。
教師データを生成するにはそのデータ構造を決めなければならない。
学習に必要なのは
その局面のsfenまたはハフマン化sfenと数手先を読んだ時の評価値
sfenはハフマン化して256bit 評価値は16bitであるので　272bitあればよい
これをクラスとして用意してファイルにバイナリ形式で書き出していく。
これで何とかなると思われる。

そして棋譜に出てくる局面にランダム性を持たせるために、
チェス960みたいに初期局面ランダムデータベースを作る必要がある。

評価値は2000を超えれば試合を打ち切る

教師データ作成時には
次の手に進むための差し手にもランダム性を持たせる。
完全にランダムでもそれなりに良いかもしれないが、30％ぐらいにしておこうかな

*/
void make_startpos_detabase();

void make_teacher();
void make_teacher_body(const int number);
bool read_teacherdata();
void reinforce_learn();