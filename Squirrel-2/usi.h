#pragma once
#include "fundation.h"
#include <string>
#include <map>



using namespace std;

namespace USI {

	struct USIOption;

	typedef std::map<string, USIOption> OptionMap;

	//�Q�lstockfish
	/// Option class implements an option as defined by UCI protocol
	struct USIOption {

	private:
		friend std::ostream& operator<<(std::ostream&, const USI::OptionMap&);//���̊֐���idx��Value�Ȃǂɒ��ڊւ���悤�ɂ��̊֐����t�����h��
		size_t idx;
		string value, type, defalt;
		int min, max;

	public:
		/*=====================
		�R���X�g���N�^
		======================*/
		//bool�l��USI
		//ponder_on�Ƃ�����Ȃ�
		USIOption(bool config) :type("button"), min(0), max(0) {
			if (config == true) { value = "true"; }
			else { value = "false"; }
		}
		//�X���b�h���Ƃ�����Ȋ����̂��
		USIOption(int v, int min_, int max_) :type("spin"), min(min_), max(max_) {
			value = std::to_string(v);
		}
		//�]���֐��o�C�i���t�@�C���p�X�Ƃ�����Ȃ�
		USIOption(const char *v) :type("string"), min(0), max(0) {
			value = v;
		}
		USIOption() {};
		//USIOption& operator=(const std::string&);

		// operator<<() inits options and assigns idx in the correct printing order
		//OPtion�����������ĕ\���̏��Ԃɕ��ׂ�
		void operator<<(const USIOption& o) {

			static size_t insert_order = 0;

			*this = o;
			idx = insert_order++;
		}

		string str() {
			ASSERT(type == "string");
			return value;
		}

		/*======================
		�l��ύX���邽�߂ɗp����B
		=======================*/
		void change(bool change) {
			ASSERT(type == "button");
			if (change == true) { value = "true"; }
			else { value = "false"; }
		}
		void change(int v) {
			ASSERT(type == "spin");
			value = std::to_string(v);
		}
		void change(const char *v) {
			ASSERT(type == "string");
			value = v;
		}
		//type��Ԃ��B
		string return_type()const {
			return type;
		}

		/*����������������������
		�l��Ԃ����߂ɗp����B
		����������������������*/
		operator int() const {
			ASSERT(type == "spin" || type == "button");
			if (type == "spin") {
				return stoi(value);
			}
			else {
				return value == "true";
			}
		}
		operator string()const {
			ASSERT(type == "string");
			return value;
		}

	};

	void init_option(OptionMap& o,string engine_name);
	void loop();


};

extern USI::OptionMap Options;