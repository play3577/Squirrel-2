#include "position.h"
#include "misc.h"
/*
no_piece,B_PAWN=1, B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING,
B_PRO_PAWN, B_PRO_LANCE, B_PRO_NIGHT, B_PRO_SILVER, B_UNICORN, B_DRAGON,
W_PAWN=17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING,
W_PRO_PAWN, W_PRO_LANCE, W_PRO_NIGHT, W_PRO_SILVER, W_UNICORN, W_DRAGON,PC_ALL,

*/

//これにプラスして
//先手駒はｙ＝０
//なっていない駒はx=0
//という符号をつける
int board_haffman[KING] = {
	0,//空き
	(0b10),//PAWN
	(0b1100),//LANCE
	(0b1101),//KNIGHT
	(0b1110),//SILVER
	(0b111110),//BISHOP
	(0b111111),//ROOK
	(0b1111),//gold
};

int board_counter[KING] = {
	1,//空き
	2,//PAWN
	4,//LANCE
	4,//KNIGHT
	4,//SILVER
	6,//BISHOP
	6,//ROOK
	6,//gold
};


int hand_haffman[KING] = {
	-129,
	(0b00),//PAWN
	0b1000,//LANCE
	0b1010,//KNIGHT
	0b1100,//SILVER
	0b111100,//BIshop
	0b111110,//ROOK
	0b1110,//GOLD
};

int hand_counter[KING] = {
	-129,
	2,//PAWN
	4,//LANCE
	4,//KNIGHT
	4,//SILVER
	6,//BIshop
	6,//ROOK
	5,//GOLD
};

string board_haffman_str[KING] = {
	"0",//空き
	"10",//PAWN
	"1100",//LANCE
	"1101",//KNIGHT
	"1110",//SILVER
	"111110",//BISHOP
	"111111",//ROOk
	"1111",//gold(あえて0を消してpromoteで0を足す)しかしこの方法だとunpackの時におかしくなる
};


string hand_haffman_str[KING] = {
	"",
	"00",//PAWN
	"1000",//LANCE
	"1010",//KNIGHT
	"1100",//SILVER
	"111100",//BIshop
	"111110",//ROOK
	"1110",//GOLD
};

//http://qiita.com/fantm21/items/a89884c8263db352aa4e
int binary(int bina) {
	int ans = 0;
	for (int i = 0; bina>0; i++)
	{
		ans = ans + (bina % 2)*pow(10, i);
		bina = bina / 2;
	}
	return ans;
}

//http://webkaru.net/clang/binary-to-decimal/
int64_t decimal(int64_t binary) {
	int64_t decimal = 0;
	int64_t base = 1;
	while (binary>0) {
		decimal = decimal + (binary % 10) * base;
		binary = binary / 10;
		base = base * 2;
	}
	return decimal;
}

//バイナリデータとして書き込むのかなぁ？？
string Position::pack_haffman_sfen(){

	//stringで確保して後でintに変換するか(´･ω･｀)
	string psfen;

	//手番
	sidetomove() != BLACK ? psfen += "1" : psfen += "0";
	//玉の位置
	string s="0000000"+ itos((int)binary(ksq(BLACK)));
	string s2 = s.substr(s.size() - 7, s.size() - 1);
	psfen += s2;//ここで7bitにあわせなければならない。
	s = "0000000" + itos((int)binary(ksq(WHITE)));
	s2 = s.substr(s.size() - 7, s.size() - 1);
	psfen += s2;
	//盤面
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		const Piece pc = piece_on(sq);
		const Color c = piece_color(pc);
		const Piece pt = rowpiece(piece_type(pc));
		//const int promote = (int)is_promote_piece(pc);
		const int promote = (piece_type(pc)>KING);

		if (piece_type(pc) == KING) { continue; }
		//if (pt == KING) { continue; }//玉のいるマスはこのパートには含めない。

		if (pc == NO_PIECE) { psfen += "0"; }
		else { ASSERT(NO_PIECE < pt&&pt < KING); psfen += (board_haffman_str[pt] + itos(promote) + itos(c)); }//1,0なのでitosだけでいいはず
		//cout <<"pc "<<pc <<"psfen " << psfen.size() << endl;
	}

	//手駒
	for (Color c = BLACK; c < ColorALL; c++) {

		const Hand hand_ = hand(c);
		for (Piece pt = PAWN; pt < KING; pt++) {

			int num = num_pt(hand_, pt);
			if (num != 0) {

				for (int n = 0; n < num; n++) {
					psfen += (hand_haffman_str[pt]+itos(c));
				}
			}
		}
	}
	cout << psfen << endl;
	ASSERT(psfen.size() == 256);
	//ここからintに直す。
	//int64_t packed_sfen[4];

	//packed_sfen[3] =  stoi(psfen.substr(0, 64),nullptr,2);
	//packed_sfen[2] = stoi(psfen.substr(64, 127), nullptr, 2);
	//packed_sfen[1] = stoi(psfen.substr(128, 191), nullptr, 2);
	//packed_sfen[0] = stoi(psfen.substr(192), nullptr, 2);
	bool packed_sfen[256];

	for (size_t i = 0; i < 256; i++) {

		if (psfen[i] == '0') {
			packed_sfen[i] = 0;
		}
		else {
			packed_sfen[i] = 1;
		}

	}

	unpack_haffman_sfen(packed_sfen);

	return psfen;
}

string board_unhaffman_str[KING] = {
	"0",//空き
	"10",//PAWN
	"1100",//LANCE
	"1101",//KNIGHT
	"1110",//SILVER
	"111110",//BISHOP
	"111111",//ROOk
	"11110",//金
};


string hand_unhaffman_str[KING] = {
	"",
	"00",//PAWN
	"1000",//LANCE
	"1010",//KNIGHT
	"1100",//SILVER
	"111100",//BIshop
	"111110",//ROOK
	"1110",//GOLD
};

string	Position::unpack_haffman_sfen(bool *psfen_){


	clear();
	
	bool *psfen = psfen_;
	string nsfen;


	int index = 0;
	
	//手版と王の位置を忘れていた(;´･ω･)

	//手版
	psfen[index++] == false ? sidetomove_ = BLACK : sidetomove_ = WHITE;

	//王の位置
	string sbksq, swksq;
	for (; index < 8; index++) { sbksq += itos(psfen[index]); }
	for (; index <15 ; index++) { swksq += itos(psfen[index]); }
	Square bksq =(Square)decimal(stoi(sbksq));
	Square wksq = (Square)decimal(stoi(swksq));
	pcboard[bksq] = B_KING;
	pcboard[wksq] = W_KING;
	int sq = 0;
	//index = 16;
	//盤上
	
		string spiece;
		Color c;
		bool promote;
		Piece pc;
		while (true) {
			if (sq >= SQ_NUM) { break; }
			if (sq == bksq || sq == wksq) { sq++; }
			
			if (index > 256) {
				cout << *this << endl;
			}
			spiece += itos(psfen[index++]);
			
			for (Piece  i = NO_PIECE; i < KING; i++) {

				if (spiece == board_unhaffman_str[(int)i]) {
					if (i == GOLD) {
						c = (Color)psfen[index++];
						pcboard[sq] = add_color(i, c);
						spiece.clear();
						sq++;
						break;
					}
					else if (i == NO_PIECE) {
						pcboard[sq] = NO_PIECE;
						spiece.clear();
						sq++;
						break;
						
					}
					else {
						promote = psfen[index++];
						c= (Color)psfen[index++];
					    promote? pcboard[sq] = promotepiece(add_color(i, c)): pcboard[sq] = add_color(i, c);
						spiece.clear();
						sq++;
						break;
					}
				}
			}
		}
	
	//indexが256であれば持ち駒が存在しない
	//if (index == 256) { goto FINISH; }
		cout << *this << endl;
	//手駒
	string shand;
	//Color c;
	while (index < 256) {
		
		shand += itos(psfen[index++]);
		ASSERT(shand.size() <= 6);
		for (Piece i = PAWN; i < KING; i++) {
			if (shand == hand_unhaffman_str[(int)i]) {
				c = (Color)psfen[index++];
				makehand(hands[c], i, num_pt(hand(c), i) + 1);
				shand.clear();
			}
		}
	}

	cout << make_sfen() << endl;



FINISH:;



	return nsfen;
}