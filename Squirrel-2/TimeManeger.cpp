#include "TimeManeger.h"
#include "usi.h"
#include <cfloat>
//TimeManeger TimeMan;

/*
http://yaneuraou.yaneu.com/2015/01/02/stockfish-dd-timeman-%E6%99%82%E9%96%93%E5%88%B6%E5%BE%A1%E9%83%A8/


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
namespace {

	enum TimeType { OptimumTime, MaxTime };

	const int MoveHorizon = 50;   // Plan time management at most this many moves ahead 最大であと50手はさせるぐらいには持ち時間を残しておく
	const double MaxRatio = 7.09; // When in trouble, we can step over reserved time with this ratio　困ったときは、この比率に基づいてとっておいた時間を踏み越えることができる
	const double StealRatio = 0.35; // However we must not steal time from remaining moves over this ratio　しかし、これ以上の比率を超えて時間を盗めない

	// move_importance() is a skew-logistic function based on naive statistical
	// analysis of "how many games are still undecided after n half-moves". Game
	// is considered "undecided" as long as neither side has >275cp advantage.
	// Data was extracted from the CCRL game database with some simple filtering criteria.
	/*
	moveimportanceは「n手指した後まだ決着がついていなかった対局がどれだけあったのか」という単純な統計分析のロジスティック関数に基づく。
	これはchessの場合の話なので将棋の場合は別の値を使うべきか？？

	形勢に差が開いてからはあまり重要な局面ではなく、それまでに問題があるのだから、そこに思考時間を投与すべきという考え方。
	*/
	double move_importance(int ply) {

		const double XScale = 7.64;
		const double XShift = 58.4;
		const double Skew = 0.183;

		return pow((1 + exp((ply - XShift) / XScale)), -Skew) + DBL_MIN; // Ensure non-zero
	}

	template<TimeType T>
	int remaining(int myTime, int movesToGo, int ply, int slowMover) {

		const double TMaxRatio = (T == OptimumTime ? 1 : MaxRatio);
		const double TStealRatio = (T == OptimumTime ? 0 : StealRatio);

		double moveImportance = (move_importance(ply) * slowMover) / 100;
		double otherMovesImportance = 0;

		for (int i = 1; i < movesToGo; ++i)
			otherMovesImportance += move_importance(ply + 2 * i);

		double ratio1 = (TMaxRatio * moveImportance) / (TMaxRatio * moveImportance + otherMovesImportance);
		double ratio2 = (moveImportance + TStealRatio * otherMovesImportance) / (moveImportance + otherMovesImportance);

		return int(myTime * std::min(ratio1, ratio2)); // Intel C++ asks for an explicit cast
	}

}


// init() is called at the beginning of the search and calculates the allowed
// thinking time out of the time control and current game ply. We support four
// different kinds of time controls, passed in 'limits':
//
//  inc == 0 && movestogo == 0 means: x basetime  [sudden death!]
//  inc == 0 && movestogo != 0 means: x moves in y minutes
//  inc >  0 && movestogo == 0 means: x basetime + z increment
//  inc >  0 && movestogo != 0 means: x moves in y minutes + z increment
void TimeManeger::init(SearchLimit & limit, Color us, int ply)
{
	TimePoint minThinkingTime = Options["Minimum Thinking Time"];
	int moveOverhead = Options["Move Overhead"];
	int slowMover = Options["Slow Mover"];

	startTime = limit.starttime;
	optimumTime = maximumTime = std::max(limit.remain_time[us], minThinkingTime);//最大残り時間で初期化

	const int MaxMTG = MoveHorizon;

	// We calculate optimum time usage for different hypothetical "moves to go"-values
	// and choose the minimum of calculated search time values. Usually the greatest
	// hypMTG gives the minimum values.
	//WCSCはフィッシャールールなのでhypMTG=1からmaxMTGまで回す必要がある。
	//そうじゃなければmaxMTGだけを取ってくればいい
	for (int hypMTG = 1; hypMTG <= MaxMTG; ++hypMTG)
	{
		// Calculate thinking time for hypothetical "moves to go"-value
		int hypMyTime = limit.remain_time[us]
			+ limit.inc_time * (hypMTG - 1)
			- moveOverhead * (2 + std::min(hypMTG, 40));

		hypMyTime = std::max(hypMyTime, 0);

		TimePoint t1 = minThinkingTime + remaining<OptimumTime>(hypMyTime, hypMTG, ply, slowMover);
		TimePoint t2 = minThinkingTime + remaining<MaxTime    >(hypMyTime, hypMTG, ply, slowMover);

		optimumTime = std::min(t1, optimumTime);
		maximumTime = std::min(t2, maximumTime);
	}

	//ponderができるときは相手手番中に考えることができるので少し考える時間を延ばしてよい。
	if (Options["USI_Ponder"]) {
		optimumTime += optimumTime / 4;
	}
	/*limit.endtime = maximumTime;
	limit.opitmumTime = optimumTime;*/
}
