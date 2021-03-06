#include "position.h"
#include "misc.h"
#include "Bitboard.h"
#include "evaluate.h"
#include <sstream>


#if defined(_MSC_VER)
#endif
#if defined(__GNUC__) 
#include <stddef.h>
#endif


Sfen2Piece Sfen2Piece_;

using namespace Eval;

//"sfen lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/LNSGKGSNL b - 1"
void Position::set(std::string sfen)
{
	//ここで初期化をしておく必要がある！

	clear();

	std::istringstream ss(sfen);

	string dummy;
	ss >> dummy;//sfenを読み飛ばす
	std::string s_sfen, s_teban, s_hands,s_ply, token;
	ss >> s_sfen;
	ss >> s_teban;
	ss >> s_hands;
	ss >> s_ply;
	/*if (token[0] != '-') {
		s_hands = token;
	}*/

	//最後の１は無視してもいい(あれは今何手目なのかを表しているので無視しちゃダメ！)
	size_t index = 0;
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
		else if ((pc = Sfen2Piece_.sfen_to_piece(s))!=NO_PIECE) {
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
			//put_rotate(sq);
			set_occ256(sq);

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
		else if ((pc = Sfen2Piece_.sfen_to_piece(s))!=NO_PIECE) {

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
	init_hash();
	st->material = Eval::eval_material(*this);

	list.makebonaPlist(*this);
	//list.print_bplist();
	Eval::eval(*this);
//	Progress::calc_prog(*this);
	
	//cout << *this << endl;
	
	//list.print_bplist();
	//cout << occ256 << endl;
	ply_from_startpos = stoi(s_ply);
	
#ifdef CHECKPOS
	
	//check_eboard();
	//check_occbitboard();
#endif
	set_check_info(st);
}



void Position::clear()
{
	std::memset(this, 0, sizeof(Position));
	this->st = &initSt;
	std::memset(this->st, 0, sizeof(StateInfo));
	this->st->clear_stPP();
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
//ニフを打った！！ちゃんとどこが悪いか確認する！！！！！！
/*
相手の駒を取ったのに自分のpawnbbが消えている

*/
//#define GIVESCHECK



void Position::do_move(const Move m, StateInfo * newst)
{
	//ここでst->PPをクリアーしておかないと差分計算上手く行かない
	//兄弟ノードを探索したときの値が入っているから
	newst->clear_stPP();
	/*
	評価値の差分計算

	駒得
	相手が駒を失ってマイナス、自分は駒を得てプラスで評価値は自分の側にこの２つ分正の方向に動く。

	駒成り
	自分が駒が成った分評価値がプラスされる。

	*/
#ifdef GIVESCHECK
	bool give_check = is_gives_check(m);
#endif 


	//stateinfoの更新
	memcpy(newst, st, offsetof(StateInfo, lastmove));

	newst->previous = st;
	st = newst;
	
	st->board_^=Zoblist::side;

	//指し手の情報の用意
	Square to = move_to(m);//移動先
	Piece movedpiece = moved_piece(m);//移動させる駒
	Piece capture;
	//取ろうとしている駒が王であることはありえない
	ASSERT(piece_type(pcboard[to]) != KING);
	/*if (piece_type(pcboard[to]) == KING) {
		cout << *this << endl;
		ASSERT(0);
	}*/
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
		ASSERT(add_color(pt, sidetomove()) == movedpiece);
		ASSERT(sidetomove() == c);
		ASSERT(num != 0);
		/*if (num == 0) {
			cout << *this << endl;
			check_move(m);
			ASSERT(0);
		}*/

		//dirtybonapの更新(一番枚数の大きい駒から打っていくことにする)
		st->dirtyuniform[0] = list.hand2Uniform[c][pt][num];
		ASSERT(st->dirtyuniform[0] != Eval::Num_Uniform);
		st->dirtybonap_fb[0] = list.bplist_fb[st->dirtyuniform[0]];
		st->dirtybonap_fw[0] = list.bplist_fw[st->dirtyuniform[0]];
		//listの更新
		list.hand2Uniform[c][pt][num] = Eval::Num_Uniform;
		list.sq2Uniform[to] = st->dirtyuniform[0];
		list.bplist_fb[st->dirtyuniform[0]] = Eval::bonapiece(to, movedpiece);
		list.bplist_fw[st->dirtyuniform[0]] = Eval::bonapiece(hihumin_eye(to), inverse(movedpiece));


		makehand(hands[sidetomove()], pt, num - 1);//持ち駒から駒を一枚減らす
		//boardの更新
		pcboard[to] = movedpiece;
		//bitboardの更新処理
		put_piece(c, pt, to);
		//rotatedにも駒を足す。
		//put_rotate(to);
		set_occ256(to);
		
		//歩を打てない場所Bitboardを更新
		if (pt == PAWN) {
			add_existpawnBB(c, to);
		}
		//hashの更新
		st->board_ += Zoblist::psq[movedpiece][to];
		st->hands_ -= Zoblist::hand[c][pt];
	}
	else {
		/*
		コマの移動
		*/
		Square from = move_from(m);
		Color us = sidetomove_;
		Piece moved_pt = piece_type(movedpiece);

		//動かす駒はfromにいる駒
		ASSERT(movedpiece == pcboard[from]);
		ASSERT(piece_color(movedpiece) == us);//動かす駒の色は手番と同じ

		//コマの移動
		//コマの捕獲
		pcboard[from] = NO_PIECE;
		capture = pcboard[to];

		//dirtybonapieceに格納
		st->dirtyuniform[0] = list.sq2Uniform[from];
		ASSERT(st->dirtyuniform[0] != Eval::Num_Uniform);
		st->dirtybonap_fb[0] = list.bplist_fb[st->dirtyuniform[0]];
		st->dirtybonap_fw[0] = list.bplist_fw[st->dirtyuniform[0]];

		//listの更新
		if (capture != NO_PIECE) {
			st->dirtyuniform[1] = list.sq2Uniform[to];
			ASSERT(st->dirtyuniform[1] != Eval::Num_Uniform);
			st->dirtybonap_fb[1] = list.bplist_fb[st->dirtyuniform[1]];
			st->dirtybonap_fw[1] = list.bplist_fw[st->dirtyuniform[1]];
		}
		list.sq2Uniform[from] = Eval::Num_Uniform;
		list.sq2Uniform[to] = st->dirtyuniform[0];




		//occの更新処理
		remove_piece(us, moved_pt, from);
		//remove_rotate(from);
		//fromの機器を取り除く
		remove_occ256(from);

		if (!is_promote(m)) {
			//成がない場合
			pcboard[to] = movedpiece;
			put_piece(us, moved_pt, to);

			//listの更新
			list.bplist_fb[list.sq2Uniform[to]] = bonapiece(to, movedpiece);
			list.bplist_fw[list.sq2Uniform[to]] = bonapiece(hihumin_eye(to), inverse(movedpiece));

			//zoblistの更新
			st->board_ -= Zoblist::psq[movedpiece][from];
			st->board_ += Zoblist::psq[movedpiece][to];
		}
		else {
			//成がある場合
			Piece propt = promotepiece(moved_pt);
			Piece propc= promotepiece(movedpiece);
			/*if (propt == PRO_PAWN&&moved_pt != PAWN) {
				ASSERT(0);
			}*/
			ASSERT(propt != NO_PIECE);
			pcboard[to] = propc;
			ASSERT(pcboard[to] != NO_PIECE);
			put_piece(us, propt, to);
			//歩が成った場合はその筋にはpawnを打てるように成る
			if (propt == PRO_PAWN) {
				remove_existpawnBB(us, to);
			}
			//コマ割の差分
			ASSERT(PAWN <= moved_pt&&moved_pt <= GOLD);
			matterialdiff += Eval::diff_promote[moved_pt];

			//listの更新
			list.bplist_fb[list.sq2Uniform[to]] = bonapiece(to, propc);
			list.bplist_fw[list.sq2Uniform[to]] = bonapiece(hihumin_eye(to), inverse(propc));

			//成を含めたzoblistの更新
			st->board_ -= Zoblist::psq[movedpiece][from];
			st->board_ += Zoblist::psq[propc][to];
		}

		//駒の捕獲
		if (capture != NO_PIECE) {
			st->DirtyPiece[1] = capture;

			Piece pt2 = rowpiece(piece_type(capture));//成り駒を取った場合はなってない駒に戻す（手駒に追加するため）
			Piece cappt = piece_type(capture);
			//王を捕獲はできない
			ASSERT(cappt != KING);
			/*if (cappt == KING) {
				cout << *this << endl;
				ASSERT(0);
			}*/
			//cは取られたコマの色
			Color c_cap = piece_color(capture);
			ASSERT(c_cap != sidetomove());//自分の駒を取ってしまってないか
			remove_piece(c_cap, cappt, to);
			//取られた駒の利きを取る。
		//	sub_effect(c, cappt, to);

			int num = num_pt(hands[sidetomove()], pt2);
			makehand(hands[sidetomove()], pt2, num + 1);
			//コマの捕獲の場合はrotetedは足さなくていい

			if (cappt == PAWN) {
				remove_existpawnBB(c_cap, to);
			}
			//コマ割の差分
			matterialdiff += Eval::capture_value[capture];

			//listの更新(ここコレでいいか怪しい。)
			list.bplist_fb[st->dirtyuniform[1]] = bonapiece(us, pt2, num + 1);
			list.bplist_fw[st->dirtyuniform[1]] = bonapiece(opposite(us), pt2, num + 1);
			list.hand2Uniform[us][pt2][num + 1] = st->dirtyuniform[1];


			//捕獲された駒のzoblistの更新
			st->board_ -= Zoblist::psq[capture][to];
			st->hands_ += Zoblist::hand[sidetomove()][pt2];
		}
		else {
			st->DirtyPiece[1] = NO_PIECE;
			//put_rotate(to);
			set_occ256(to);
		}
		//ksqの更新
		if (moved_pt == KING) { st->ksq_[us] = to; }
	}

	//先手の場合駒割は正の方向にdiffだけ動く
	if (sidetomove() == BLACK) {
		st->material = st->previous->material + Value(matterialdiff);
	}
	else {
		st->material = st->previous->material - Value(matterialdiff);
	}



	st->ply_from_startpos++;
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
	//if (st->inCheck&&st->checker.isNot() == false) { ASSERT(0); }
	ASSERT(!st->inCheck || st->checker.isNot());
#ifdef GIVESCHECK
	if (give_check != st->inCheck) {
		cout << *this << endl;
		check_move(m);
		ASSERT(0);
	}
#endif
	/*if (pcboard[to] == NO_PIECE) {
		cout << *this << endl;
		ASSERT(0);
	}*/
	ASSERT(pcboard[to] != NO_PIECE);

	ASSERT((sidetomove() == BLACK && (st->board_&Zoblist::side) == 0) || (sidetomove() == WHITE && (st->board_&Zoblist::side) == 1));
	set_check_info(st);
}

void Position::do_move(const Move m, StateInfo * newst, const bool givescheck)
{

	//ここでst->PPをクリアーしておかないと差分計算上手く行かない
	//兄弟ノードを探索したときの値が入っているから
	newst->clear_stPP();
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

	st->board_ ^= Zoblist::side;

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
		ASSERT(pcboard[to] == NO_PIECE);
		/*if (pcboard[to] != NO_PIECE) {
			cout << *this << endl;
			check_move(m);
			ASSERT(0);
		}*/


		//打つ駒の準備
		Piece pt = piece_type(movedpiece);
		Color c = piece_color(movedpiece);
		int num = num_pt(hands[sidetomove()], pt);
		ASSERT(add_color(pt, sidetomove()) == movedpiece);
		ASSERT(sidetomove() == c);
		ASSERT(num != 0);

		//dirtybonapの更新(一番枚数の大きい駒から打っていくことにする)
		st->dirtyuniform[0] = list.hand2Uniform[c][pt][num];
		ASSERT(st->dirtyuniform[0] != Eval::Num_Uniform);
		st->dirtybonap_fb[0] = list.bplist_fb[st->dirtyuniform[0]];
		st->dirtybonap_fw[0] = list.bplist_fw[st->dirtyuniform[0]];
		//listの更新
		list.hand2Uniform[c][pt][num] = Eval::Num_Uniform;
		list.sq2Uniform[to] = st->dirtyuniform[0];
		list.bplist_fb[st->dirtyuniform[0]] = Eval::bonapiece(to, movedpiece);
		list.bplist_fw[st->dirtyuniform[0]] = Eval::bonapiece(hihumin_eye(to), inverse(movedpiece));


		makehand(hands[sidetomove()], pt, num - 1);//持ち駒から駒を一枚減らす
												   //boardの更新
		pcboard[to] = movedpiece;
		//bitboardの更新処理
		put_piece(c, pt, to);
		//rotatedにも駒を足す。
		//put_rotate(to);
		set_occ256(to);

		//歩を打てない場所Bitboardを更新
		if (pt == PAWN) {
			add_existpawnBB(c, to);
		}
		//hashの更新
		st->board_ += Zoblist::psq[movedpiece][to];
		st->hands_ -= Zoblist::hand[c][pt];
	}
	else {
		/*
		コマの移動
		*/
		Square from = move_from(m);
		Color us = sidetomove_;
		Piece moved_pt = piece_type(movedpiece);

		//動かす駒はfromにいる駒
		ASSERT(movedpiece == pcboard[from]);
		ASSERT(piece_color(movedpiece) == us);//動かす駒の色は手番と同じ

											  //コマの移動
											  //コマの捕獲
		pcboard[from] = NO_PIECE;
		capture = pcboard[to];

		//dirtybonapieceに格納
		st->dirtyuniform[0] = list.sq2Uniform[from];
		ASSERT(st->dirtyuniform[0] != Eval::Num_Uniform);
		st->dirtybonap_fb[0] = list.bplist_fb[st->dirtyuniform[0]];
		st->dirtybonap_fw[0] = list.bplist_fw[st->dirtyuniform[0]];

		//listの更新
		if (capture != NO_PIECE) {
			st->dirtyuniform[1] = list.sq2Uniform[to];
			ASSERT(st->dirtyuniform[1] != Eval::Num_Uniform);
			st->dirtybonap_fb[1] = list.bplist_fb[st->dirtyuniform[1]];
			st->dirtybonap_fw[1] = list.bplist_fw[st->dirtyuniform[1]];
		}
		list.sq2Uniform[from] = Eval::Num_Uniform;
		list.sq2Uniform[to] = st->dirtyuniform[0];




		//occの更新処理
		remove_piece(us, moved_pt, from);
		//remove_rotate(from);
		//fromの機器を取り除く
		remove_occ256(from);

		if (!is_promote(m)) {
			//成がない場合
			pcboard[to] = movedpiece;
			put_piece(us, moved_pt, to);

			//listの更新
			list.bplist_fb[list.sq2Uniform[to]] = bonapiece(to, movedpiece);
			list.bplist_fw[list.sq2Uniform[to]] = bonapiece(hihumin_eye(to), inverse(movedpiece));

			//zoblistの更新
			st->board_ -= Zoblist::psq[movedpiece][from];
			st->board_ += Zoblist::psq[movedpiece][to];
		}
		else {
			//成がある場合
			Piece propt = promotepiece(moved_pt);
			Piece propc = promotepiece(movedpiece);
			/*if (propt == PRO_PAWN&&moved_pt != PAWN) {
			ASSERT(0);
			}*/
			ASSERT(propt != NO_PIECE);
			pcboard[to] = propc;
			ASSERT(pcboard[to] != NO_PIECE);
			put_piece(us, propt, to);
			//歩が成った場合はその筋にはpawnを打てるように成る
			if (propt == PRO_PAWN) {
				remove_existpawnBB(us, to);
			}
			//コマ割の差分
			ASSERT(PAWN <= moved_pt&&moved_pt <= GOLD);
			matterialdiff += Eval::diff_promote[moved_pt];

			//listの更新
			list.bplist_fb[list.sq2Uniform[to]] = bonapiece(to, propc);
			list.bplist_fw[list.sq2Uniform[to]] = bonapiece(hihumin_eye(to), inverse(propc));

			//成を含めたzoblistの更新
			st->board_ -= Zoblist::psq[movedpiece][from];
			st->board_ += Zoblist::psq[propc][to];
		}

		//駒の捕獲
		if (capture != NO_PIECE) {
			st->DirtyPiece[1] = capture;

			Piece pt2 = rowpiece(piece_type(capture));//成り駒を取った場合はなってない駒に戻す（手駒に追加するため）
			Piece cappt = piece_type(capture);
			//王を捕獲はできない

			ASSERT(cappt != KING);
			/*if (cappt == KING) {
				cout << *this << endl;
				ASSERT(0);
			}*/


			//cは取られたコマの色
			Color c_cap = piece_color(capture);
			ASSERT(c_cap != sidetomove());//自分の駒を取ってしまってないか
			remove_piece(c_cap, cappt, to);
			//取られた駒の利きを取る。
			//	sub_effect(c, cappt, to);

			int num = num_pt(hands[sidetomove()], pt2);
			makehand(hands[sidetomove()], pt2, num + 1);
			//コマの捕獲の場合はrotetedは足さなくていい

			if (cappt == PAWN) {
				remove_existpawnBB(c_cap, to);
			}
			//コマ割の差分
			matterialdiff += Eval::capture_value[capture];

			//listの更新(ここコレでいいか怪しい。)
			list.bplist_fb[st->dirtyuniform[1]] = bonapiece(us, pt2, num + 1);
			list.bplist_fw[st->dirtyuniform[1]] = bonapiece(opposite(us), pt2, num + 1);
			list.hand2Uniform[us][pt2][num + 1] = st->dirtyuniform[1];


			//捕獲された駒のzoblistの更新
			st->board_ -= Zoblist::psq[capture][to];
			st->hands_ += Zoblist::hand[sidetomove()][pt2];
		}
		else {
			st->DirtyPiece[1] = NO_PIECE;
			//put_rotate(to);
			set_occ256(to);
		}
		//ksqの更新
		if (moved_pt == KING) { st->ksq_[us] = to; }
	}

	//先手の場合駒割は正の方向にdiffだけ動く
	if (sidetomove() == BLACK) {
		st->material = st->previous->material + Value(matterialdiff);
	}
	else {
		st->material = st->previous->material - Value(matterialdiff);
	}



	st->ply_from_startpos++;
	nodes++;
	sidetomove_ = opposite(sidetomove_);//指す順番の入れ替え

										//先程の指し手が相手側に王手をかけたか（王手をかけた場合はそのcheckerBBを更新する）
	if (givescheck) {
		st->inCheck = true;
		st->checker = effect_toBB(opposite(sidetomove_), ksq(sidetomove_));
	}
	else {
		st->inCheck = false;
		st->checker = ZeroBB;
	}


	//if (st->inCheck&&st->checker.isNot() == false) { ASSERT(0); }
	ASSERT(!st->inCheck||st->checker.isNot());


#ifdef GIVESCHECK
	if (givescheck != is_effect_to(opposite(sidetomove_), ksq(sidetomove_))) {
		cout << *this << endl;
		check_move(m);
		ASSERT(0);
	}
#endif
	/*if (pcboard[to] == NO_PIECE) {
		cout << *this << endl;
		ASSERT(0);
	}*/
	ASSERT(pcboard[to] != NO_PIECE);


	ASSERT((sidetomove() == BLACK && (st->board_&Zoblist::side) == 0) || (sidetomove() == WHITE && (st->board_&Zoblist::side) == 1));
	set_check_info(st);
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

	ASSERT(pcboard[to] != NO_PIECE);
	/*if (pcboard[to] == NO_PIECE) {
		cout << *this << endl;
		ASSERT(0);
	}*/


	//evallistの差分用
	const Eval::BonaPiece movedbonap_fb = st->dirtybonap_fb[0];
	const Eval::BonaPiece capturedbonap_fb = st->dirtybonap_fb[1];
	const Eval::BonaPiece movedbonap_fw = st->dirtybonap_fw[0];
	const Eval::BonaPiece capturedbonap_fw = st->dirtybonap_fw[1];
	const Eval::UniformNumber moveduniform = st->dirtyuniform[0];
	const Eval::UniformNumber captureduniform = st->dirtyuniform[1];



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
		//remove_rotate(to);
		remove_occ256(to);
		//cは動かしたコマの色 (打った駒が元に戻るのでここは駒を打てるように成る)
		if (pt == PAWN) {
			remove_existpawnBB(c, to);
		}
		list.bplist_fb[moveduniform] = movedbonap_fb;//元の位置に戻す。
		list.bplist_fw[moveduniform] = movedbonap_fw;
		list.sq2Uniform[to] = Eval::Num_Uniform;//toには何もなかった
		list.hand2Uniform[c][pt][num + 1] = moveduniform;//持ち駒に戻る。

	}
	else {
		//コマの移動
		Square from = move_from(LMove);
		Piece afterpiece = pcboard[to];//移動した駒はなっている場合があるためpcboard[to]を使う

		//これすでに上でチェックしている
		/*if (afterpiece == NO_PIECE) {
			cout << *this << endl;
			ASSERT(0);
		}*/
		pcboard[from] = movedpiece;
		
		pcboard[to] = capture;

		//fromにoccupiedを追加
		Color from_c = piece_color(movedpiece);
		Piece from_pt = piece_type(movedpiece);
		put_piece(from_c, from_pt, from);
		//put_rotate(from);
		set_occ256(from);
		//直前の指し手が成で、動いたあとの駒がPRO_PAWNであれば
		if (is_promote(LMove) && piece_type(afterpiece) == PRO_PAWN) {
			add_existpawnBB(piece_color(afterpiece),from);
		}

		//fromのいちに戻ってくるのは捕獲であろうと、成であろうと共通
		list.bplist_fb[moveduniform] = movedbonap_fb;//元の位置に戻す。(成りの場合でもコレでいいと考えられる。)
		list.bplist_fw[moveduniform] = movedbonap_fw;
		if (capture == NO_PIECE) {
			list.sq2Uniform[to] = Eval::Num_Uniform;
		}
		else {
			list.sq2Uniform[to] = captureduniform;
			list.bplist_fb[captureduniform] = capturedbonap_fb;
			list.bplist_fw[captureduniform] = capturedbonap_fw;
		}
		list.sq2Uniform[from] = moveduniform;




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


			//
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
			//remove_rotate(to);
			remove_occ256(to);
		}
		ASSERT(pcboard[from] != NO_PIECE);
	}//コマの移動
	sidetomove_ = opposite(sidetomove_);

	st = st->previous;

	

}

void Position::do_nullmove(StateInfo * newst) {
	//王手かけられていると王を取られてしまう
	ASSERT(state()->checker.isNot() == false);
	ASSERT(newst != st);

	//パスなので全部コピーしてOK 
	std::memcpy(newst, st, sizeof(StateInfo));
	newst->previous = st;
	st = newst;
	//手番を反転
	st->board_ ^= Zoblist::side;
	sidetomove_ = opposite(sidetomove_);
	st->lastmove = MOVE_NULL;

	//ここでvalue_errorにしてはダメ。
	/*st->bpp = Value_error;
	st->wpp = Value_error;*/

	//この局面ではeval()されてから何も動いていないのでdirtyはゼロでいいはず
	state()->dirtybonap_fb[0]= BONA_PIECE_ZERO;
	state()->dirtybonap_fb[1] = BONA_PIECE_ZERO;
	state()->dirtybonap_fw[0] = BONA_PIECE_ZERO;
	state()->dirtybonap_fw[1] = BONA_PIECE_ZERO;
	state()->dirtyuniform[0] =Eval::Num_Uniform;
	state()->dirtyuniform[1] = Eval::Num_Uniform;

	ASSERT((sidetomove() == BLACK && (st->board_&Zoblist::side) == 0) || (sidetomove() == WHITE && (st->board_&Zoblist::side) == 1));
}

void Position::undo_nullmove()
{
	st = st->previous;
	sidetomove_ = opposite(sidetomove_);

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

void Position::check_effectocc256()
{



	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
	//	//初期局面OK
	//	int8_t obstacle_tate = (occ256.b256.m256i_u64[0] >> occ256_shift_table_tate[sq])&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_tate) << endl;
	//	cout << LongRookEffect_tate[sq][obstacle_tate] << endl;
	//}
	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
	//	//初期局面OK
	//	int8_t obstacle_tate = (occ256.b256.m256i_u64[1] >> occ256_shift_table_yoko[sq])&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_tate) << endl;
	//	cout << LongRookEffect_yoko[sq][obstacle_tate] << endl;
	//}
	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
	//	//初期局面OK
	//	int8_t obstacle_tate = (occ256.b256.m256i_u64[2] >> occ256_shift_table_p45[sq])&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_tate) << endl;
	//	cout << LongBishopEffect_plus45[sq][obstacle_tate] << endl;
	//}
	//for (Square sq = SQ1A; sq < SQ_NUM; sq++) {
	//	//初期局面OK
	//	int8_t obstacle_tate = (occ256.b256.m256i_u64[3] >> occ256_shift_table_m45[sq])&effectmask;
	//	cout << "obstacle" << static_cast<std::bitset<7>>(obstacle_tate) << endl;
	//	cout << LongBishopEffect_minus45[sq][obstacle_tate] << endl;
	//}
}


void Position::check_occbitboard()const {

	cout << "occupied " << endl;
	cout << occ_all() << endl;
	/*cout << "occ 90" << endl;
	cout << occupied90 << endl;
	cout << "occupied_plus45" << endl;
	cout << occupied_plus45 << endl;
	cout << "occupied minus45" << endl;
	cout << occupied_minus45 << endl;*/
	cout << occ256 << endl;
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

bool Position::capture_or_propawn(const Move m) const {

	if (pcboard[move_to(m)] != NO_PIECE) {
		/*//手番の駒を取ろうとしていないかのチェックはしておく
		if (piece_color(pcboard[move_to(m)]) == sidetomove()) {
			cout << *this << endl;
			cout << m << endl;
			ASSERT(0);
		}*/
		return true;
	}
	else if (piece_type(moved_piece(m)) == PAWN&&is_promote(m)) {
		return true;
	}

	return false;
}

bool Position::capture(const Move m) const
{
	if (pcboard[move_to(m)] != NO_PIECE) {
		ASSERT(piece_color(pcboard[move_to(m)]) != sidetomove());
		return true;
	}
	return false;
}

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
	//if (piece_color(movedpiece) != sidetomove_) { goto Error; }
	////fromにいる駒と動かそうとしている駒は同じ
	//if (!isDrop) { ASSERT(piece_on(from) == movedpiece); }

	ASSERT(piece_color(movedpiece) == sidetomove_);
	ASSERT(isDrop || piece_on(from) == movedpiece);

	//取ろうとしている駒は自分の駒ではない
#ifdef LEARN
	if (piece_on(to) != NO_PIECE && piece_color(piece_on(to)) == sidetomove_) { return false; }
#else
	ASSERT(piece_on(to) == NO_PIECE || piece_color(piece_on(to)) != sidetomove_);
#endif
	
	

	//ASSERT(piece_type(piece_on(to)) != KING);
	//取ろうとしている駒は玉ではない
	if (piece_type(piece_on(to)) == KING) {
//#ifndef LEARN
		cout <<endl<< *this << endl;
		cout << ret_occ_256() << endl;
		cout << occ_all() << endl;
		check_move(m);
		ASSERT(0);
//#else 
		//学習中は人間の棋譜で相手の効きのある場所に王を動かしてしまうことが起こってしまうこともあるみたい
		//return false;
//#endif
	}



#ifdef LEARN
	if (is_drop(m) && (num_pt(hand(sidetomove()), piece_type(movedpiece)) == 0)) {
		return false;
	}
#endif


	return true;


Error:;

	cout << *this << endl;
	cout << "----error move" << m << endl;
	ASSERT(0);
	return false;

}

bool Position::pseudo_legal(const Move m) const
{
	const Color us = sidetomove();
	const Square to = move_to(m);
	const Piece movedpiece = moved_piece(m);

	//blackが0であるのでNO_PIECEも考えないといけない
	if (movedpiece==NO_PIECE||piece_color(movedpiece) != us) { return false; }

	if (is_drop(m)) {
		//移動先に他の駒がある,コマを持っていない　二歩（打ち歩詰めはlegalでチェック）
		if (piece_on(to) != NO_PIECE || num_pt(hand(us), piece_type(movedpiece)) == 0||check_nihu(m)) {
			return false;
		}
		//王手時
		if (is_incheck()) {

			Bitboard target = state()->checker;
			Square esq = target.pop();
			//二重王手
			if (target.isNot()) {
				return false;
			}
			//(ksq,esq)の範囲にtoが無いと王手を防げていない
			if ((BetweenBB[ksq(us)][esq] & SquareBB[to]).isNot()==false) {
				return false;
			}

		}

	}
	else {
		//コマの移動

		const Square from = move_from(m);
		const Piece pc_from = piece_on(from);
		if (pc_from != movedpiece) {
			return false;
		}
		if ((occ(us)&SquareBB[to]).isNot()) { return false; }
		//コマを飛び越えて進むことは出来ない
		if ((BetweenBB[from][to] & occ_all()).isNot()) { return false; }


		//王手時(KING以外の指しての場合だけ考える)
		if (is_incheck()&&piece_type(movedpiece)!=KING) {

			Bitboard target = state()->checker;
			Square esq = target.pop();
			//二重王手
			if (target.isNot()) {
				return false;
			}
			//(ksq,esq]の範囲にtoが無いと王手を防げていない
			if (((BetweenBB[ksq(us)][esq]|SquareBB[esq]) & SquareBB[to]).isNot()==false) {
				return false;
			}

		}

	}

	return true;
}


/*
現在局面に対応するsfen文字列を生成するための関数（デバッグ用であり、定跡を管理するためにも用いる）

持ち駒は一意には定まらないが
先手の歩から駒の価値毎に並べ次に後手の歩から駒の価値毎に並べていくスタイルでSquirrelでは一意に定める。

*/
string Position::make_sfen()const 
{
	string sfen;

	int space = 0;

	//先頭にはsfenをつける
	sfen += "sfen ";

	//----------------------------------------盤上
	for (Rank r = RankA; r <= RankI; r++) {
		for (File f = File9; f >= File1; f--) {

			const Square sq = make_square(r, f);
			const Piece pc = piece_on(sq);

			if (pc != NO_PIECE) {

				if (space != 0) {
					sfen += itos(space);
					space = 0;
				}

				//駒があればその駒のUSIPieceをsfenに足す。
				sfen += USIPiece[pc];
			}
			else {
				//駒がない場所であれば駒のない場所の数をインクリメント
				space++;
			}

		}
		if (space != 0) {
			sfen += itos(space);
			space = 0;
		}
		if (r != RankI) {
			sfen += "/";
		}
	}

	//--------------------------------------------手番
	sidetomove() == BLACK ? sfen += " b" : sfen += " w";

	//--------------------------------------------手駒
	int num;

	if (have_hand(hand(BLACK)) == false && have_hand(hand(WHITE)) == false) {
		sfen += " -";
	}
	else {
		sfen += " ";
		for (Color c = BLACK; c <= WHITE; c++) {

			const Hand h = hand(c);
			for (Piece pt = PAWN; pt < KING; pt++) {

				if ((num = num_pt(h, pt)) != 0) {

					if (num != 1) {
						sfen += itos(num);
					}
					sfen += USIPiece[add_color(pt, c)];
				}
			}
		}
	}
	
	sfen += " ";
	sfen+=itos(ply_from_startpos);

	return sfen;
}



void Position::init_hash()
{
	//盤上
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		const Piece pc = piece_on(sq);
		st->board_ += Zoblist::psq[pc][sq];
	}
	//手駒
	for (Color c = BLACK; c < ColorALL; c++) {
		const Hand h = hand(c);
		for (Piece pt = PAWN; pt <= GOLD; pt++) {

			int num = num_pt(h, pt);
			if (num != 0) {
				for (int i = 1; i <= num; i++) {
					st->hands_ += Zoblist::hand[c][pt];
				}
			}
		}
	}

	//手番　白番ならば1を足す
	if (sidetomove() == WHITE) {
		st->board_ += Zoblist::side;
	}


}

/*
与えられた差し手で局面を動かした後のhashkeyを計算する、
これはprefetchに必要になる

prefetchはできるだけ早くしておく必要があるのか,,,先に局面を動かしたhashkeyを用意してまで!!!

ここまでして早くなるとは思えないんだがどうなのだ???

*/
Key Position::key_after_move(const Move m) {

	const Square from = move_from(m);
	const Square to = move_to(m);
	const Piece pc = moved_piece(m);
	const bool promote = is_promote(m);
	const bool drop = is_drop(m);
	const Piece captured = piece_on(to);
	Key k = key();

	//駒うち
	if (drop) {
		k += Zoblist::psq[pc][to];
		k -= Zoblist::hand[sidetomove()][piece_type(pc)];

	}
	else {
		//駒の移動

		//fromにいた駒の分を消す
		k -= Zoblist::psq[pc][from];

		//toにいる駒を足す
		if (promote) {
			k += Zoblist::psq[promotepiece(pc)][to];
		}
		else {
			k += Zoblist::psq[pc][to];
		}

		//駒をとった。
		if (captured != NO_PIECE) {
			k -= Zoblist::psq[captured][to];
			k += Zoblist::hand[sidetomove()][rowpiece(piece_type(captured))];
		}
	}

	k ^= Zoblist::side;

	return k;
}




std::ostream & operator<<(std::ostream & os, const Position & pos)
{
	for (Rank r = RankA; r < Rank_Num; r++) {
		for (File f = File9; f >= File1; f--) {
			os << outputPiece[pos.piece_on(make_square(r, f))];
		}
		os << endl;
	}
	
	//pos.evallist().print_bplist();
	/*for (Color c = BLACK; c <= WHITE; c++) {
		os << " color " << c << endl << pos.occ(c) << endl;
	}*/
	/*os << "occ all" << endl << pos.occ_all() << endl;
	os << " occ 90" << endl << pos.occ_90() << endl;
	os << "occ +45 " << endl << pos.occ_plus45() << endl;
	os << " occ -45 " << endl << pos.occ_minus45() << endl;*/
	//os << "unicorn " << pos.occ_pt(WHITE, UNICORN) << endl;;

	for (Color c = BLACK; c < ColorALL; c++) {
		cout << "_______持ち駒_______" << endl;
		(c == BLACK) ? os << "    先手 " << std::endl : os << "    後手 " << std::endl;
		os << pos.hand(c) << endl;
	}

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
	os << "hash " << (pos.state()->board_ + pos.state()->hands_) << endl;
	os <<"position "<< pos.make_sfen() << endl;
	return os;
}

void Position::check_longeffect() {


	Bitboard rook = occ_pt(WHITE, ROOK) | occ_pt(BLACK, ROOK) | occ_pt(WHITE, DRAGON) | occ_pt(BLACK, DRAGON);
	Bitboard effect;


	while (rook.isNot()) {

		Square sq = rook.pop();
		/*int8_t obstacle_tate = (occ_all().b[index_tate(sq)] >> shift_tate(sq))&effectmask;
		int8_t obstacle_yoko = (occ_90().b[index_yoko(sq)] >> shift_yoko(sq))&effectmask;*/
		int8_t obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (occ256.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];

		cout << "ROOK sq " << sq << endl << effect << endl;

	}
	Bitboard bishop = occ_pt(WHITE, BISHOP) | occ_pt(BLACK, BISHOP) | occ_pt(WHITE, UNICORN) | occ_pt(BLACK, UNICORN);

	while (bishop.isNot()) {
		Square sq = bishop.pop();
		/*int obstacle_plus45 = (occ_plus45().b[index_plus45(sq)] >> shift_plus45(sq))&effectmask;
		int obstacle_Minus45 = (occ_minus45().b[index_Minus45(sq)] >> shift_Minus45(sq))&effectmask;*/
		int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];

		cout << "BISHOP sq " << sq << endl << effect << endl;
	}
}

void Position::check_longeffect256()
{
	Bitboard rook = occ_pt(WHITE, ROOK) | occ_pt(BLACK, ROOK) | occ_pt(WHITE, DRAGON) | occ_pt(BLACK, DRAGON);
	Bitboard effect;


	while (rook.isNot()) {

		Square sq = rook.pop();
		int8_t obstacle_tate = (occ256.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
		int8_t obstacle_yoko = (occ256.b64(1)>> occ256_shift_table_yoko[sq])&effectmask;
		effect = LongRookEffect_tate[sq][obstacle_tate] | LongRookEffect_yoko[sq][obstacle_yoko];

		cout << "ROOK sq " << sq << endl << effect << endl;

	}
	Bitboard bishop = occ_pt(WHITE, BISHOP) | occ_pt(BLACK, BISHOP) | occ_pt(WHITE, UNICORN) | occ_pt(BLACK, UNICORN);

	while (bishop.isNot()) {
		Square sq = bishop.pop();
		int obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
		int obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
		effect = LongBishopEffect_plus45[sq][obstacle_plus45] | LongBishopEffect_minus45[sq][(obstacle_Minus45)];

		cout << "BISHOP sq " << sq << endl << effect << endl;
	}



}





#define REMOVE_OCC/*(occupied,occ90,occp45,occm45,from)*/ { occupied^= SquareBB[from]; occ90^= SquareBB[sq_to_sq90(from)];occp45^= SquareBB[sq_to_sqplus45(from)];occm45^=SquareBB[sq_to_sqminus45(from)]; }
//
//Bitboard Position::attackers_to(Color stm, Square to, Bitboard& occupied, Bitboard& oc90, Bitboard& ocm45, Bitboard& ocp45) const {
//
//	//手番側の攻撃ごま
//	//HDK(SquirrelはUDK）一つにまとめたほうがいいか？あとPROMOTE小駒も
//	//う〜むこれだとだいぶ重いだろうな....
//	Color enemy = opposite(stm);
//	
//	//return (step_effect(enemy, PAWN, to)&occ_pt(stm, PAWN))
//	//	| (lance_effect(occupied, enemy, to)&occ_pt(stm, LANCE))
//	//	| (step_effect(enemy, KNIGHT, to)&occ_pt(stm, KNIGHT))
//	//	| (step_effect(enemy, SILVER, to)&occ_pt(stm, SILVER))
//	//	| (step_effect(enemy, GOLD, to)&(occ_pt(stm, GOLD) | occ_pt(stm, PRO_PAWN) | occ_pt(stm, PRO_LANCE) | occ_pt(stm, PRO_NIGHT) | occ_pt(stm, PRO_SILVER)))
//	//	| (step_effect(enemy, KING, to)&(occ_pt(stm, KING) | occ_pt(stm, DRAGON) | occ_pt(stm, UNICORN)))//王が最後に残る場合もあるので王も含めておく
//	//	| (rook_effect(occupied, oc90, to)&(occ_pt(stm, ROOK) | occ_pt(stm, DRAGON)))
//	//	| (bishop_effect(ocp45, ocm45, to)&(occ_pt(stm, BISHOP) | occ_pt(stm, UNICORN)))
//	//	;
//	re
//}

Bitboard Position::attackers_to(Color stm, Square to, Occ_256& occ) const
{
	//手番側の攻撃ごま
	//HDK(SquirrelはUDK）一つにまとめたほうがいいか？あとPROMOTE小駒も
	//う〜むこれだとだいぶ重いだろうな....
	Color enemy = opposite(stm);

	return (step_effect(enemy, PAWN, to)&occ_pt(stm, PAWN))
		| (lance_effect(occ, enemy, to)&occ_pt(stm, LANCE))
		| (step_effect(enemy, KNIGHT, to)&occ_pt(stm, KNIGHT))
		| (step_effect(enemy, SILVER, to)&occ_pt(stm, SILVER))
		| (step_effect(enemy, GOLD, to)&(occ_pt(stm, GOLD) | occ_pt(stm, PRO_PAWN) | occ_pt(stm, PRO_LANCE) | occ_pt(stm, PRO_NIGHT) | occ_pt(stm, PRO_SILVER)))
		| (step_effect(enemy, KING, to)&(occ_pt(stm, KING) | occ_pt(stm, DRAGON) | occ_pt(stm, UNICORN)))//王が最後に残る場合もあるので王も含めておく
		| (rook_effect(occ,to)&(occ_pt(stm, ROOK) | occ_pt(stm, DRAGON)))
		| (bishop_effect(occ, to)&(occ_pt(stm, BISHOP) | occ_pt(stm, UNICORN)))
		;
}


Bitboard Position::attackers_to_all(const Square to,const Occ_256& occ) const
{
	//手番側の攻撃ごま
	//HDK(SquirrelはUDK）一つにまとめたほうがいいか？あとPROMOTE小駒も
	//う〜むこれだとだいぶ重いだろうな....
	

	return (step_effect(BLACK, PAWN, to)&occ_pt(WHITE, PAWN))
		| (step_effect(WHITE, PAWN, to)&occ_pt(BLACK, PAWN))

		| (lance_effect(occ, BLACK, to)&occ_pt(WHITE, LANCE))
		| (lance_effect(occ, WHITE, to)&occ_pt(BLACK, LANCE))

		| (step_effect(BLACK, KNIGHT, to)&occ_pt(WHITE, KNIGHT))
		| (step_effect(WHITE, KNIGHT, to)&occ_pt(BLACK, KNIGHT))

		| (step_effect(BLACK, SILVER, to)&occ_pt(WHITE, SILVER))
		| (step_effect(WHITE, SILVER, to)&occ_pt(BLACK, SILVER))

		| (step_effect(BLACK, GOLD, to)&(occ_pt(WHITE, GOLD) | occ_pt(WHITE, PRO_PAWN) | occ_pt(WHITE, PRO_LANCE) | occ_pt(WHITE, PRO_NIGHT) | occ_pt(WHITE, PRO_SILVER)))
		| (step_effect(WHITE, GOLD, to)&(occ_pt(BLACK, GOLD) | occ_pt(BLACK, PRO_PAWN) | occ_pt(BLACK, PRO_LANCE) | occ_pt(BLACK, PRO_NIGHT) | occ_pt(BLACK, PRO_SILVER)))

		| (step_effect(BLACK, KING, to)&(occ_pt(WHITE, KING) | occ_pt(WHITE, DRAGON) | occ_pt(WHITE, UNICORN)))//王が最後に残る場合もあるので王も含めておく
		| (step_effect(WHITE, KING, to)&(occ_pt(BLACK, KING) | occ_pt(BLACK, DRAGON) | occ_pt(BLACK, UNICORN)))//王が最後に残る場合もあるので王も含めておく

		| (rook_effect(occ, to)&(occ_pt(BLACK, ROOK) | occ_pt(WHITE, DRAGON)))
		| (rook_effect(occ, to)&(occ_pt(WHITE, ROOK) | occ_pt(BLACK, DRAGON)))

		| (bishop_effect(occ, to)&(occ_pt(BLACK, BISHOP) | occ_pt(WHITE, UNICORN)))
		| (bishop_effect(occ, to)&(occ_pt(WHITE, BISHOP) | occ_pt(BLACK, UNICORN)))

		;
}




/*
NO_PIECE, PAWN, LANCE, KNIGHT, SILVER, BISHOP, ROOK, GOLD, 
PRO_PAWN, PRO_LANCE, PRO_NIGHT, PRO_SILVER, UNICORN, DRAGON,KING,
*/
const Piece pieces[PT_ALL] = { NO_PIECE,PAWN, LANCE, KNIGHT, SILVER,PRO_PAWN, PRO_LANCE, PRO_NIGHT,PRO_SILVER,GOLD, BISHOP, ROOK,UNICORN,DRAGON,KING };
Piece Position::min_attacker_pt(const int index, const Color stm, const Square to, const Bitboard & stmattacker, Bitboard & allattackers, Occ_256 & occ, Bitboard& occupied_)const
{
	ASSERT(0 < index&&index < PT_ALL);

	if (pieces[index]==KING) { return KING; }

	Bitboard b = stmattacker &occ_pt(stm, pieces[index]);;
	if (b.isNot()==false) { return min_attacker_pt(index+1,stm, to, stmattacker, allattackers, occ, occupied_);}

	//bにあった駒は移動するはずなので取り除く
	Square sq = b.pop();
	occ ^= SquareBB256[sq];
	occupied_ ^= SquareBB[sq];

	Piece pt = piece_type(piece_on(sq));
	//成れる場合は成った値を返す
	if (can_promote(pt) && ((SquareBB[to] & canPromoteBB[stm]).isNot() || (SquareBB[sq] & canPromoteBB[stm]).isNot())) {
		pt = promotepiece(pt);
	}
	ASSERT(pt != KING);
	Direction d = direct_table[sq][to];

	uint8_t obstacle_tate;
	uint8_t obstacle_yoko;
	uint8_t obstacle_plus45;
	uint8_t obstacle_Minus45;


	//駒が移動したことで新たにtoに駒の危機が追加されるかもしれない。
	if (d) {
		switch (d)
		{
		case UP:
			//fromから見てtoが上方向にあれば飛車の効きと先手の香車がtoに効いてくる可能性がある。
			obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
			//toに香車を置いた時のwhite側の効きに駒があるか
			allattackers |= LanceEffect[WHITE][to][obstacle_tate] & (occ_pt(BLACK, ROOK) | occ_pt(BLACK, DRAGON) | occ_pt(WHITE, ROOK) | occ_pt(WHITE, DRAGON) | occ_pt(BLACK, LANCE));
			break;
		case RightUP:
		case LeftDOWN:
			obstacle_plus45 = (occ256.b64(2) >> occ256_shift_table_p45[sq])&effectmask;
			allattackers |= LongBishopEffect_plus45[sq][obstacle_plus45] & (occ_pt(BLACK, BISHOP) | occ_pt(BLACK, UNICORN) | occ_pt(WHITE, BISHOP) | occ_pt(WHITE, UNICORN));
			break;
		case RightDOWN:
		case LeftUP:
			//斜め方向であれば角の効き
			obstacle_Minus45 = (occ256.b64(3) >> occ256_shift_table_m45[sq])&effectmask;
			allattackers |= LongBishopEffect_minus45[sq][(obstacle_Minus45)] & (occ_pt(BLACK, BISHOP) | occ_pt(BLACK, UNICORN) | occ_pt(WHITE, BISHOP) | occ_pt(WHITE, UNICORN));
			break;
		case DOWN:
			//飛車　後手の効き
			obstacle_tate = (occ.b64(0) >> occ256_shift_table_tate[sq])&effectmask;
			allattackers |= LanceEffect[BLACK][sq][obstacle_tate] & (occ_pt(BLACK, ROOK) | occ_pt(BLACK, DRAGON) | occ_pt(WHITE, ROOK) | occ_pt(WHITE, DRAGON) | occ_pt(WHITE, LANCE));
			break;
		case Left:
		case Right:
			//飛車の効き
			obstacle_yoko = (occ.b64(1) >> occ256_shift_table_yoko[sq])&effectmask;
			allattackers |= LongRookEffect_yoko[sq][obstacle_yoko] & (occ_pt(BLACK, ROOK) | occ_pt(BLACK, DRAGON) | occ_pt(WHITE, ROOK) | occ_pt(WHITE, DRAGON));
			break;
		default:
			UNREACHABLE;
			break;
		}
	}
	//ここでallttackersからsqは消される。
	allattackers = allattackers & occupied_;

	return pt;
}


bool Position::see_ge(const Move m, const Value v) const
{
	ASSERT(is_ok(m));

	Square to = move_to(m);
	Piece nextVictim = piece_type(moved_piece(m));//動かす駒は取られてしまう

	Color stm =  opposite( piece_color(moved_piece(m)));
	Value balance;
	Bitboard oc, stmAttackers;
	Occ_256 occ_256;

	balance = (Value)Eval::capture_value[(piece_on(to))];

	//最初に捕獲した駒の価値がｖを超えなかったので
	if (balance < v) return false;

	
	if (nextVictim == KING) return true;

	balance -= (Value)Eval::capture_value[nextVictim];//動かした駒はやられる

	if (balance >= v) return true;//自分の駒がとられてもbalance>vであるのでここで取り合いをやめればこちらの勝ち

	Square from = move_from(m);

	//fromのこまもtoの駒も取られている
	oc = is_drop(m) ? occ_all() ^ SquareBB[to]: occ_all() ^ SquareBB[to]^SquareBB[from];
	occ_256 = is_drop(m) ? ret_occ_256() ^ SquareBB256[to] : ret_occ_256() ^ SquareBB256[to] ^ SquareBB256[from];

	//すべてのtoに危機のある駒
	Bitboard attackers = attackers_to_all(to, occ_256)&oc;
	//pinnerとblockerの作成(sfみたいにdo_moveするときに作るのがベストか？)
	//Bitboard pinner[ColorALL], blocker[ColorALL];
	//slider_blockers(BLACK, ksq(WHITE), pinner[BLACK], blocker[WHITE]);
	//slider_blockers(WHITE, ksq(BLACK), pinner[WHITE], blocker[BLACK]);

	bool relativeStm = true; // True if the opponent is to move　相手が動こうとしている予定ならtrueになる？？　(相対side to move)(相手番の時はtrueになる)

	while (true)
	{
		stmAttackers = attackers&occ(stm);

		// Don't allow pinned pieces to attack pieces except the king as long all
		// pinners are on their original square.
		/*
		ピンされている駒(blocker)はpinゴマが元の場所にある限り攻撃に参加してはいけない
		*/
		if (!(st->pinner[opposite(stm)] & ~oc).isNot()) { stmAttackers.andnot(st->blocker[stm]); }//pinnerとblockerのcolorはこれでいいのか？？？

		if (!stmAttackers.isNot()) { return relativeStm; }

		nextVictim = min_attacker_pt(1,stm, to, stmAttackers, attackers, occ_256, oc);
		

		if (nextVictim == KING) { return relativeStm == (attackers&occ(opposite(stm))).isNot();}

		balance += relativeStm ? (Value)Eval::capture_value[nextVictim] : -(Value)Eval::capture_value[nextVictim];

		relativeStm = !relativeStm;

		/*
		relativestm==true&&balance>=v ||
		relativestm==false&&balance<=v
		であればrelativestm　側が有利なのでそちらのboolを返す
		*/
		if (relativeStm == (balance >= v)) {
			return relativeStm;
		}
		stm = opposite(stm);
	}



}


bool Position::is_nyugyoku() const{

	/*
//(a) 宣言側の手番である。(調べる必要はない)
//(b) 宣言側の玉が敵陣三段目以内に入っている。
(c) 宣言側が(大駒5点小駒1点の計算で) (大駒は飛角竜馬)
・先手の場合28点以上の持点がある。
・後手の場合27点以上の持点がある。
・点数の対象となるのは、宣言側の持駒と敵陣三段目
以内に存在する玉を除く宣言側の駒のみである。
//(d) 宣言側の敵陣三段目以内の駒は、玉を除いて10枚以上存在する。
//(e) 宣言側の玉に王手がかかっていない。(詰めろや必死であることは関係ない)
//(f) 宣言側の持ち時間が残っている。(切れ負けの場合)（調べる必要はない）
	*/
	const Color stm = sidetomove();
	const Bitboard field = canPromoteBB[stm];
	//(e)宣言側の玉に王手がかかっていない。
	if (is_incheck()) { return false; }
	//(b) 宣言側の玉が敵陣三段目以内に入っている。
	if (!(SquareBB[ksq(stm)] & field).isNot()) { return false; }

	
	const Bitboard nyuugyokuBB = field&occ(stm);
	const int threshold = (stm == BLACK) ? 28 : 27;

	//(d) 宣言側の敵陣三段目以内の駒は、玉を除いて10枚以上存在する。
	//王を含めて11枚以上であればいい
	if (nyuugyokuBB.popcount() < 11) { return false; }

	Bitboard nyuugyokuBig = nyuugyokuBB&(occ_pt(stm, ROOK) | occ_pt(stm, BISHOP) | occ_pt(stm, DRAGON) | occ_pt(stm, UNICORN));
	Bitboard nyuugyokuSmall = andnot(nyuugyokuBB,nyuugyokuBig);//ここにはkingも含まれている
	ASSERT(!(nyuugyokuBig | nyuugyokuSmall).andnot(nyuugyokuBB).isNot());
	int score=0;

	score += nyuugyokuBig.popcount() * 5;
	score += nyuugyokuSmall.popcount()-1;//kingの分１を引く

	//これを呼び出すのは終盤なので終盤なので手ごまは持ってるだろう
	for (Piece p = GOLD; p > NO_PIECE;p--) {
		int num = num_pt(hand(stm), p);
		if (p == ROOK || p == BISHOP) { score += 5 * num; }
		else { score += num; }
		if (score >= threshold) { goto WIN_NYUUGYOKU; }
	}
	
	return false;
WIN_NYUUGYOKU:
	return true;
}

/*
千日手　
http://www2.computer-shogi.org/wcsc27/　
https://ja.wikipedia.org/wiki/%E5%8D%83%E6%97%A5%E6%89%8B

将棋においては駒の配置、両対局者の持ち駒の種類や数、手番が全く同じ状態が1局中に4回現れると千日手になる

千日手は指し直さず、引き分けとする。
但し、連続王手の千日手（同一局面の最初と４回目の間の一方の指し手が王手のみだった場合）は、
連続王手をかけていたほうが負けとなる

これ、前にnullmoveが入っていたりするとややこしいな...

王手をかけた千日手をする→望ましくない
王手をかけた千日手をされる→望ましい

勝っている局面で王手のない千日手をする→望ましくない
負けている局面で王手のない千日手をする→引き分けになるので望ましい


一方の指し手が王手のみじゃないといけないのか！！！厳しい....
もうめんどくさいし4回目出てきたらreturn trueでいいか()
*/
#ifdef SENNICHI
bool Position::is_sennichite(){
	for (ReputationInfo& a : reputaion_infos) {
		if (a.key == key()) {
		//千日手局面である
			int count = a.count+1;
		if (count == 4) {
			return true;
		}
		break; }
	}
	return false;
}
#endif



bool Position::operator==(const Position & p) const
{
	if (sidetomove() != p.sidetomove()) { return false; }//手番が違う
	//盤上の駒が違う
	for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
		if (piece_on(sq) != p.piece_on(sq)) { return false; }
	}
	//持ち駒があってるか
	if ((uint32_t)hand(BLACK) != (uint32_t)p.hand(BLACK)) { return false; }
	//盤上とhand(BLACK)が合っているのならhand_whiteもあっている



	return true;
}
