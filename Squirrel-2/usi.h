#pragma once
#include "fundation.h"
#include <string>
#include <map>



using namespace std;

namespace USI {

	struct USIOption;

	typedef std::map<string, USIOption> OptionMap;

	//参考stockfish
	/// Option class implements an option as defined by UCI protocol
	struct USIOption {

	private:
		friend std::ostream& operator<<(std::ostream&, const USI::OptionMap&);//この関数がidxやValueなどに直接関われるようにこの関数をフレンド化
		size_t idx;
		string value, type, defalt;
		int min, max;

	public:
		/*=====================
		コンストラクタ
		======================*/
		//bool値のUSI
		//ponder_onとかそんなん
		USIOption(bool config) :type("button"), min(0), max(0) {
			if (config == true) { value = "true"; }
			else { value = "false"; }
		}
		//スレッド数とかそんな感じのやつ
		USIOption(int v, int min_, int max_) :type("spin"), min(min_), max(max_) {
			value = std::to_string(v);
		}
		//評価関数バイナリファイルパスとかそんなん
		USIOption(const char *v) :type("string"), min(0), max(0) {
			value = v;
		}
		USIOption() {};
		//USIOption& operator=(const std::string&);

		// operator<<() inits options and assigns idx in the correct printing order
		//OPtionを初期化して表示の順番に並べる
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
		値を変更するために用いる。
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
		//typeを返す。
		string return_type()const {
			return type;
		}

		/*＝＝＝＝＝＝＝＝＝＝＝
		値を返すために用いる。
		＝＝＝＝＝＝＝＝＝＝＝*/
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