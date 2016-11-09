#include "position.h"
#include "misc.h"
#include "Bitboard.h"
#include <sstream>

Sfen2Piece Sfen2Piece_;

//"sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
void Position::set(std::string sfen)
{
	//ここで初期化をしておく必要がある！

	clear();

	std::istringstream ss(sfen);

	string dummy;
	ss >> dummy;//sfenを読み飛ばす
	std::string s_sfen, s_teban, s_hands, token;
	ss >> s_sfen;
	ss >> s_teban;
	ss >> s_hands;
	/*if (token[0] != '-') {
		s_hands = token;
	}*/

	//最後の１は無視してもいい
	int index = 0;
	bool promote = false;
	char s;
	Piece pc;
	Rank r = RankA;
	File f = File9;
	while (r < Rank_Num&&index < s_sfen.size()) {

		s = s_sfen[index];

		//数字であればそのマスだけすすめる。
		if (isdigit(s)) {
			f = f - (s - '0');
		}
		// /であればrankを増やして　ファイルを戻す。
		else if (s == '/') {
			r++;
			f = File9;
		}
		//なりのフラグを立てる
		else if(s=='+'){
			promote = true;
		}
		else if (pc = Sfen2Piece_.sfen_to_piece(s)) {
			//ここで入って来るpcは成を含んでいないはずなので成であればその情報を付与する。
			if (promote) {
				pc = promotepiece(pc);
			}
			Piece pt = piece_type(pc);
			Color c = piece_color(pc);
			Square sq = make_square(r, f);

			if (pt == KING) {st->ksq_[c] = sq;}

			pcboard[sq] = pc;//OK
			occupied[c] |= SquareBB[sq];
			occupiedPt[c][pt] |= SquareBB[sq];
			put_rotate(sq);
			promote = false;
			f--;
		}
		index++;
	}//board

	 //手番
	if (s_teban.size()) {
		s_teban != "w" ? sidetomove_ = BLACK : sidetomove_ = WHITE;
	}
	//持ち駒
	index = 0;
	int num = 1;
	int num2;
	bool before_isdigit=false;
	while (index < s_hands.size()) {

		s = s_hands[index];
		if (s == '-') { break; }
		//handは２桁にも成りうる！！！
		if (isdigit(s)) {
			if (!before_isdigit) {
				num = (s - '0');
				//ASSERT(num != 1);
			}
			if (before_isdigit) {
				num2= (s - '0');
				num = num * 10 + num2;
			}
			before_isdigit = true;
		}
		else if (pc = Sfen2Piece_.sfen_to_piece(s)) {

			before_isdigit = false;
			Piece pt = piece_type(pc);
			Color c = piece_color(pc);

			makehand(hands[c], pt, num);

			num = 1;
		}
		index++;
	}
	init_eboard();
	
	if (return_effect(opposite(sidetomove_), ksq(sidetomove_))) {
		st->inCheck = true;
		make_checker_sq(sidetomove());
	}



#ifdef CHECKPOS
	cout << *this << endl;
	//check_eboard();
	//check_occbitboard();
#endif
}

void Position::clear()
{
	std::memset(this, 0, sizeof(Position));
	this->st = &initSt;
	std::memset(this->st, 0, sizeof(StateInfo));
}

//ここもバグが伝搬しやすいようにxorにした方がいいか？
void Position::remove_piece(const Color c, const Piece pt, const Square sq)
{
	/*occupiedPt[c][pt].andnot(SquareBB[sq]);
	occupied[c].andnot(SquareBB[sq]);
	occupied[2].andnot(SquareBB[sq]);*/
	occupiedPt[c][pt]^=(SquareBB[sq]);
	occupied[c]^=(SquareBB[sq]);
	//occupied[2]^=(SquareBB[sq]);
}

void Position::put_piece(const Color c, const Piece pt, const Square sq)
{
	/*occupiedPt[c][pt] |= SquareBB[sq];
	occupied[c] |= SquareBB[sq];
	occupied[2] |= SquareBB[sq];*/
	occupiedPt[c][pt] ^= (SquareBB[sq]);
	occupied[c] ^= (SquareBB[sq]);
	//occupied[2] ^= (SquareBB[sq]);
}

/*
効きの更新（あとで）
飛び機器があるため効きの更新はそんなに単純に計算はできない！！！！！！（どうすれば素早く機器の更新ができるのか......）


リストの差分計算（あとで）
駒得の差分計算（あとで）

*/

void Position::do_move(Move m, StateInfo * newst)
{
	//stateinfoの更新
	memcpy(newst, st, offsetof(StateInfo, lastmove));

	newst->previous = st;
	st = newst;

	//指し手の情報の用意
	Square to = move_to(m);//移動先
	Piece movedpiece = moved_piece(m);//移動した駒
	Piece capture;
	if (piece_type(pcboard[to]) == KING) {
		cout << *this << endl;
		ASSERT(0);
	}
	//undomoveの為の情報を用意
	st->DirtyPiece[0] = movedpiece;//dirtypieceに動いた駒を入れる
	st->lastmove = m;//今回指した指して

	ASSERT(is_ok(movedpiece));

	if (is_drop(m)) {

		//打つ駒の準備
		Piece pt = piece_type(movedpiece);
		Color c = piece_color(movedpiece);
		int num = num_pt(hands[sidetomove()], pt);
		ASSERT(num != 0);

		makehand(hands[sidetomove()], pt, num - 1);//持ち駒から駒を一枚減らす
		//boardの更新
		pcboard[to] = movedpiece;
		//bitboardの更新処理
		put_piece(c, pt, to);
		//rotatedにも駒を足す。
		put_rotate(to);
		//利きを追加する。
		//add_effect(c, pt, to);
	}
	else {
		/*
		コマの移動
		*/
		Square from = move_from(m);
		Color us = sidetomove_;
		Piece pt = piece_type(movedpiece);
		if (pt == KING&&st->Eboard[opposite(us)][to]) {
			cout << *this << endl;
			ASSERT(0);
		}
		ASSERT(movedpiece == pcboard[from]);
		ASSERT(piece_color(movedpiece) == us);

		//コマの移動
		//コマの移動
		pcboard[from] = NO_PIECE;
		capture = pcboard[to];


		//occの更新処理
		remove_piece(us, pt, from);
		remove_rotate(from);
		//fromの機器を取り除く
		sub_effect(us, pt, from);

		if (!is_promote(m)) {
			//成がない場合
			pcboard[to] = movedpiece;
			put_piece(us, pt, to);
			//toに効きを追加する
			//add_effect(us, pt, to);
		}
		else {
			//成がある場合
			Piece propt = promotepiece(pt);

			pcboard[to] = promotepiece(movedpiece);
			put_piece(us, propt, to);
			//toに効きを追加する
			//add_effect(us, propt, to);
		}

		//駒の捕獲
		if (capture != NO_PIECE) {
			st->DirtyPiece[1] = capture;

			Piece pt2 = rowpiece(piece_type(capture));//成り駒を取った場合はなってない駒に戻す
			Piece cappt = piece_type(capture);
			if (cappt == KING) {
				cout << *this << endl;
				ASSERT(0);
			}
			Color c = piece_color(capture);
			ASSERT(c != sidetomove());//自分の駒を取ってしまってないか
			remove_piece(c, cappt, to);
			//取られた駒の利きを取る。
		//	sub_effect(c, cappt, to);

			int num = num_pt(hands[sidetomove()], pt2);
			makehand(hands[sidetomove()], pt2, num + 1);
			//コマの捕獲の場合はrotetedは足さなくていい
		}
		else {
			st->DirtyPiece[1] = NO_PIECE;
			put_rotate(to);
		}
		//ksqの更新
		if (pt == KING) { st->ksq_[us] = to; }
	}

	st->ply_from_root++;
	nodes++;
	sidetomove_ = opposite(sidetomove_);//指す順番の入れ替え

}


void Position::undo_move()
{
	/*
	stをpreviousに戻せば利きは元に戻るのでここではsubeffectなどはしなくていい。
	この実装でほんとにいいのだろうか...メモリやアクセスの知識が必要だ
	*/

	Move LMove = st->lastmove;
	Square to = move_to(LMove);
	Piece movedpiece = st->DirtyPiece[0];
	Piece capture = st->DirtyPiece[1];

	//駒打ちの場合
	if (is_drop(LMove)) {
		/*
		移動先にはこまがなかったのでその駒を取り除く
		*/
		pcboard[to] = NO_PIECE;
		Piece pt = piece_type(movedpiece);
		Color c = piece_color(movedpiece);
		int num = num_pt(hands[c], pt);
		makehand(hands[c], pt, num + 1);

		remove_piece(c, pt, to);
		remove_rotate(to);
	}
	else {
		//コマの移動
		Square from = move_from(LMove);
		pcboard[from] = movedpiece;
		Piece afterpiece = pcboard[to];
		ASSERT(afterpiece != NO_PIECE);
		pcboard[to] = capture;

		//fromにoccupiedを追加
		Color from_c = piece_color(movedpiece);
		Piece from_pt = piece_type(movedpiece);
		put_piece(from_c, from_pt, from);
		put_rotate(from);
		//駒の捕獲がある場合
		if (capture != NO_PIECE) {
			Piece pt;

			pt = rowpiece(piece_type(capture));//captureから成と手番を除く 取られた駒なので成りは除かなければならない

			Color enemy = opposite(sidetomove());
			int num = num_pt(hands[enemy], pt);
			makehand(hands[enemy], pt, num - 1);//持ち駒の処理がおかしい。
			Piece cappt = piece_type(capture);
			Color c = piece_color(capture);
			//捕獲されていた駒
			put_piece(c, cappt, to);
			//もともといた駒の分
			remove_piece(from_c, piece_type(afterpiece), to);
			ASSERT(from_c != c);
			//コマの捕獲の場合はtoにいたrotetedを消す必要はない


		}
		else {
			//駒の捕獲のない場合
			Piece pt = piece_type(afterpiece);
			Color c = piece_color(afterpiece);
			//toにいた駒を消すだけでいい
			remove_piece(c, pt, to);
			remove_rotate(to);
		}
	}
	sidetomove_ = opposite(sidetomove_);

	st = st->previous;


}


void Position::check_effect() {


	/*
	初期局面で上手く言っているのでちゃんと実装できたと思いたい
	*/

	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {

	//	if (sq == SQ5I) {
	//		cout << "b[1]" << endl;
	//	}
	//	cout << "index_tate" << index_tate(sq) << " shifttable " << shift_tate(sq);
	//	int64_t obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_tate) << endl;
	//	cout << LongRookEffect_tate[sq][obstacle_tate] << endl;
	//}

	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout << "index_yoko" << index_yoko(sq) << " shifttable " << shift_yoko(sq);
		int64_t obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_yoko) << endl;
		cout << LongRookEffect_yoko[sq][obstacle_yoko] << endl;
	}*/

	/*for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
		cout <<"Square "<<int(sq)<<" "<<"index_plus45" << index_plus45(sq) << " shifttable " << shift_plus45(sq);
		int64_t obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_plus45) << endl;
		cout << LongBishopEffect_plus45[sq][obstacle_plus45] << endl;
	}*/

	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
	//	cout << "Square " << int(sq) << " " << "index_Minus45" << index_Minus45(sq) << " shifttable " << shift_Minus45(sq) << endl;;
	//	int64_t obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_Minus45) << endl;
	//	//cout << " change obstacle " << static_cast<std::bitset<7>>(change_indian(obstacle_Minus45)) << endl;
	//	cout << LongBishopEffect_minus45[sq][(obstacle_Minus45)] << endl;
	//}
}


void Position::check_occbitboard()const {

	cout << "occupied " << endl;
	cout << occ_all() << endl;
	cout << "occ 90" << endl;
	cout << occupied90 << endl;
	cout << "occupied_plus45" << endl;
	cout << occupied_plus45 << endl;
	cout << "occupied minus45" << endl;
	cout << occupied_minus45 << endl;

	return;
}

void Position::init_eboard()
{
	auto eboard = st->Eboard;
	Piece pc;
	std::memset(this->st->Eboard, 0, sizeof(this->st->Eboard[ColorALL][SQ_NUM]));//Eboradの初期化
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		if ((pc = pcboard[sq]) != NO_PIECE) {

			Color c = piece_color(pc);
			Piece pt = piece_type(pc);

			Bitboard effect;
			//ここの条件分岐もうちょっとなんとか成らないか？
			if (pt == LANCE) {
				int64_t obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
				effect = LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
				//cout << effect << endl;
			}
			else if (pt == ROOK) {
				int64_t obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
				int64_t obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
				effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
				//	cout << effect << endl;
			}
			else if (pt == DRAGON) {
				int64_t obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
				int64_t obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
				effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
				//	cout << effect << endl;
			}
			else if (pt == BISHOP) {
				int64_t obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
				int64_t obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
				effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
				//	cout << effect << endl;
			}
			else if (pt == UNICORN) {
				int64_t obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
				int64_t obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
				effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
				//	cout << effect << endl;
			}
			else {
				effect = StepEffect[c][pt][sq];
			}
			//cout << effect << endl;
			while (effect.isNot()) {
				Square esq = effect.pop();
				eboard[c][esq]++;
			}
		}
	}





}

void Position::add_effect(const Color c, const Piece pt, const Square sq)
{
	auto eboard = st->Eboard;

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;


	Bitboard effect;
	//ここの条件分岐もうちょっとなんとか成らないか？(switchぶんにする！！！！！！！！！)

	switch (pt)
	{
	case PAWN:
	case KNIGHT:
	case SILVER:
	case GOLD:
	case PRO_PAWN:
	case PRO_LANCE:
	case PRO_NIGHT:
	case PRO_SILVER:
	case KING:
		effect = StepEffect[c][pt][sq];
		break;
	case LANCE:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
		break;
	case BISHOP:
		 obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
		break;
	case ROOK:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case UNICORN:
		 obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
		break;
	case DRAGON:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
		break;
	
	default:
		ASSERT(0);
		break;
	}

	
	while (effect.isNot()) {

		Square esq = effect.pop();
		ASSERT(is_ok(esq));
		eboard[c][esq]++;
	}

}

void Position::sub_effect(const Color c, const Piece pt, const Square sq)
{
	auto eboard = st->Eboard;
	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;

	Bitboard effect;
	//ここの条件分岐もうちょっとなんとか成らないか？(switchぶんにする！！！！！！！！！)
	switch (pt)
	{
	case PAWN:
	case KNIGHT:
	case SILVER:
	case GOLD:
	case PRO_PAWN:
	case PRO_LANCE:
	case PRO_NIGHT:
	case PRO_SILVER:
	case KING:
		effect = StepEffect[c][pt][sq];
		break;
	case LANCE:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] & InFront_BB[c][sqtorank(sq)];
		break;
	case BISHOP:
		 obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];
		break;
	case ROOK:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];
		break;
	case UNICORN:
		 obstacle_plus45 = (occupied_plus45.b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		 obstacle_Minus45 = (occupied_minus45.b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)] | StepEffect[c][KING][sq];
		break;
	case DRAGON:
		 obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		 obstacle_yoko = (occupied90.b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko] | StepEffect[c][KING][sq];
		break;
	default:
		ASSERT(0);
		break;
	}
	//cout << effect << endl;
	while (effect.isNot()) {

		Square esq = effect.pop();
		ASSERT(is_ok(esq));
		ASSERT(eboard[c][esq] != 0);
		eboard[c][esq]--;
	}

}

void Position::check_eboard() const
{
	cout << "check eboard" << endl;
	for (Color c = BLACK; c < ColorALL; c++) {
		cout << "COLOR " << c << endl;
		for (Rank r = RankA; r < Rank_Num; r++) {
			for (File f = File9; f >= File1; f--) {
				Square sq = make_square(r, f);
				cout << int(st->Eboard[c][sq]);
			}
			cout << endl;
		}

		
		cout << endl;
	}

}

void Position::make_checker_sq(Color c) const
{
	//王のいる位置が１になっているbitboard
	Bitboard k = SquareBB[ksq(c)];

	Color enemy = opposite(c);
	//もっと効率的な方法はないか？？？
	/*
	近接駒と跳駒を分けて考えると
	近接ごまはfromにいてた駒の種類をtoに動かしたのでtoだけ見れば良くて、
	ksqとfromのdirection関係がわかり、fromにそのdirectionからのトビ機器が効いており、toがksqとdirectionのあいだでなかった場合はdirectionにその飛び利きの原因ごまを探して王手をしているかどうか再確認する
	
	*/
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {

		Piece pc = pcboard[sq];
		Piece pt = piece_type(pc);
		if (piece_color(pc) == enemy) {
			if ((k&effectBB(this,pt,c,sq)).isNot()) {
				ASSERT(is_ok(sq));
				st->checker |= SquareBB[sq];
			}
		}
	}
}


std::ostream & operator<<(std::ostream & os, const Position & pos)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {
			os << outputPiece[pos.piece_on(make_square(r, f))];
		}
		os << endl;
	}
	for (Color c = BLACK; c < ColorALL; c++) {
		(c == BLACK) ? os << "    先手 " << std::endl : os << "    後手 " << std::endl;
		os << pos.hand(c) << endl;
	}
	/*for (Color c = BLACK; c <= WHITE; c++) {
		os << " color " << c << endl << pos.occ(c) << endl;
	}
	os << "occ all" << endl << pos.occ_all() << endl;

	for (Color c = BLACK; c <= WHITE; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			os << " color " << c << " pt " << pt << endl << pos.occ_pt(c, pt) << endl;
		}
	}*/
	
	os << " 手番 " << pos.sidetomove() << endl;

	pos.check_eboard();
	

	return os;
}
