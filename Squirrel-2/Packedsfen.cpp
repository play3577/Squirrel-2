#include "position.h"
#include "misc.h"
#include <sstream>


//�����͂��܂������Ă���ۂ�
#if defined(REIN) || defined(MAKETEACHER)


/*
no_piece,B_PAWN=1, B_LANCE, B_KNIGHT, B_SILVER, B_BISHOP, B_ROOK, B_GOLD, B_KING,
B_PRO_PAWN, B_PRO_LANCE, B_PRO_NIGHT, B_PRO_SILVER, B_UNICORN, B_DRAGON,
W_PAWN=17, W_LANCE, W_KNIGHT, W_SILVER, W_BISHOP, W_ROOK, W_GOLD, W_KING,
W_PRO_PAWN, W_PRO_LANCE, W_PRO_NIGHT, W_PRO_SILVER, W_UNICORN, W_DRAGON,PC_ALL,
*/

//����Ƀv���X����
//����͂����O
//�Ȃ��Ă��Ȃ����x=0
//�Ƃ�������������
int board_haffman[KING] = {
	0,//��
	(0b10),//PAWN
	(0b1100),//LANCE
	(0b1101),//KNIGHT
	(0b1110),//SILVER
	(0b111110),//BISHOP
	(0b111111),//ROOK
	(0b1111),//gold
};

int board_counter[KING] = {
	1,//��
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
	"0",//��
	"10",//PAWN
	"1100",//LANCE
	"1101",//KNIGHT
	"1110",//SILVER
	"111110",//BISHOP
	"111111",//ROOk
	"1111",//gold(������0��������promote��0�𑫂�)���������̕��@����unpack�̎��ɂ��������Ȃ�
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

//�o�C�i���f�[�^�Ƃ��ď������ނ̂��Ȃ��H�H
void Position::pack_haffman_sfen(){

	//string�Ŋm�ۂ��Č��int�ɕϊ����邩(�L��֥�M)
	string psfen;

	//���
	sidetomove() != BLACK ? psfen += "1" : psfen += "0";

	//�ʂ̈ʒu
	string s="0000000"+ itos((int)binary(ksq(BLACK)));
	string s2 = s.substr(s.size() - 7, s.size() - 1);
	psfen += s2;//������7bit�ɂ��킹�Ȃ���΂Ȃ�Ȃ��B
	s = "0000000" + itos((int)binary(ksq(WHITE)));
	s2 = s.substr(s.size() - 7, s.size() - 1);
	psfen += s2;
	//�Ֆ�
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		const Piece pc = piece_on(sq);
		const Color c = piece_color(pc);
		const Piece pt = rowpiece(piece_type(pc));
		//const int promote = (int)is_promote_piece(pc);
		const int promote = (piece_type(pc)>KING);

		if (pc == B_KING||pc==W_KING) { continue; }
		//if (pt == KING) { continue; }//�ʂ̂���}�X�͂��̃p�[�g�ɂ͊܂߂Ȃ��B

		if (pc == NO_PIECE) { psfen += "0"; }
		else { ASSERT(NO_PIECE < pt&&pt < KING); psfen += (board_haffman_str[pt] + itos(promote) + itos(c)); }//1,0�Ȃ̂�itos�����ł����͂�
		//cout <<"pc "<<pc <<"psfen " << psfen.size() << endl;
	}

	//���
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
	//cout << psfen << endl;
	ASSERT(psfen.size() == 256);
	//��������int�ɒ����B
	//int64_t packed_sfen[4];

	//packed_sfen[3] =  stoi(psfen.substr(0, 64),nullptr,2);
	//packed_sfen[2] = stoi(psfen.substr(64, 127), nullptr, 2);
	//packed_sfen[1] = stoi(psfen.substr(128, 191), nullptr, 2);
	//packed_sfen[0] = stoi(psfen.substr(192), nullptr, 2);
	

	for (size_t i = 0; i < 256; i++) {

		if (psfen[i] == '0') {
			packed_sfen[i] = 0;
		}
		else {
			packed_sfen[i] = 1;
		}

	}

	//unpack_haffman_sfen(packed_sfen);

	//return psfen;
}

string board_unhaffman_str[KING] = {
	"0",//��
	"10",//PAWN
	"1100",//LANCE
	"1101",//KNIGHT
	"1110",//SILVER
	"111110",//BISHOP
	"111111",//ROOk
	"11110",//��
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


string itos_bool(bool number)
{
	/*stringstream ss;
	ss << number;
	return ss.str();*/
	return (number == true) ? "1" : "0";
}


void	Position::unpack_haffman_sfen(bool *psfen_){


	clear();
	
	bool *psfen = psfen_;
	string nsfen;


	int index = 0;
	
	//��łƉ��̈ʒu��Y��Ă���(;�L��֥)

	//���
	psfen[index++] == false ? sidetomove_ = BLACK : sidetomove_ = WHITE;

	//���̈ʒu
	string sbksq, swksq;
	for (; index < 8; index++) { sbksq += itos_bool(psfen[index]); }
	for (; index <15 ; index++) { swksq += itos_bool(psfen[index]); }
	Square bksq =(Square)decimal(stoi(sbksq));
	Square wksq = (Square)decimal(stoi(swksq));

	ASSERT(is_ok(bksq));
	ASSERT(is_ok(wksq));
	pcboard[bksq] = B_KING;
	occupied[BLACK] |= SquareBB[bksq];
	occupiedPt[BLACK][KING] |= SquareBB[bksq];
	set_occ256(bksq);
	st->ksq_[BLACK] = bksq;

	pcboard[wksq] = W_KING;
	occupied[WHITE] |= SquareBB[wksq];
	occupiedPt[WHITE][KING] |= SquareBB[wksq];
	set_occ256(wksq);
	st->ksq_[WHITE] = wksq;

	int sq = 0;
	//index = 16;
	//�Տ�
	
		string spiece;
		Color c;
		bool promote;
		Piece pc;
		while (true) {
			if (sq >= SQ_NUM) { break; }
			if (sq == bksq ) { sq++; }//���̕��@�ł�BLACKK��WHITEK���A�����Ă������ɂ��������Ȃ��Ă��܂��I�I�I�I�I�I�I�i�C������.���Ȃ�̃��A�P�[�X��������..�j
			if (sq == wksq) { sq++; }
			if (sq >= SQ_NUM) { break; }//spiece�Ɉꕶ���c���Ă��܂��āAshand�����������Ȃ��Ă��܂����Ƃ��N������
			if (index > 256) {
				cout << *this << endl;
				ASSERT(0);
			}
			spiece += itos_bool(psfen[index++]);
			
			for (Piece  i = NO_PIECE; i < KING; i++) {

				if (spiece == board_unhaffman_str[(int)i]) {

					if (i == GOLD) {
						c = (Color)psfen[index++];

						pcboard[sq] = add_color(i, c);
						occupied[c] |= SquareBB[sq];
						occupiedPt[c][i] |= SquareBB[sq];
						set_occ256((Square)sq);

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
						Piece pc= promote ? promotepiece(add_color(i, c)) : pcboard[sq] = add_color(i, c);
						Piece pt = piece_type(pc);
						pcboard[sq] = pc;
						occupied[c] |= SquareBB[sq];
						occupiedPt[c][pt] |= SquareBB[sq];
						set_occ256((Square)sq);


						spiece.clear();
						sq++;
						break;
					}
				}
			}
		}
	
	//index��256�ł���Ύ�������݂��Ȃ�
	//if (index == 256) { goto FINISH; }
		//cout << *this << endl;
	//���
	string shand;
	//Color c;
	while (index < 256) {
		
		shand += itos(psfen[index++]);
		if (shand.size() > 6) {
			cout <<endl<< *this << endl;
			cout << occ_all() << endl;
			ASSERT(0);
		}
		for (Piece i = PAWN; i < KING; i++) {
			if (shand == hand_unhaffman_str[(int)i]) {
				c = (Color)psfen[index++];
				makehand(hands[c], i, num_pt(hand(c), i) + 1);
				shand.clear();
			}
		}
	}

	//cout << make_sfen() << endl;



FINISH:;

	init_existpawnBB();


	//��ԑ��ɉ��肪�������Ă��邩�ǂ�����������������΂����B�i���葤�ɉ��肪�������Ă�������Ŏ������I��邵����Ȃ�USI�œ����Ă������j
	if (is_effect_to(opposite(sidetomove_), ksq(sidetomove_))) {
		st->inCheck = true;
		st->checker = effect_toBB(opposite(sidetomove_), ksq(sidetomove_));

	}

	st->material = Eval::eval_material(*this);

	list.makebonaPlist(*this);
	Eval::eval(*this);
	init_hash();
	//cout << *this << endl;

	//list.print_bplist();
	//cout << occ256 << endl;
	ply_from_startpos = 1;

#ifdef CHECKPOS

	//check_eboard();
	//check_occbitboard();
#endif
	set_check_info(st);


	//return nsfen;
}


//Aperywcsc26�e�X�g�����邽�߂ɂ͂�˂��牤�̌`���ɍ��킹��K�v������....
//�Ȃ񂩃t���X�N���b�`�̊w�K���̃e�X�g�����邽�߂Ƀt���X�N���b�`����Ȃ��Ȃ�K�v������͉̂��Ƃ�������Ȃ�..............................
struct BitStream {

private:
	int bitcursor;//�����o��bit�ʒu
	uint8_t *data;//�������ރf�[�^�ւ̃|�C���^
public:
	void reset_cursor() { bitcursor = 0; }

	int ret_cursor()const { return bitcursor; }
	uint8_t* ret_data()const { return data; }

	void set_data(uint8_t* data_) { data = data_; reset_cursor(); }

	void write_one_bit(const int b) {
		if (b)
			data[bitcursor / 8] |= 1 << (bitcursor & 7);

		++bitcursor;
	}

	int read_one_bit()
	{
		int b = (data[bitcursor / 8] >> (bitcursor & 7)) & 1;
		++bitcursor;

		return b;
	}
	void write_n_bit(int d, int n)
	{
		for (int i = 0; i < n; ++i)
			write_one_bit(d & (1 << i));
	}

	int read_n_bit(int n)
	{
		int result = 0;
		for (int i = 0; i < n; ++i)
			result |= read_one_bit() ? (1 << i) : 0;

		return result;
	}
};

struct HuffmanedPiece
{
	int code; // �ǂ��R�[�h������邩
	int bits; // ��bit��L����̂�
};

HuffmanedPiece huffman_table[] =
{
	{ 0x00,1 }, // NO_PIECE
	{ 0x01,2 }, // PAWN
	{ 0x03,4 }, // LANCE
	{ 0x0b,4 }, // KNIGHT
	{ 0x07,4 }, // SILVER
	{ 0x1f,6 }, // BISHOP
	{ 0x3f,6 }, // ROOK
	{ 0x0f,5 }, // GOLD
};


struct SfenPacker {

	BitStream stream;

	Piece read_board_piece() {
		Piece p;

		int code = 0, bits = 0;

		//stream����1bit�ǂ݂�����bits�ɒǉ����Ă��ꂪhaffman code�ƈ�v���邩�m�F����
		while (true)
		{
			
			code |= stream.read_one_bit() << bits;
			++bits;

			ASSERT(bits <= 6);

			for (p = NO_PIECE; p < KING; ++p) {
				if (huffman_table[p].code == code&&huffman_table[p].bits == bits) {
					goto Found;
				}
			}
		}
	Found:;
		if (p == NO_PIECE) { return p; }
		bool ispromote = (p == GOLD) ? false : stream.read_one_bit();

		Color c = (Color)stream.read_one_bit();

		return add_color(p + (ispromote ? PROMOTE : NO_PIECE), c);
	}

	Piece read_hand_piece() {
		Piece p;

		int code = 0, bits = 0;

		//stream����1bit�ǂ݂�����bits�ɒǉ����Ă��ꂪhaffman code�ƈ�v���邩�m�F����
		while (true)
		{

			code |= stream.read_one_bit() << bits;
			++bits;

			ASSERT(bits <= 6);

			for (p = PAWN; p < KING; ++p) {
				if ((huffman_table[p].code>>1) == code&&(huffman_table[p].bits-1) == bits) {
					goto Found;
				}
			}
		}
	Found:;
		ASSERT(p != NO_PIECE);
		if (p != GOLD) { stream.read_one_bit(); }

		Color c = (Color)stream.read_one_bit();

		return add_color(p, c);
	}

	void write_board_piece(const Piece pc) {
		const Piece p = rowpiece(piece_type(pc));
		auto c = huffman_table[p];
		stream.write_n_bit(c.code, c.bits);
		if (pc == NO_PIECE) { return; }

		if (p != GOLD) {
			stream.write_one_bit((PROMOTE&pc) ? 1 : 0);
		}
		stream.write_one_bit(piece_color(pc));

	}
	void write_hand_piece(const Piece pc) {
		ASSERT(pc != NO_PIECE);
		const Piece p= rowpiece(piece_type(pc));
		auto c = huffman_table[p];
		stream.write_n_bit(c.code >> 1, c.bits - 1);
		// ���ȊO�͎��ł����Ă��s�����o�͂��āA�Տ�̋��bit��-1��ۂ�
		if (p != GOLD) {
			stream.write_one_bit(false);
		}
		// ���t���O
		stream.write_one_bit(piece_color(pc));
	}
};

void Position::unpack_haffman_sfen(const PackedSfen & sfen)
{
	SfenPacker packer;
	auto& stream = packer.stream;

	stream.set_data((uint8_t*)&sfen);
	clear();//�ǖʂ�����������

	//-------------------------------------------------------------------------���
	sidetomove_ = (Color)stream.read_one_bit();

	//--------------------------------------------------------------------------��
	const Square bksq = (Square)stream.read_n_bit(7);
	const Square wksq = (Square)stream.read_n_bit(7);
	ASSERT(is_ok(bksq) && is_ok(wksq));
	pcboard[bksq] = B_KING;
	occupied[BLACK] |= SquareBB[bksq];
	occupiedPt[BLACK][KING] |= SquareBB[bksq];
	set_occ256(bksq);
	st->ksq_[BLACK] = bksq;

	pcboard[wksq] = W_KING;
	occupied[WHITE] |= SquareBB[wksq];
	occupiedPt[WHITE][KING] |= SquareBB[wksq];
	set_occ256(wksq);
	st->ksq_[WHITE] = wksq;


	//-----------------------------------------------------------------------------�Տ�
	Piece pc;
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		if (pcboard[sq] == B_KING || pcboard[sq] == W_KING) {
			continue;
		}
		pc= packer.read_board_piece();
		if (pc == NO_PIECE) { continue; }

		const Piece pt = piece_type(pc);
		const Color c = piece_color(pc);
		pcboard[sq] = pc;
		occupied[c] |= SquareBB[sq];
		occupiedPt[c][pt] |= SquareBB[sq];
		set_occ256(sq);

		ASSERT(stream.ret_cursor() <= 256);
	}
		
	//-----------------------------------------------------------------------------���
	while (stream.ret_cursor() != 256) {

		pc = packer.read_hand_piece();
		const Piece pt = piece_type(pc);
		const Color c = piece_color(pc);
		makehand(hands[c], pt, num_pt(hand(c), pt) + 1);
	}


	//-----------------------------------------------------------------------------�������ɕt������ݒ�


	init_existpawnBB();

	//��ԑ��ɉ��肪�������Ă��邩�ǂ�����������������΂����B�i���葤�ɉ��肪�������Ă�������Ŏ������I��邵����Ȃ�USI�œ����Ă������j
	if (is_effect_to(opposite(sidetomove_), ksq(sidetomove_))) {
		st->inCheck = true;
		st->checker = effect_toBB(opposite(sidetomove_), ksq(sidetomove_));

	}

	st->material = Eval::eval_material(*this);

	list.makebonaPlist(*this);
	Eval::eval(*this);
	init_hash();
	//cout << *this << endl;

	//list.print_bplist();
	//cout << occ256 << endl;
	ply_from_startpos = 1;

#ifdef CHECKPOS

	//check_eboard();
	//check_occbitboard();
#endif
	set_check_info(st);



}


#endif


