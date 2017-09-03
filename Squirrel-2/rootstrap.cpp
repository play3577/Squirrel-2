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
void renewal_PP_nozomi(dJValue &data);//nozomi����ɋ���������@
//�����_���J�n�ǖʂ���1�胉���_���ɂ������邽�߂Ɏg��
void do_randommove(Position& pos, StateInfo* s, std::mt19937& mt);
#endif

#if defined(LEARN) && defined(MAKETEACHER)
/*---------------------------------------------------------------------------
�����̕]���l�Ƌǖʂ̑g���Z�b�g�ɂ������t�f�[�^���쐬���邽�߂̊֐�
����͏�������thread�̎��܂Ƃ߂̂��߂̊֐��ł���̂�
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

	//�J�n�ǖʃf�[�^�x�[�X�ǂݍ���
	/*
	�o�Ă��Ȃ����傭�߂�΂��肾�����\������Ȃ̂Œ�Ղ���ǂݍ��܂���
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
	//���t�f�[�^�i�[�ɗp��

	//GCC�őS�R���܂������Ȃ�..................(�L��֥�M)
	maxthreadnum__ = omp_get_max_threads();
	//maxthreadnum__ = 4;
	for (size_t i = 0; i < maxthreadnum__; i++) {
		teachers.emplace_back();
	}
	cout << "teachers" << teachers.size() << endl;
	//�J�n�ǖʃf�[�^�x�[�X�V���b�t�� 
	std::random_device rd;
	std::mt19937 g_mt(rd());
	//�X���b�h�쐬
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
		//teacher�̍쐬
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
		//thread����teacher��merge
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
		//�Ȃ񂩃V���b�t�����Ȃ��ق��������Ƃ�������������̂ŃV���b�t�����Ȃ��ق��������̂��H�܂����ʂɍl������minbatch�ɓ����Ă���ǖʂ̃����_�����������ق��������̂ŃV���b�t�����ׂ����Ǝv���̂���...
		std::printf("shuffle teacherdata\n");
		std::shuffle(sum_teachers.begin(), sum_teachers.end(), g_mt);


		std::printf("write teacher_data\n");
		sumteachersize += sum_teachers.size();
		cout << "writed teacher num:" << sumteachersize << endl;
		//�ӂ�����ɏ����o���i�㏑���j
		/*
		�ǂݍ��ނƂ���vector����Ƃ��Ă��āA�����pushback���Ă����΂����ƍl������̂���
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
		//�Ȃ���������̂Ŗ����̒��`�F�b�N������
	}


	cout << "finish!" << endl;
}

void Make_Teacher::make_teacher_body(const int number) {

	cout << number << endl;
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> random_move_probability(0.0, 1.0);//double�̂ق��������₷����
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

		if (startposdb_string.size() < 10) { continue; }//������̒��������肦�Ȃ��قǒZ���̂̓G���[�ł���̂Ŏg��Ȃ�
		pos.set(startposdb_string);//random�J�n�ǖʏW�������o����set����B�i���̃����_���J�n�ǖʂ͉�����o�Ă���̂ł��������蓮�������ق��������j
		do_randommove(pos, &s_start, mt);//random�����ǖʂ���1�胉���_���ɐi�߂Ă���
		th.cleartable();

		random_probability = 1.0;

		for (int i = 0; i < 300; i++) {

			//�T���O��thread�����������Ă���
			Eval::eval_PP(pos);//�����v�Z�Ńo�O���o���Ȃ����߂Ɍv�Z���Ă���



			th.set(pos);
			//th.cleartable();//������Ƃ���������������������̂��낤���H�H

			//�Z�INDF�͐[���Œ�ł͂Ȃ��b���Œ�B
			//�b���Œ�̂ق�������������ɂȂ�Ȃ��̂ł����炵���B
			th.l_depth = DEPTH;
			th.l_alpha = -Value_Infinite;
			th.l_beta = Value_Infinite;
			Value v = th.think();//�T�������s���� ����͎�ԑ����猩���]���l�ł��� �ǂݔ�������������������Ȃ��̂Ŏ}�����}�����T�����s���ׂ����H
			if (abs(v) > 3000) { goto NEXT_STARTPOS; }//�]���l��3000�𒴂��Ă��܂����ꍇ�͎��̋ǖʂֈڂ�
			/*if (th.pv.size() > 6) {
				cout << th.pv.size() << endl;
			}*/
			//pos.pack_haffman_sfen();
			//root�ǖʂ�haffman������p�ӂ��Ă���
		/*	bool HaffmanrootPos[256];
			memcpy(HaffmanrootPos, pos.packed_sfen, sizeof(HaffmanrootPos));*/
			string sfen_rootpos = pos.make_sfen();//root�ǖʂ�sfen
			//pos.pack_haffman_sfen();
			//------------------------------PV�̖��[�̃m�[�h�Ɉڂ��Ă����ł̐Î~�T���̒l������teacher_data�Ɋi�[
			/*--------------------------------------------------------------------------------------------------------
			pv�̖��[�Ɉړ������Ȃ������ꍇdeepvalue���T���̒l�Ƃ�������Ă��܂��Ƃ������Ƃ��N�������I����ŏ����͂܂��ɂȂ�
			--------------------------------------------------------------------------------------------------------*/
			//#define GOLEAF//USETT���Ă�̂�GLEAF�͂ł��Ȃ�
			Value deepvalue = v;
#if defined(GOLEAF)
			StateInfo si2[64];
			for (int i = 0; i < 64; i++) si2[i].clear();
			int pv_depth = 0;
			const Color rootColor = pos.sidetomove();//HaffmanrootPos�̎��

			//pos.do_move(th.RootMoves[0].move, &si2[pv_depth++]);
			for (Move m : th.pv) {
				if (pos.is_legal(m) == false || pos.pseudo_legal(m) == false) { ASSERT(0); }
				pos.do_move(m, &si2[pv_depth++]);
			}
			//root���猩���]���l���i�[����
#ifdef EVAL_KPP
			pos.state()->sumBKPP = Value_error; pos.state()->previous->sumBKPP = Value_error;
#elif defined(EVAL_PP)
			pos.state()->bpp = pos.state()->wpp = Value_error;//�����v�Z�𖳌��ɂ��Ă݂�
			pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error;
#elif defined(EVAL_PPT)
			pos.state()->bpp = pos.state()->wpp = Value_error;//�����v�Z�𖳌��ɂ��Ă݂�
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
			//���̍������I�ʁB(���@�肪�I�΂��܂łȂ�ǂ��J��Ԃ��K�v������)
			memset(moves, 0, sizeof(moves));//������
			end = test_move_generation(pos, moves);
			ptrdiff_t num_moves = end - moves;
			Move m;



			while (true) {
				//���̋ǖʂɐi�߂邽�߂̎w����̑I��
				//���@�肪�I�������܂�while�ŉ��

				//���肪�������Ă��Ȃ��ꍇ�A40�p�[�Z���g�Ń����_���̎w����Ŏ��ǖʂ�
				if (!pos.is_incheck()
					&& random_move_probability(rd) < random_probability)
				{
					random_probability /= 1.5; m = moves[mt() % num_moves];
				}
				else {
					//60�p�[�Z���g��pv
					m = th.RootMoves[0].move;
				}

				if (pos.is_legal(m) && pos.pseudo_legal(m)) { break; }
			}
			//������Ŏ��̋ǖʂɈړ�����B
			pos.do_move(m, &si[i]);

		}
	NEXT_STARTPOS:;
	}

}

//�����Z���k���X�^�̃R���X�g���N�^�͒x���̂ŎQ�Ƃœn��
void do_randommove(Position& pos, StateInfo* s, std::mt19937& mt) {


	if (pos.is_incheck()) { return; }

	ExtMove moves[600], *end;

	memset(moves, 0, sizeof(moves));//�w����̏�����
	end = test_move_generation(pos, moves);//�w���萶��
	ptrdiff_t num_moves = end - moves;

	Move m;

	while (true) {
		//���̋ǖʂɐi�߂邽�߂̎w����̑I��
		//���@�肪�I�������܂�while�ŉ��

		m = moves[mt() % num_moves];


		if (pos.is_legal(m) && pos.pseudo_legal(m)) { break; }
	}
	pos.do_move(m, s);

}


#endif

#if defined(REIN) 
//�萔
int read_teacher_counter = 0;
int batchsize = 1000000;//nozomi�����minbatch100���ł������Č����Ă�
double loss = 0;

//ok
//�Q�l�@http://gurigumi.s349.xrea.com/programming/binary.html
/*
���̕��@�ŕ������ǂ����Ƃ���ƍŏ��̕������������ǂݒ����Ă��܂��I�I
ifstream���O������Q�Ɠn�����邱�Ƃɂ���

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


	//read_teacherdata();//�����ŋ��t�f�[�^��ǂݍ���

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
		if (sys::is_regular_file(p)) { // �t�@�C���Ȃ�...
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






	//gradJ�i�[�ɗp��
	maxthreadnum__ = omp_get_max_threads();
	for (size_t i = 0; i < maxthreadnum__; i++) {
		gradJs.emplace_back();
	}
	//�X���b�h�쐬
	vector<std::thread> threads(maxthreadnum__ - 1);

	//�����iteration���񂵂Ă������㕨�ł͂Ȃ����낤
	read_teacher_counter = 0;
	//������
	sum_gradJ.clear();
	for (dJValue& dJ : gradJs) { dJ.clear(); }

	//�w�K�J�n(readteacherdata�Ńo�b�`�T�C�Y����������ǂݍ���Ń~�j�o�b�`�w�K���s��)
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

		//�l�X�ȊJ�n�ǖʂ���̑�ʂ̃f�[�^������̂Ŏ��������͕K�v�Ȃ��ƍl������
		/*
		�l�̍X�V�̕��@��bonanza method�Ɠ����ł̓_���I�I�I
		�L�^���ꂽ�[���T���̒l���p�����[�^�[���ς���Ă���Ƃ���͐[���T���̒l�ł͂Ȃ��Ȃ��Ă���͂�
		�ƂȂ�ƑS���t�f�[�^��p���Ĉ�񂵂��l���X�V�ł��Ȃ��H�H�H�H�H���`�`��....����͂������ɂȂ��悤�ȋC������̂������....
		�l�̍X�V�̕��@��׋����Ȃ��Ƃ����Ȃ�...�茳��adadelta�̘_�������邵������������H�H
		*/

		//nozomi����H�����������͂��Ȃ��ق��������Ƃ̂��ƁB
		//#ifdef JIGENSAGE
		//		lowdim_.clear();
		//		lower__dimPP(lowdim_, sum_gradJ);
		//		sum_gradJ.clear();
		//		weave_lowdim_to_gradj(sum_gradJ, lowdim_);
		//#endif




				//renewal_PP_rein(sum_gradJ);//nan�������ς������Ă��Ă����ƒl�̍X�V���ł��Ă��Ȃ������̂ŋ������ς��Ȃ������^�f�B
		renewal_PP_nozomi(sum_gradJ);//�������͊m���ɒl�����Ă���ƍl������B


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
����ȑO��unpacksfen�͒ʏ�ɓ��삵�Ă���H�H
���̕Ӄo�O���Ă����A����̂��߂�sfen�ŏo�͂��邱�Ƃɂ���
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

		const Color rootColor = pos.sidetomove();//rootcolor��pos set������O�ɗp�ӂ��Ă��܂��Ă����I
		/*
		http://www2.computer-shogi.org/wcsc27/appeal/Apery/appeal_wcsc27.html
		��{�I�ɂ�6��ǂ݂Ő��\���ǖʂɓ_����t���A������0~1��ǂ�(�Î~�T���͊��S�ɍs��)�œ_�����ߕt����悤�ɃI�����C���w�K���s���܂�

		Apery�͂��������Ă邵�A���q�T���͂����ق��������̂���...??�܂��m���ɓ��I�ȋǖʂ̐ÓI�]���l�Ȃ�ĐM���Ȃ�񂵕K�v���H
		�悭�l�����炤���̃\�t�g�̐Î~�T���͋�̎�荇���������Ă��Ȃ��̂ŐÎ~�T������Ă��Ӗ��Ȃ���...
		�i�Î~�T����recapture�ȊO����ꂽ��キ�Ȃ���������̓o�O������������Ȃ̂�������Ȃ���...������񎎂��Ă݂��ق����������j

		1��T�����炢�͂����ق��������̂�������Ȃ�


		�Î~�T���̖��[�̒l���X�V����̂ɂ͋^����o����̂ł��Ƃŕς��Ă݂�


		*/
		//const Value shallow_v = Eval::eval(pos);
#if 0
		th.set(pos);
		//th.cleartable();//����͎��ʂقǒx��
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		th.l_depth = 2;
		//Eval::eval(pos);
		Value shallow_serch_value = th.think();
		if (abs(shallow_serch_value) > 3000 || shallow_serch_value == 0) { continue; }

		int ii = 0;
		for (Move m : th.pv) { pos.do_move(m, &si[ii]); ii++; }//pv�̖��[�ֈړ�
#elif 0
		th.set(pos);
		//th.cleartable();//����͎��ʂقǒx��
		th.l_alpha = -Value_Infinite;
		th.l_beta = Value_Infinite;
		Value shallow_serch_value = th.Qsearch();
		//if (abs(shallow_serch_value) > 3000 ) { continue; }

		int ii = 0;
		//ASSERT(th.pv[0]==th.RootMoves[0].move) //���`�`�񂿂���PV�����Ă邩��������...


		for (Move m : th.pv) {
			if (!pos.pseudo_legal(m) || !pos.is_legal(m)) {
				cout << pos << endl;
				th.print_pv(0, shallow_serch_value);
				ASSERT(0);
			}
			pos.do_move(m, &si[ii]); ii++;
		}//pv�̖��[�ֈړ�



#endif

#ifdef EVAL_KPP
		pos.state()->sumBKPP = Value_error; pos.state()->previous->sumBKPP = Value_error;
#elif defined(EVAL_PP)
		pos.state()->bpp = pos.state()->wpp = Value_error;//�����v�Z�𖳌��ɂ��Ă݂�
		if (pos.state()->previous != nullptr) { pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error; }
		//previous��valueerror�ɂ���̂�Y��Ă���
#elif defined(EVAL_PPT)
		pos.state()->bpp = pos.state()->wpp = Value_error;//�����v�Z�𖳌��ɂ��Ă݂�
		if (pos.state()->previous != nullptr) { pos.state()->previous->bpp = pos.state()->previous->wpp = Value_error; }
#endif
		//root���猩���_���ɕϊ�����iteacher��root���猩���]���l�̂͂��j]


		Value shallow_v = (rootColor == pos.sidetomove()) ? Eval::eval(pos) : -Eval::eval(pos);//�����T���ŋA���Ă����l�ɂ��ׂ��H�i�悭�Ȃ������j
		//Value shallow_v = shallow_serch_value;

		double win_teacher = win_sig(teacher);
		double win_shallow = win_sig(shallow_v);
		double diffsig_ = win_shallow - win_teacher;//�����G���g���s�[ ����̂ق�����������nozomi���񂪌����Ă�

		/*
		PP�̕]���l�̌X�������炵��ponanza�̏����̎��͎g���Ȃ��Ǝv���ĕ]���l�̍������ɂ��Ă������A
		���̂܂܂̕]���l���ƍ����傫������Ƃ���̒l���x�z�I�ɂȂ��Ă��܂��̂ł�͂菟���ɕϊ����ׂ����H�H
		*/
		//double diffsig_ = (shallow_v - teacher);

		//loss += diffsig_*diffsig_;
		loss += pow(int(shallow_v - teacher), 2);



#ifdef EVAL_KPPT
		double diffsig_withoutturn = (rootColor == BLACK ? diffsig_ : -diffsig_);//+bpp-wpp�̊֌W
		double diffsig_turn = diffsig_;
		std::array<float, 2> diffsig = { -diffsig_withoutturn,(rootColor == pos.sidetomove() ? -diffsig_turn : diffsig_turn) };
		gradJs[index].update_dJ(pos, diffsig);
#else
		double diffsig = (rootColor == BLACK ? diffsig_ : -diffsig_);
		double diffsig_turn = (rootColor == pos.sidetomove() ? -diffsig_ : diffsig_);//rootcolor�܂��Ԃ������Ă��鑤�ɑ΂��ă{�[�i�X��^����
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

//Adadelta�������Ă݂�

void renewal_PP_rein(dJValue &data) {


	//PP_to_doublePP();
	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
	/*
	�ړI�֐��͏������Ȃ����̂����A�������キ�Ȃ����B
	�ړI�֐����������Ȃ����Ƃ������Ƃ͊w�K�ɂ͐������Ă���͂��ł���B
	���t�f�[�^�쐬�A�J�n�ǖʍ쐬���G�������A�n�t�}�������������܂������ĂȂ������Ƃ������Ƃ����邩������Ȃ����A�������񋭉��w�K�����痣��Ăق��̂Ƃ����M�낤�Ǝv���B
	*/
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {



			PP_double[i][j] = double(PP[i][j]);

			double gt = data.absolute_PP[i][j];
			double gt2 = gt*gt;
			double Egt = lastEg[i][j] = (row)*lastEg[i][j] + (1 - row)*gt2;
			double delta_x = gt*RMS(last_Edeltax[i][j]) / RMS(Egt);
			last_Edeltax[i][j] = (row)*last_Edeltax[i][j] + (1 - row)*delta_x;

			//FV_SCALE�ł����̂��H
			/*
			�������̂ق��������Ȃ����Ǝv�������z���nan�������Ă��Ēl�������Ă��Ȃ�����������

			*/
			if (delta_x == NAN) { continue; }
			int add = int(delta_x)*FV_SCALE;//clamp�����������ق����������H(�Ƃ�����PP����������double�ɕϊ����Ă����int�ɖ߂��ق����������H�H)
			if (abs(PP[i][j] + add) < INT16_MAX) {
				PP[i][j] += add;
			}

			////���̕��@�ł̓p�����[�^���S�R�����Ă��Ȃ��������`�`�`�`��I�����C���w�K�ł��n�߂邩�H�H
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
wcsc27��nozomi����ɋ����Ă������������@�B
���z�̕�����1�����������I�I�I�I�I�I�I�I
kpp�ȂǂɋǏ����͂��܂�Ȃ��Ƃ����̂������Ƃ��Ă킩�����炵���̂�0~2�܂ł͈̔͂œ������l��ς���Ȃ�Ă��Ȃ��Ă����݂����B
�Ȃ񂩐̂̓n�C�p�[�p�����[�^���낢��M�����炵���������ɗ����������悤��...

https://twitter.com/hillbig/status/868053156223004672
https://twitter.com/hillbig/status/869122361001246720
https://twitter.com/hillbig/status/869348563259490304

������݂�Ƃ������ɂ��̕��@�łȂ񂾂��񂾂�����悤�ȋC������
�������������l��1�����Ȃ̂Ŋw�K�f�[�^�͂����ς��ق���
�i1�����ł͂��邪�ꍇ�ɂ���Ă�1���傫���Ƃ������Ƃ����邩������Ȃ����j
*/
void renewal_PP_nozomi(dJValue &data) {



	int h;

	h = 1;
#ifdef EVAL_PP
	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
	for (BonaPiece i = f_hand_pawn; i < fe_end2; i++) {
		for (BonaPiece j = f_hand_pawn; j < fe_end2; j++) {

			int inc = h*sign(data.absolute_PP[i][j]);
			if (abs(PP[i][j] + inc) < INT16_MAX) {
				PP[i][j] += inc;
			}

		}
	}
#elif defined(EVAL_PPT)
	//�Ώ̐���dJ�̒��Ɋ܂܂�Ă���̂ł����ł͍l���Ȃ��Ă���
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
����clear table����ƂقƂ�ǂ̕]���l�̌덷�͔��ɏ������͈͂Ɏ��܂���.���������1�������Ȃ�
���������񏉊������ĒT�����s���Ǝ��Ԃ�n���݂����ɐH��...�܂����񏉊����͋��t�f�[�^�쐬���ɂ͂��Ȃ��Ă������낤�B



AperyWCSC26�̋��t�f�[�^�t�@�C���ǂݍ��݂ɊԈႢ�͂Ȃ�����
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
			Value v = th.think();//�T�������s����

								 //table�̒l�̕t�������ς���Ă��肷�邩������Ȃ��̂�100�͋��e����i����ł��傫�����j
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