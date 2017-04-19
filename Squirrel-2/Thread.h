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
	//rootmovesに非合法手が入ってくるのを防がなければならない
	
public:
	Position rootpos;//rootposはスレッド毎に保つ必要があるので参照渡しではいけない。(これpublicにするの危険だよなぁ..)

	int completedDepth;

	size_t idx, PVIdx;

	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomicにすることでスレッド観競合が起こらないようにする
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore=Value_Zero;
	//並列学習中にここが侵されてしまうのを防ぐ
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

	std::vector<Move> pv;//読み筋

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
//こんな継承ができるのか...勉強になるなぁ
/*
しかしこれを作ってしまうと学習させるときの探索の設計を見直さないといけないな.....
*/
struct ThreadPool :public std::vector<Thread*> {

	/*
	コンストラクタとデストラクタは存在しない。
	threadsはglobalにたより、初期化なされ、確実でなければならない。
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


	
	Position rootpos;//rootposはスレッド毎に保つ必要があるので参照渡しではいけない。
	int rootdepth;
	//rootmovesに非合法手が入ってくるのを防がなければならない

public:

	size_t idx, PVIdx;

	ExtMove RootMoves[600], *end;
	std::atomic_bool resetCalls;//atomicにすることでスレッド観競合が起こらないようにする
	int call_count = 0;
	int seldepth = 0;
	HistoryStats history;
	MoveStats counterMoves;
	FromToStats fromTo;
	Value previousScore = Value_Zero;
	//並列学習中にここが侵されてしまうのを防ぐ
	CounterMoveHistoryStats CounterMoveHistory;

#if defined(LEARN) || defined(MAKEBOOK)
	Value l_alpha;
	Value l_beta;
	int l_depth;
#endif


public:

	std::vector<Move> pv;//読み筋

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