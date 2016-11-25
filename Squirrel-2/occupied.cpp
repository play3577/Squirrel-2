#include "occupied.h"

int indexPlus45[SQ_NUM];


void make_indexplus45() {

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		int s = sq % 10;
		if (s == 0 || s == 9 || s == 8 || s == 7 || s == 6 || sq == 5 || sq == 15 || sq == 25 || sq == 35) {

			indexPlus45[sq] = 0;

		}
		else {
			indexPlus45[sq] = 1;
		}

	}

}

int shiftPlus45[SQ_NUM];


void make_shiftplus45() {

	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		//10で割ったあまりの値で大体は区別できる
		int s = sq % 10;
		
		if (s==0) {
			shiftPlus45[sq] = 1;
		}
		else if(s==9){
			shiftPlus45[sq] = 2 + 9 * 1;
		}
		else if (s == 8) {

			//これは端っこであるのでどのようなobstacleであろうが機器の数はゼロ
			if (sq == 8) {
				shiftPlus45[sq] = 9 * 1;
			}
			else {
				shiftPlus45[sq] = 3 + 9 * 2;
			}

		}
		else if (s == 7) {

			if (sq <= 17) {
				shiftPlus45[sq] = 9 * 2;
			}
			else {
				shiftPlus45[sq] = 4 + 9 * 3;
			}
		}
		else if (s == 6) {
			if (sq <= 26) {
				shiftPlus45[sq] = 1 + 9 * 3;
			}
			else {
				shiftPlus45[sq] = 5 + 9 * 4;
			}
		}
		else if (s == 5) {
			if (sq <= 35) {
				shiftPlus45[sq] = 1 + 9 * 4;
			}
			else {
				shiftPlus45[sq]= 6;
			}
		}
		else if (s == 4) {
			if (sq <= 44) {
				shiftPlus45[sq] = 1;
			}
			else {
				shiftPlus45[sq] = 7 + 9 * 1;
			}
		}
		else if (s == 3) {
			shiftPlus45[sq] = 1 + 9 * 1;
			//63 73の場合は別に場合分けする必要はない
		}
		else if (s == 2) {
			//72の場合も考えられるが72は一番端っこであるため利きはゼロに成るので無視していい
			shiftPlus45[sq] = 1 + 9 * 2;
		}
		else if (s == 1) {
			shiftPlus45[sq] = 1 + 9 * 3;
		}
	}

}


int indexMinus45[SQ_NUM] = {

	0,0,0,0,1,1,1,1,0,//8
	0,0,0,1,1,1,1,0,0,//17
	0,0,1,1,1,1,0,0,0,//26
	0,1,1,1,1,0,0,0,0,//35
	1,1,1,1,0,0,0,0,0,//44
	1,1,1,0,0,0,0,0,1,//53
	1,1,0,0,0,0,0,1,1,//62
	1,0,0,0,0,0,1,1,1,//71
	0,0,0,0,0,1,1,1,1,


};

int shiftMinus45[SQ_NUM];

//
//void make_indexMinus45()
//{
//
//
//}

void make_shiftMinus45()
{
	for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

		int s = sq % 8;

		if (s == 0) {
			if (sq == 0) {
				shiftMinus45[sq] = 0;
			}
			else if (sq == 80) {
				shiftMinus45[sq] = 0;
			}
			else {
				shiftMinus45[sq] = 1;
			}
		}
		if (s == 1) {
			if (sq <= 9) {
				shiftMinus45[sq] = 0;
			}
			else {
				shiftMinus45[sq] = 2 + 9 * 1;
			}
		}
		if (s == 2) {
			if (sq <= 18) {
				shiftMinus45[sq] = 1 + 9 * 3;
			}
			else {
				shiftMinus45[sq] = 3 + 9 * 2;
			}
		}
		if (s == 3) {
			if (sq <= 27) {
				shiftMinus45[sq] = 1 + 9 * 4;
			}
			else {
				shiftMinus45[sq] = 4 + 9 * 3;
			}
		}
		if (s == 4) {
			if (sq <= 36) {
				shiftMinus45[sq] = 1;
			}
			else {
				shiftMinus45[sq] = 5 + 9 * 4;
			}
		}
		if (s == 5) {
			if (sq <= 45) {
				shiftMinus45[sq] = 1 + 9 * 1;
			}
			else {
				shiftMinus45[sq] = 6;
			}
		}
		if (s == 6) {
			if (sq <= 54) {
				shiftMinus45[sq] = 1 + 9 * 2;
			}
			else {
				shiftMinus45[sq] = 7 + 9 * 1;
			}
		}
		if (s == 7) {
			if (sq <= 63) {
				shiftMinus45[sq] = 1 + 9 * 3;
			}
			else {
				shiftMinus45[sq] = 0;
			}
		}
			
	}

}



int shift_table_tate[SQ_NUM];
int index_table_tate[SQ_NUM];

int shift_table_yoko[SQ_NUM];
int index_table_yoko[SQ_NUM];