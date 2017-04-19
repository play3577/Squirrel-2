#pragma once
#include "fundation.h"
#include "position.h"
#include "search.h"
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "moveStats.h"
using namespace std;


#if defined(_WIN32) && !defined(_MSC_VER)

#ifndef NOMINMAX
#  define NOMINMAX // Disable macros min() and max()
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX

/// Mutex and ConditionVariable struct are wrappers of the low level locking
/// machinery and are modeled after the corresponding C++11 classes.

struct Mutex {
	Mutex() { InitializeCriticalSection(&cs); }
	~Mutex() { DeleteCriticalSection(&cs); }
	void lock() { EnterCriticalSection(&cs); }
	void unlock() { LeaveCriticalSection(&cs); }

private:
	CRITICAL_SECTION cs;
};

typedef std::condition_variable_any ConditionVariable;

#else // Default case: use STL classes

typedef std::mutex Mutex;
typedef std::condition_variable ConditionVariable;

#endif




const string print_value(Value v);

#ifndef LEARN
struct Thread {

private:


	std::thread nativeThread;
	Mutex mutex;
	ConditionVariable sleepCondition;
	bool exit, searching;


	
	int rootdepth;
	//rootmoves�ɔ񍇖@�肪�����Ă���̂�h���Ȃ���΂Ȃ�Ȃ�
	
public:
	Position rootpos;//rootpos�̓X���b�h���ɕۂK�v������̂ŎQ�Ɠn���ł͂����Ȃ��B(����public�ɂ���̊댯����Ȃ�..)

	int completedDepth;

	size_t idx, PVIdx;

	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomic�ɂ��邱�ƂŃX���b�h�ϋ������N����Ȃ��悤�ɂ���
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore=Value_Zero;
	//����w�K���ɂ������N����Ă��܂��̂�h��
	CounterMoveHistoryStats CounterMoveHistory;

#if defined(LEARN) || defined(MAKEBOOK) || defined(MAKESTARTPOS)
	Value l_alpha;
	Value l_beta;
	int l_depth;
#endif


public:
	Thread();
	virtual ~Thread();

	void idle_loop();
	void start_searching(bool resume = false);
	void wait_for_search_finished();
	void wait(std::atomic_bool& b);

	std::vector<Move> pv;//�ǂ݋�

	void set(Position pos);
	int rdepth() { return rootdepth; }
	virtual Value think();

	ExtMove* find_rootmove(const Move object) {
		for (ExtMove* i = RootMoves; i < end; i++) {
			if (i->move == object) { return i; }
		}
		return nullptr;
	}
	void sort_RootMove();
	void print_pv(const int depth, Value v);
	void cleartable() {
		history.clear();
		counterMoves.clear();
		fromTo.clear();
		CounterMoveHistory.clear();
	}
	int64_t searchednodes() { return rootpos.searched_nodes(); }
	bool extract_ponder_from_tt(Position& pos);
};


struct MainThread :public Thread
{
	virtual Value think();

	bool easyMovePlayed, failedLow;
	double bestMoveChanges;
	Value previousScore;
	
};



/// ThreadPool struct handles all the threads-related stuff like init, starting,
/// parking and, most importantly, launching a thread. All the access to threads
/// data is done through this class.
//����Ȍp�����ł���̂�...�׋��ɂȂ�Ȃ�
/*
���������������Ă��܂��Ɗw�K������Ƃ��̒T���̐݌v���������Ȃ��Ƃ����Ȃ���.....
*/
struct ThreadPool :public std::vector<Thread*> {

	/*
	�R���X�g���N�^�ƃf�X�g���N�^�͑��݂��Ȃ��B
	threads��global�ɂ����A�������Ȃ���A�m���łȂ���΂Ȃ�Ȃ��B
	*/
	void init();
	void exit();

	MainThread* main() { return static_cast<MainThread*>(at(0)); }
	void start_thinking(Position pos);
	void read_usi_options();
	int64_t nodes_searched();

};

extern ThreadPool Threads;


#else
struct Thread {

private:


	
	Position rootpos;//rootpos�̓X���b�h���ɕۂK�v������̂ŎQ�Ɠn���ł͂����Ȃ��B
	int rootdepth;
	//rootmoves�ɔ񍇖@�肪�����Ă���̂�h���Ȃ���΂Ȃ�Ȃ�

public:

	size_t idx, PVIdx;

	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomic�ɂ��邱�ƂŃX���b�h�ϋ������N����Ȃ��悤�ɂ���
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore = Value_Zero;
	//����w�K���ɂ������N����Ă��܂��̂�h��
	CounterMoveHistoryStats CounterMoveHistory;

#if defined(LEARN) || defined(MAKEBOOK)
	Value l_alpha;
	Value l_beta;
	int l_depth;
#endif


public:

	std::vector<Move> pv;//�ǂ݋�

	void set(Position pos);
	int rdepth() { return rootdepth; }
	Value think();

	ExtMove* find_rootmove(const Move object) {
		for (ExtMove* i = RootMoves; i < end; i++) {
			if (i->move == object) { return i; }
		}
		return nullptr;
	}
	void sort_RootMove();
	void print_pv(const int depth, Value v);
	void cleartable() {
		history.clear();
		counterMoves.clear();
		fromTo.clear();
		CounterMoveHistory.clear();
	}
	int64_t searchednodes() { return rootpos.searched_nodes(); }
	bool extract_ponder_from_tt(Position& pos);
};
#endif