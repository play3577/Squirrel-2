#pragma once

#include "fundation.h"
#include "position.h"
#include <unordered_map>
#include <string>
#include <chrono>
#include <thread>

using namespace std;

//sfenの文字列を駒の種類に変換するためのマップ
//unorderd_map ...mapより高速であるが順序付けはされていない
class Sfen2Piece :public std::unordered_map<char, Piece> {

public:
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

Move Sfen2Move(const string smove, const Position& pos);

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


//評価値を勝率に変換するための関数
//この数式はponanzaに習っている。
inline double eval2rate(double eval) {
	const double a = 1.0 / 600.0;
	return (1.0) / (1.0 + exp(-a*eval));
}


extern Sfen2Piece Sfen2Piece_;

