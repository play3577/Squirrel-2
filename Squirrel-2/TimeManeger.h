#pragma once

/*
http://qhapaq.hatenablog.com/entry/2017/01/24/230851
http://qhapaq.hatenablog.com/entry/2017/01/31/195428

１．終局までの１手の平均時間を見積もる（20手目で残り時間50分、120手目ぐらいで決着がつくと予想するなら１手1分と言った要領）

２．最大思考時間を決める（近似的には平均時間の5-7倍）

３．局面を読み進める

４．同時に、読みを１手深くする毎に最適手が変化する確率を見積もる

５．消費時間（にある一定の数をかけたもの）が４で求めた確率を上回ったら読みを中断する

この中で特に優れているのが５の部分です。
消費時間を増やす毎に、読みを打ち切る条件つけを厳しくすることがミソです
。将棋（やチェス）は深く読めば読むほど、手が固まりやすくなると同時に読むべき局面が増えてしまうため
、過去の自分の読みを信じて、煮詰まったと思ったら普段ならもう少し読む局面も放置して先に進めようという発想です。

*/

#include "misc.h"
#include "search.h"


class TimeManeger {

private:
	TimePoint startTime;
	int optimumTime;
	int maximumTime;
public:
	void init(SearchLimit& limit, Color us, int ply);
	int optimum() const { return optimumTime; }
	int maximum()const { return maximumTime; }
	int elasped()const { return int(now() - startTime); }

};
extern TimeManeger TimeMan;
