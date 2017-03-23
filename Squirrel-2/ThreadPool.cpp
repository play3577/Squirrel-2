
#include "fundation.h"
#include "usi.h"
#include "Thread.h"


/*
���̕ӑS�R�킩���....
*/


ThreadPool Threadpool;

/*
ThreadPool::init() creates and launches requested threads that will go
immediately to sleep. We cannot use a constructor because Threads is a
static object and we need a fully initialized engine at this point due to
allocation of Endgames in the Thread constructor.


ThreadPool :: init�i�j�͗v�����ꂽ�X���b�h���쐬���N�����A�����ɃX���[�v��ԂɂȂ�܂��B
Threadpool�͐ÓI�I�u�W�F�N�g�ł���AThread�R���X�g���N�^��Endgames�����蓖�Ă邽�߁A
���̎��_�Ŋ��S�ɏ��������ꂽ�G���W�����K�v�Ȃ��߁A�R���X�g���N�^�͎g�p�ł��܂���B

�����ł�Endgame�̊��蓖�Ă��Ȃ��̂ŃR���X�g���N�^�ł������Ǝv�������
�܂�SF�ɂ��������Ƃ���
*/
void ThreadPool::init() {

	push_back(new MainThread);//main thread��p�ӂ���
	read_usi_options();//�����ł����̃X���b�h�����Ă邩�Ȃǂ�ݒ肩��ǂݍ��ށi�������܂�setoption���󂯎��O�ł���̂ł���͂܂��ĂтȂ������B�j

}


//�R���X�g���N�^�B���ꂽ�炷����idle_loop�ɓ���Asleep��ԂɂȂ�B
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
exit �Ȃ��f�X�g���N�^�ł̓_���Ȃ̂�������Ă������܂��悭�킩���̂ł��������Ƃ�
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

//�v�����ꂽ�X���b�h���ɂȂ�悤�ɃX���b�h�𐶐����ł�����
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

//�S�X���b�h�̒T���m�[�h��
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
	
	//����S�X���b�h�����萶��������͖̂��ʂ����...
	//pos���Q�Ɠn���ł��Ȃ���...
	for (Thread*th : Threadpool) {
		th->set(pos);
	}

	main()->start_searching();

}



