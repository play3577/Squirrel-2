
#include "fundation.h"
#include "usi.h"
#include "Thread.h"


/*
この辺全然わからん....
*/


ThreadPool Threadpool;

/*
ThreadPool::init() creates and launches requested threads that will go
immediately to sleep. We cannot use a constructor because Threads is a
static object and we need a fully initialized engine at this point due to
allocation of Endgames in the Thread constructor.


ThreadPool :: init（）は要求されたスレッドを作成し起動し、直ちにスリープ状態になります。
Threadpoolは静的オブジェクトであり、ThreadコンストラクタでEndgamesを割り当てるため、
この時点で完全に初期化されたエンジンが必要なため、コンストラクタは使用できません。

将棋ではEndgameの割り当てがないのでコンストラクタでもいいと思うけれど
まあSFにしたがっとくか
*/
void ThreadPool::init() {

	push_back(new MainThread);//main threadを用意する
	read_usi_options();//ここでいくつのスレッドをたてるかなどを設定から読み込む（しかしまだsetoptionを受け取る前であるのでこれはまた呼びなおされる。）

}


//コンストラクタ。作られたらすぐにidle_loopに入り、sleep状態になる。
Thread::Thread() {

	exit =resetCalls= false;
	cleartable();
	idx = Threadpool.size();
	//https://cpprefjp.github.io/reference/mutex/unique_lock.html
	//http://qiita.com/termoshtt/items/c01745ea4bcc89d37edc
	std::unique_lock<Mutex> lk(mutex);
	searching = true;
	nativeThread = std::thread(&Thread::idle_loop, this);
	sleepCondition.wait(lk, [&] {return !searching; });

}

void Thread::wait_for_search_finished() {

	std::unique_lock<Mutex> lk(mutex);
	sleepCondition.wait(lk, [&] { return !searching; });
}


void Thread::wait(std::atomic_bool& condition) {

	std::unique_lock<Mutex> lk(mutex);
	sleepCondition.wait(lk, [&] { return bool(condition); });
}


void Thread::start_searching(bool resume) {

	std::unique_lock<Mutex> lk(mutex);

	if (!resume)
		searching = true;

	sleepCondition.notify_one();
}


void Thread::idle_loop() {

	while (!exit)
	{
		std::unique_lock<Mutex> lk(mutex);

		searching = false;

		while (!searching && !exit)
		{
			sleepCondition.notify_one(); // Wake up any waiting thread
			sleepCondition.wait(lk);
		}

		lk.unlock();

		if (!exit)
			think();
	}
}







/*
exit なぜデストラクタではダメなのか書かれていたがまあよくわからんのでしたがっとく
*/
void ThreadPool::exit() {

	while (size())
	{
		delete back(), pop_back();
	}
}

Thread::~Thread() {

	mutex.lock();
	exit = true;
	sleepCondition.notify_one();
	mutex.unlock();
	nativeThread.join();
}

//要求されたスレッド数になるようにスレッドを生成消滅させる
void ThreadPool::read_usi_options() {

	int requested = Options["Threads"];
	ASSERT(requested > 0);

	while (size()<requested)
	{
		push_back(new Thread);
	}
	while (size() > requested) {
		delete back(), pop_back();
	}

}

//全スレッドの探索ノード数
int64_t ThreadPool::nodes_searched() {

	int64_t sum = 0;

	for (Thread* th : *this) {
		sum += th->searchednodes();
	}

	return sum;
}





void ThreadPool::start_thinking(Position pos) {

	main()->wait_for_search_finished();
	signal.stopOnPonderHit = signal.stop = false;
	
	//毎回全スレッド差し手生成させるのは無駄だよな...
	//posも参照渡しできないし...
	for (Thread*th : Threadpool) {
		th->set(pos);
	}

	main()->start_searching();

}



