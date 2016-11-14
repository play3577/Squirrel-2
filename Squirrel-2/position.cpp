#include "position.h"
#include "misc.h"
#include "Bitboard.h"
#include "evaluate.h"
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

	init_existpawnBB();


	//手番側に王手がかかっているかどうかだけ初期化すればいい。（相手側に王手がかかっていたら一手で試合が終わるしそんなんUSIで入ってこやんやろ）
	if (is_effect_to(opposite(sidetomove_), ksq(sidetomove_))) {
		st->inCheck=true;
		st->checker = effect_toBB(opposite(sidetomove_), ksq(sidetomove_));

	}

	st->material = Eval::eval_material(*this);



	//cout << *this << endl;

#ifdef CHECKPOS
	
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
効きの更新（あとで）←難しかったので実装取りやめ


リストの差分計算（あとで）
駒得の差分計算（あとで）

*/
//pawnbbの処理OK
void Position::do_move(const Move m, StateInfo * newst)
{

	/*
	評価値の差分計算

	駒得
	相手が駒を失ってマイナス、自分は駒を得てプラスで評価値は自分の側にこの２つ分正の方向に動く。

	駒成り
	自分が駒が成った分評価値がプラスされる。

	*/

	//stateinfoの更新
	memcpy(newst, st, offsetof(StateInfo, lastmove));

	newst->previous = st;
	st = newst;

	//指し手の情報の用意
	Square to = move_to(m);//移動先
	Piece movedpiece = moved_piece(m);//移動させる駒
	Piece capture;
	//取ろうとしている駒が王であることはありえない
	if (piece_type(pcboard[to]) == KING) {
		cout << *this << endl;
		ASSERT(0);
	}
	//undomoveの為の情報を用意
	st->DirtyPiece[0] = movedpiece;//dirtypieceに動いた駒を入れる
	st->lastmove = m;//今回指した指して
	int16_t matterialdiff = 0;
	ASSERT(is_ok(movedpiece));

	if (is_drop(m)) {

		//駒打ちなので移動先には駒はいないはず（コレがundo_moveでのエラーの原因か!!!!こんちくしょう）
		//なんで駒打ちなのに移動先に駒がいる？（eversionで王手をかけている駒を取ろうとするときに駒を打って取ろうとしている！！！）
		if (pcboard[to] != NO_PIECE) {
			cout << *this << endl;
			check_move(m);
			ASSERT(0);
		}
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
		
		//歩を打てない場所Bitboardを更新
		if (pt == PAWN) {
			add_existpawnBB(c, to);
		}
	}
	else {
		/*
		コマの移動
		*/
		Square from = move_from(m);
		Color us = sidetomove_;
		Piece pt = piece_type(movedpiece);

		//動かす駒はfromにいる駒
		ASSERT(movedpiece == pcboard[from]);
		ASSERT(piece_color(movedpiece) == us);//動かす駒の色は手番と同じ

		//コマの移動
		//コマの捕獲
		pcboard[from] = NO_PIECE;
		capture = pcboard[to];


		//occの更新処理
		remove_piece(us, pt, from);
		remove_rotate(from);
		//fromの機器を取り除く
		

		if (!is_promote(m)) {
			//成がない場合
			pcboard[to] = movedpiece;
			put_piece(us, pt, to);
		}
		else {
			//成がある場合
			Piece propt = promotepiece(pt);
			ASSERT(propt != NO_PIECE);
			pcboard[to] = promotepiece(movedpiece);
			ASSERT(pcboard[to] != NO_PIECE);
			put_piece(us, propt, to);
			//歩が成った場合はその筋にはpawnを打てるように成る
			if (propt = PRO_PAWN) {
				remove_existpawnBB(us, to);
			}
			//コマ割の差分
			ASSERT(PAWN <= pt&&pt <= GOLD);
			matterialdiff += Eval::diff_promote[pt];
		}

		//駒の捕獲
		if (capture != NO_PIECE) {
			st->DirtyPiece[1] = capture;

			Piece pt2 = rowpiece(piece_type(capture));//成り駒を取った場合はなってない駒に戻す（手駒に追加するため）
			Piece cappt = piece_type(capture);
			//王を捕獲はできない
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

			if (cappt == PAWN) {
				remove_existpawnBB(c, to);
			}
			//コマ割の差分
			matterialdiff += Eval::capture_value[capture];
		}
		else {
			st->DirtyPiece[1] = NO_PIECE;
			put_rotate(to);
		}
		//ksqの更新
		if (pt == KING) { st->ksq_[us] = to; }
	}

	//先手の場合駒割は正の方向にdiffだけ動く
	if (sidetomove() == BLACK) {
		st->material = st->previous->material + Value(matterialdiff);
	}
	else {
		st->material = st->previous->material - Value(matterialdiff);
	}



	st->ply_from_root++;
	nodes++;
	sidetomove_ = opposite(sidetomove_);//指す順番の入れ替え

	//先程の指し手が相手側に王手をかけたか（王手をかけた場合はそのcheckerBBを更新する）
	if (is_effect_to(opposite(sidetomove_), ksq(sidetomove_))) {
		st->inCheck = true;
		st->checker = effect_toBB(opposite(sidetomove_), ksq(sidetomove_));
	}
	else {
		st->inCheck = false;
		st->checker = ZeroBB;
	}
	if (st->inCheck&&st->checker.isNot() == false) { ASSERT(0); }

	if (pcboard[to] == NO_PIECE) {
		cout << *this << endl;
		ASSERT(0);
	}
}

/*
なんでoccには駒はいるはずなのにptboardには駒はいないのか
*/
void Position::undo_move()
{
	
	
	Move LMove = st->lastmove;//前回の指し手
	Square to = move_to(LMove);//前回の移動先
	Piece movedpiece = st->DirtyPiece[0];//前回移動した駒
	Piece capture = st->DirtyPiece[1];//捕獲された駒
	if (pcboard[to] == NO_PIECE) {
		cout << *this << endl;
		ASSERT(0);
	}
	

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
		if (pt == PAWN) {
			remove_existpawnBB(c, to);
		}
	}
	else {
		//コマの移動
		Square from = move_from(LMove);
		Piece afterpiece = pcboard[to];//移動した駒はなっている場合があるためpcboard[to]を使う
		if (afterpiece == NO_PIECE) {
			cout << *this << endl;
			ASSERT(0);
		}
		pcboard[from] = movedpiece;
		
		pcboard[to] = capture;

		//fromにoccupiedを追加
		Color from_c = piece_color(movedpiece);
		Piece from_pt = piece_type(movedpiece);
		put_piece(from_c, from_pt, from);
		put_rotate(from);

		if (is_promote(LMove) && piece_type(afterpiece) == PRO_PAWN) {
			add_existpawnBB(piece_color(afterpiece), to);
		}

		//駒の捕獲がある場合
		if (capture != NO_PIECE) {
			Piece pt;

			pt = rowpiece(piece_type(capture));//captureから成と手番を除く 取られた駒を手駒から減算する操作なので成りは除かなければならない

			Color enemy = opposite(sidetomove());
			int num = num_pt(hands[enemy], pt);
			makehand(hands[enemy], pt, num - 1);
			Piece cappt = piece_type(capture);//盤面に戻す操作なので成も残す
			Color c = piece_color(capture);
			//捕獲されていた駒
			put_piece(c, cappt, to);
			//もともといた駒の分
			remove_piece(from_c, piece_type(afterpiece), to);
			ASSERT(from_c != c);
			//コマの捕獲の場合はtoにいたrotetedを消す必要はない
			
			if (cappt == PAWN) {
				add_existpawnBB(c, to);
			}

		}
		else {
			//駒の捕獲のない場合
			Piece pt = piece_type(afterpiece);
			Color c = piece_color(afterpiece);
			//toにいた駒を消すだけでいい
			remove_piece(c, pt, to);
			remove_rotate(to);
		}
		ASSERT(pcboard[from] != NO_PIECE);
	}//コマの移動
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

//この関数で打ち歩詰め、王の自殺手を省く。
/*
指し手が省かれる確率は非常に低いため
指し手のloopでこの関数を呼び出すのは後ろの方でいいかもしれない（Stockfish方式）
＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝＝PINされている駒を動かさないようにする処理もいる！！！！！！！！


この関数のpin駒の処理なかなか良くないので後で改良する

ちゃんと動くか確認する

２つの状況でだけ確認したがちゃんと動いてた
しかしすべての状況に対してちゃんと動くかどうかは分からないのでランダムプレイヤーで局面をすすめてテストする関数を用意する
*/

bool Position::is_legal(const Move m) const {

	Piece movedpiece = moved_piece(m);
	Square from = move_from(m);
	Square to = move_to(m);
	Square our_ksq = ksq(sidetomove());

	//王の自殺手
	//王が自殺しているかどうか調べるために使うoccupiedから王を場外しなければならない！！
	if (piece_type(movedpiece) == KING) {
		if (is_king_suiside(sidetomove_, to,from)) { return false; }
	}

	//打ち歩詰め
	bool  isDrop = is_drop(m);
	if (isDrop&& piece_type(movedpiece) == PAWN) {
		if (is_uchihu(sidetomove_, to)) { return false; }
	}

	/*
	pinされていた駒を動かしてしまっても相手のトビ効きが玉を貫通しないかどうか確認
	*/
	//Bitboard occ2 = occ_all()|SquareBB[move_to(m)]&SquareBB[move_from(m)];
	/*
	飛び効きが貫通しないかどうか確認するには
	駒と王が縦横斜めの関係に無いか調べる。
	縦横斜めのいずれかの方向にあればfromにその方向からの飛び効きが来ていないか調べる。
	来ていた場合はtoがその直線上の移動であるか調べる。
	その直線から外れる移動であればそれはpinされていた駒が動いたことに成る

	ーーーーーーーーーーーーーーーーfromとksqの間に他の駒があるかどうかの確認も必要！
	*/
	if (!isDrop) {
		Direction d = direct_table[from][our_ksq];
		//fromが縦横ナナメの関係にいる。
		if (d != Direction(0)) {

			/*fromとksqの間に他の駒があるかどうかの確認も必要！*///betweenBBの作成が必要
											//他の駒がある場合はそこで飛び効きが遮られるのでダイジョーブ
			if (!(BetweenBB[from][our_ksq] & occ_all()).isNot()) {
				Direction ksq2to = direct_table[to][our_ksq];
				//toが同じdirectionではない
				if (ksq2to != d) {
					//fromにd方向から飛び効きが効いていた場合は飛び効きを許してしまっている。
					if (is_longeffectdirection(sidetomove(), from, d)) { return false; }
				}
			}
		}
	}

	//動かす駒は自分の駒
	ASSERT(piece_color(movedpiece) == sidetomove_);
	//fromにいる駒と動かそうとしている駒は同じ
	if (!isDrop) { ASSERT(piece_on(from) == movedpiece); }
	//取ろうとしている駒は自分の駒ではない
	ASSERT(piece_on(to) == NO_PIECE || piece_color(piece_on(to)) != sidetomove_);
	//取ろうとしている駒は玉ではない
	if (piece_type(piece_on(to)) == KING) {

		cout << *this << endl;
		check_move(m);
		ASSERT(0);
	}
	return true;
}


std::ostream & operator<<(std::ostream & os, const Position & pos)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {
			os << outputPiece[pos.piece_on(make_square(r, f))];
		}
		os << endl;
	}
	/*for (Color c = BLACK; c < ColorALL; c++) {
		(c == BLACK) ? os << "    先手 " << std::endl : os << "    後手 " << std::endl;
		os << pos.hand(c) << endl;
	}
	for (Color c = BLACK; c <= WHITE; c++) {
		os << " color " << c << endl << pos.occ(c) << endl;
	}
	os << "occ all" << endl << pos.occ_all() << endl;

	os << "unicorn " << pos.occ_pt(WHITE, UNICORN) << endl;;
*/

	/*
	for (Color c = BLACK; c <= WHITE; c++) {
		for (Piece pt = PAWN; pt < PT_ALL; pt++) {
			os << " color " << c << " pt " << pt << endl << pos.occ_pt(c, pt) << endl;
		}
	}*/
	
	os << "ksq black " << pos.ksq(BLACK) << " white " << pos.ksq(WHITE) << endl;

	os << " 手番					" << pos.sidetomove() << endl;

	os << " 評価値　コマ割: " << pos.state()->material << endl;
	os << "lastmove"; check_move(pos.state()->lastmove);
	if (pos.state()->previous) {
		os << " lastlastmove "; check_move(pos.state()->previous->lastmove);
	}
	return os;
}
