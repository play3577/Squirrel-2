#pragma once
/*
���K�͊����w�K�p�̊֐��������Ő錾����

�����̕]���l�����t�ɂ��Ċw�K���s���̂ł�����ċ����w�K�H�H�H�Ƃ͎v������ǂق��̋����\�t�g�������w�K���Č����Ă�̂ŋ����w�K�Ȃ񂾂낤�B
�i�������������w�K�����Ȃ̂��킩���Ă��Ȃ��j


�܂��͋��t�f�[�^�����B
���t�f�[�^�𐶐�����ɂ͂��̃f�[�^�\�������߂Ȃ���΂Ȃ�Ȃ��B
�w�K�ɕK�v�Ȃ̂�
���̋ǖʂ�sfen�܂��̓n�t�}����sfen�Ɛ�����ǂ񂾎��̕]���l
sfen�̓n�t�}��������256bit �]���l��16bit�ł���̂Ł@272bit����΂悢(padding���Ȃ����Ȃ��Ƃ����Ȃ���...)
������N���X�Ƃ��ėp�ӂ��ăt�@�C���Ƀo�C�i���`���ŏ����o���Ă����B
����ŉ��Ƃ��Ȃ�Ǝv����B

�����Ċ����ɏo�Ă���ǖʂɃ����_�������������邽�߂ɁA
�`�F�X960�݂����ɏ����ǖʃ����_���f�[�^�x�[�X�����K�v������B

�]���l��2000�𒴂���Ύ�����ł��؂�

���t�f�[�^�쐬���ɂ�
���̎�ɐi�ނ��߂̍�����ɂ������_��������������B
���S�Ƀ����_���ł�����Ȃ�ɗǂ���������Ȃ����A30�����炢�ɂ��Ă���������


�w�K�������͂�ÓI�Ɏ��ĂȂ��Ȃ��Ă��܂����̂Ŋw�K���N���X�������
�����ɕK�v�ȃf�[�^�S���˂�����ł���𓮓I�Ɋm�ۂ���݌v�ɂ��Ȃ���΂Ȃ�Ȃ��I�I�I�I�I�I�I�I


*/
#include <fstream>
#include <sstream>
#include "learner.h"


#if defined(REIN) || defined(MAKETEACHER)
struct teacher_data {

	//bool haffman[256];

	string sfen;//�n�t�}�����s���܂������̂�string�ōs���B�@���I����Ď��Ԃ��ł������n�t�}���ϊ��֐��̃f�o�b�O���邩...

				//
	int16_t teacher_value;
	uint16_t move;
	uint16_t gameply;
	bool isWin;
	uint8_t padding;
	//teacher_data() {};
	/*
	elmo�����g���Ȃ炱���ɏ���
	qhapak���ŋ��t��������l�̍����������Ⴍ���邽�߂ɂ͍�������܂߂Ȃ���΂Ȃ�Ȃ�

	���s�����邱�Ƃ�
	����悾���̋Ǐ��œK�����h����iNDF�̋��򂳂�j
	�]���l�Ə�������������Ă����^���w�K�������Ƃł���悤�ɂȂ�i���p�b�N���V�c����j
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

//�w�K�p�A�f�[�^�쐬�p�̃N���X�𓮓I�m�ۂ�����
class Make_Teacher {
private:
	vector<string> startpos_db;//�J�n�ǖʏW
	vector<vector<teacher_data>> teachers;//thread���Ƃ̍쐬�������t�ǖʃf�[�^�B
	vector<teacher_data> sum_teachers;//�Ō�ɂ�����thread���Ƃɍ쐬�������t�ǖʃf�[�^���܂Ƃ߂Ă�����

	uint64_t sumteachersize = 0;
	int maxthreadnum__;


	//------------------------------------------------maltithread����randam�J�n�ǖ�database��index�Ƃ����t�f�[�^��index�Ƃ��Ɏg��
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

	vector<teacher_data> sum_teachers;//�Ō�ɂ�����thread���Ƃɍ쐬�������t�ǖʃf�[�^���܂Ƃ߂Ă�����
	int maxthreadnum__;

	vector<string> teacher_list;
	int listcounter = 0;



	//------------------------------------------------maltithread����randam�J�n�ǖ�database��index�Ƃ����t�f�[�^��index�Ƃ��Ɏg��
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