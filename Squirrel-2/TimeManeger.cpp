#include "TimeManeger.h"
#include "usi.h"
#include <cfloat>
//TimeManeger TimeMan;

/*
http://yaneuraou.yaneu.com/2015/01/02/stockfish-dd-timeman-%E6%99%82%E9%96%93%E5%88%B6%E5%BE%A1%E9%83%A8/


http://qhapaq.hatenablog.com/entry/2017/01/24/230851
http://qhapaq.hatenablog.com/entry/2017/01/31/195428

�P�D�I�ǂ܂ł̂P��̕��ώ��Ԃ����ς���i20��ڂŎc�莞��50���A120��ڂ��炢�Ō��������Ɨ\�z����Ȃ�P��1���ƌ������v�́j

�Q�D�ő�v�l���Ԃ����߂�i�ߎ��I�ɂ͕��ώ��Ԃ�5-7�{�j

�R�D�ǖʂ�ǂݐi�߂�

�S�D�����ɁA�ǂ݂��P��[�����閈�ɍœK�肪�ω�����m�������ς���

�T�D����ԁi�ɂ�����̐������������́j���S�ŋ��߂��m������������ǂ݂𒆒f����

���̒��œ��ɗD��Ă���̂��T�̕����ł��B
����Ԃ𑝂₷���ɁA�ǂ݂�ł��؂�����������������邱�Ƃ��~�\�ł�
�B�����i��`�F�X�j�͐[���ǂ߂ΓǂނقǁA�肪�ł܂�₷���Ȃ�Ɠ����ɓǂނׂ��ǖʂ������Ă��܂�����
�A�ߋ��̎����̓ǂ݂�M���āA�ϋl�܂����Ǝv�����畁�i�Ȃ���������ǂދǖʂ����u���Đ�ɐi�߂悤�Ƃ������z�ł��B

*/
namespace {

	enum TimeType { OptimumTime, MaxTime };

	const int MoveHorizon = 50;   // Plan time management at most this many moves ahead �ő�ł���50��͂����邮�炢�ɂ͎������Ԃ��c���Ă���
	const double MaxRatio = 7.09; // When in trouble, we can step over reserved time with this ratio�@�������Ƃ��́A���̔䗦�Ɋ�Â��ĂƂ��Ă��������Ԃ𓥂݉z���邱�Ƃ��ł���
	const double StealRatio = 0.35; // However we must not steal time from remaining moves over this ratio�@�������A����ȏ�̔䗦�𒴂��Ď��Ԃ𓐂߂Ȃ�

	// move_importance() is a skew-logistic function based on naive statistical
	// analysis of "how many games are still undecided after n half-moves". Game
	// is considered "undecided" as long as neither side has >275cp advantage.
	// Data was extracted from the CCRL game database with some simple filtering criteria.
	/*
	moveimportance�́un��w������܂����������Ă��Ȃ������΋ǂ��ǂꂾ���������̂��v�Ƃ����P���ȓ��v���͂̃��W�X�e�B�b�N�֐��Ɋ�Â��B
	�����chess�̏ꍇ�̘b�Ȃ̂ŏ����̏ꍇ�͕ʂ̒l���g���ׂ����H�H

	�`���ɍ����J���Ă���͂��܂�d�v�ȋǖʂł͂Ȃ��A����܂łɖ�肪����̂�����A�����Ɏv�l���Ԃ𓊗^���ׂ��Ƃ����l�����B
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
	optimumTime = maximumTime = std::max(limit.remain_time[us], minThinkingTime);//�ő�c�莞�Ԃŏ�����

	const int MaxMTG = MoveHorizon;

	// We calculate optimum time usage for different hypothetical "moves to go"-values
	// and choose the minimum of calculated search time values. Usually the greatest
	// hypMTG gives the minimum values.
	//WCSC�̓t�B�b�V���[���[���Ȃ̂�hypMTG=1����maxMTG�܂ŉ񂷕K�v������B
	//��������Ȃ����maxMTG����������Ă���΂���
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

	//ponder���ł���Ƃ��͑����Ԓ��ɍl���邱�Ƃ��ł���̂ŏ����l���鎞�Ԃ����΂��Ă悢�B
	if (Options["USI_Ponder"]) {
		optimumTime += optimumTime / 4;
	}
	/*limit.endtime = maximumTime;
	limit.opitmumTime = optimumTime;*/
}
