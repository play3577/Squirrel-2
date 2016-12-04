#pragma once

#include "fundation.h"
#include "position.h"
#include <unordered_map>
#include <string>
#include <chrono>
#include <thread>

using namespace std;


string itos(int number);

//sfenの文字列を駒の種類に変換するためのマップ
//unorderd_map ...mapより高速であるが順序付けはされていない
class Sfen2Piece :public std::unordered_map<char, Piece> {

public:
	//成り駒が入ってないやん！(まあusaの場合は外側に+があるかどうかで処理できるか...)
	Sfen2Piece() {

		(*this)['P'] = B_PAWN;
		(*this)['L'] = B_LANCE;
		(*this)['N'] = B_KNIGHT;
		(*this)['S'] = B_SILVER;
		(*this)['B'] = B_BISHOP;
		(*this)['R'] = B_ROOK;
		(*this)['G'] = B_GOLD;
		(*this)['K'] = B_KING;

		(*this)['p'] = W_PAWN;
		(*this)['l'] = W_LANCE;
		(*this)['n'] = W_KNIGHT;
		(*this)['s'] = W_SILVER;
		(*this)['b'] = W_BISHOP;
		(*this)['r'] = W_ROOK;
		(*this)['g'] = W_GOLD;
		(*this)['k'] = W_KING;

	}

	bool is_ok(const char psuedo_piece) const {
		return (this->find(psuedo_piece) != this->end());
	}

	Piece sfen_to_piece(char piece) const {
		return this->find(piece)->second;
	}

};

//CSA
/*

（移動元（file）(Rank)）（移動先(file)(Rank)）(駒種)というようになっている
2824HI
8586FU
8786FU
8286HI
5958OU
8676HI
8877KA
5152OU
2434HI
7626HI
0028FU
6172KI
3736FU
0083FU
2937KE
2277UM

*/
class CSA2Piece :public std::unordered_map<string, Piece> {

public:
	CSA2Piece() {
		//成り駒が入ってないやん(こっちは問題)
		(*this)["FU"] = PAWN;
		(*this)["KY"] = LANCE;
		(*this)["KE"] = KNIGHT;
		(*this)["GI"] = SILVER;
		(*this)["KA"] = BISHOP;
		(*this)["HI"] = ROOK;
		(*this)["KI"] = GOLD;
		(*this)["OU"] = KING;
		(*this)["TO"] = PRO_PAWN;
		(*this)["NY"] = PRO_LANCE;
		(*this)["NK"] = PRO_NIGHT;
		(*this)["NG"] = PRO_SILVER;
		(*this)["UM"] = UNICORN;
		(*this)["RY"] = DRAGON;
	}

	bool is_ok(const string psuedo_piece) const {
		return (this->find(psuedo_piece) != this->end());
	}

	Piece csa_to_piece(string piece) const {
		return this->find(piece)->second;
	}

};

extern CSA2Piece CSA2Piece_;

Move Sfen2Move(const string smove, const Position& pos);
Move CSA2Move(const string smove, const Position& pos);

//
//
//時間計測関連

typedef std::chrono::milliseconds::rep TimePoint;

inline TimePoint now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void sleep(int ms)
{
	std::this_thread::sleep_for(std::chrono::microseconds(ms));
}

extern Sfen2Piece Sfen2Piece_;



//評価値を勝率に変換するための関数
//この数式はponanzaに習っている。
inline double eval2rate(double eval) {
	const double a = 1.0 / 600.0;
	return (1.0) / (1.0 + exp(-a*eval));
}
//シグモイド関数
inline double sigmoid(const double x) {
	return (1.0) / (1.0 + exp(-x));
}

//シグモイド関数の微分
/*
FVWindowと係数をかけてやる必要がある。
*/
inline double dsigmoid(const double x) {
	return sigmoid(x) * (1.0 - sigmoid(x));
}

inline double normal_dist(double mean, double stddiv);

