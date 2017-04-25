#include "position.h"


#define matedrop
#define matemove
/*
王手一手詰めテスト局面

飛車うち　詰ませることができる
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b RSr4p 1 OK

飛車うち　ほかの駒で飛車とれる
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RSr4p 1 ok

飛車うち　逃げれる
position sfen lngp1gRnl/2G3Gb1/pp3Sppp/3k5/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RS3p 1 OK

飛車うち　pinゴマ動かすので飛車とれない
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b RS3p 1 OK

金うち　詰ませることができる
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b GSr4p 1 OK

金うち　ほかの駒で飛車とれる
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GSr4p 1 OK

金うち　逃げれる
position sfen lngp1gRnl/2G3Gb1/pp3Sppp/3k5/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GS3p 1 OK

金うち　pinゴマ動かすので飛車とれない
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b GS3p 1 OK


銀うち　　　詰ませることができる
position sfen ln2k2nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNSGKGSNL b Sr4p 1 OK
ぎん　ほかの駒で取れる
position sfen ln1gkg1nl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b Sr4p 1 OK

銀　pinゴマ動かすので飛車とれない
position sfen lngpkgRnl/2G3Gb1/pp3Sppp/9/9/9/PPPPPPPPP/1B7/LNS1K1SNL b S3p 1　OK




金の移動　詰む
position sfen lnsg1gsnl/2pppp1b1/pp1R2ppp/4k4/1B3R3/4G4/PPPP1PPPP/9/LNSGK1SNL b P 1

金移動　詰まない
position sfen lnsg1gsnl/2pppp1b1/pp1R2ppp/4k4/5R3/4G4/PPPP1PPPP/1B7/LNSGK1SNL b P 1

ピンは移動させない
position sfen lnsg1gsn1/2pppp1b1/pp1R2ppp/4k4/1B2lR3/4G4/PPPP1PPPP/9/LNSGK1SNL b P 1


*/


/*
一手詰め関数

一つでも相手の王を詰ませるような差し手を発見できればこちらのもの

---------------とびゴマによる離れた王手
近接王手で詰むのなら離れた王手でも手数はかかるが詰む。逆は真ではない。
とびゴマによる離れた王手を考えるのは相手の持ちゴマがない場合のみを考える。

---------------駒うち
(駒のいない場所)かつ(ksqに相手の駒種に打って発生する効きのある場所)かつ(近接王手であればほかの駒に支えられているところ)のみを考える。
;;;;;;
ksqをoccから取り除くocc2
玉の逃げ先にほかの駒の効きがある。
またはほかの駒でその駒を捕獲できない。
この場合は詰み。

考えるのは
①飛車(これで詰まなければ金銀角の斜めと桂の駒うちのみを考える)
②角(これで積まなければ金銀の縦横と香車と桂の駒うちのみを考える。（飛車は持っていなかったことが上で確認されている。）)
③金（詰まなければ銀の斜め後ろ,桂馬、香車）
4銀
5香車
6桂馬

いや周囲八升に発生する効きのある位置が変わってくるのでこれではだめ！！！

この順番。

---------------駒の移動
王手できる範囲にいる駒を王手をかけることができるマスかつ味方の効きのあるマスに動かしたとき、
その駒をほかの駒で取れないまたは王の逃げ先にほかの味方の効きがあれば詰み。

これは先に考えた駒によって次の駒の移動を楽できるとかそういうのはないと思う。
駒の移動によって盤上の効きが変わってしまうので

---------------間接王手　空き王手
相手の持ち駒が存在しないときに考える
駒を打ったマスと王のあいだ+駒を打った升に移動できる相手の駒があるのでは詰まない。
*/
//template<Color us> 
Move Position::mate1ply()
{
	const Color us = sidetomove();
	//ASSERT(us == sidetomove());
	const Color enemy = opposite(us);
	const Square eksq = ksq(enemy);//詰ませたい相手玉の位置。
	const Hand h = hand(us);//手番側の持ち駒。

	Bitboard f_effect = ZeroBB;
	Bitboard can_dropBB = andnot(ALLBB, occ(BLACK) | occ(WHITE));//駒のいない場所BB
	const Occ_256 occ_without_eksq = occ256^SquareBB256[eksq];//相手の王の場所を除いたoccupied
	//玉を移動させる前の王の周り8マスで味方の駒の効きがある場所のBB
	//この方法はどうかと思うけれど....(´･ω･｀)
	Bitboard friend_effectBB[8] = {ZeroBB};
	Bitboard matecandicateBB;
	Bitboard can_escape;

	int d[8] = { -10,-9,-8,-1,1,8,9,10 };
	bool didrookdrop=false;
	bool didgolddrop = false;
	bool didbishopdrop = false;

	//---------今のところ駒うちだけ見る
	if (h == (Hand)0) { goto movecheck; }

	/*
	641
	7k2
	853
	*/
	//int dx[8] = { -9,-9,-9,0,0,9,9,9 }, dy[8] = { -1,0,1,-1,1,-1,0,1 };

	for (int i = 0; i < 8; i++) {
		Square around_ksq = eksq + d[i];
		if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {
			
			//こうした方がその升にどこから駒が聞いているかわかるのでいいか？(とび機器の原因がわかる。)
			friend_effectBB[i] = attackers_to(us, around_ksq, occ256);
			if (friend_effectBB[i].isNot()) {
				//------------------------------------------------------------------------------------------
				//そうか....feffectは駒を打った時に遮られて状態が変わってしまうこともあるのか....
				//これは駒を打つ場所を決めるときには使えるが、王が逃げる場所を確定させるときには使えない。
				//------------------------------------------------------------------------------------------
				f_effect |= SquareBB[around_ksq];
			}
		}
	}
	//まずはここまで動いているか確認するか(´･ω･｀)　→OK
	//cout <<"feffect"<<endl<< f_effect << endl;

	//-----------------------------------------------------------------駒うち
	//----------------------------------------------飛車
#ifdef matedrop
	if (num_pt(h, ROOK) != 0) {
		//近接王手の駒を打てる味方の効きの聞いている場所のみを考える(飛車のstepeffectなんて作ってもしゃーないと思っていたけれどこんなところで役に立つとはなぁ(´･ω･｀))
		matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

//		cout <<"matecandicate"<<endl<< matecandicateBB << endl;
		Square to;
		
		//王手をかけることのできる、一手離れた、味方の効きが存在した場合
		while (matecandicateBB.isNot()) {
			to = matecandicateBB.pop();

			
	//		cout << to << endl;
			//王が逃げることのできる升
			can_escape =andnot( step_effect(enemy,KING,eksq),/*f_effect*/SquareBB[to]| rook_effect(occ_without_eksq, to));

//			cout <<"canescape"<<endl<< can_escape << endl;
			//王が逃げられないまたはほかの駒で王手駒を捕獲できない　。。。。。。。。。王手駒を捕獲しようとした駒がpinされていた場合はそれは詰みになってしまうな
			//この方法はかなり遅いと思う
			//駒を移動させてpinゴマの効きが通らないかチェックするのは簡単ではないな.......
			if (cancapture_checkpiece(to) == true) { 
				goto cant_matedrop_rook; 
			}
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。

			//setはcancaptureの後で行う
			set_occ256(to);
			put_piece(us, ROOK, to);

			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) { 
					remove_occ256(to);
					remove_piece(us, ROOK, to);
					goto cant_matedrop_rook; 
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。
			remove_occ256(to);
			remove_piece(us, ROOK, to);
			
			return make_drop(to,add_color(ROOK,us));
cant_matedrop_rook:;
			
		}
	
		//(これで詰まなければ金銀角の斜めと桂の駒うちのみを考える)いや周囲八升に発生する効きのある位置が変わってくるのでこれではだめ！！
		//これで詰まないということからわかるのは香車と金の後ろ打ちでは詰まないというくらいのものか....厳しいな...
		didrookdrop = true;
	}

	//ここまでOK

	//---------------------------------金


	if (num_pt(h, GOLD) != 0) {

		Square kingback = ((us == BLACK) ? eksq - Square(1) : eksq + Square(1));
		//kingが端っこにいれば後ろには打てないのでerrorSqにしておく
		if ((us == BLACK&&sqtorank(eksq) == RankA) || (us == WHITE&&sqtorank(eksq) == RankI)) { kingback = Error_SQ; }

		matecandicateBB = can_dropBB&StepEffect[enemy][GOLD][eksq] & f_effect;
		
		//もし飛車うって詰まないことが確認されていて、玉が端っこにいなければ、後ろから金を打つのをやめる必要がある
		if (didrookdrop&&kingback != Error_SQ) { andnot(matecandicateBB, SquareBB[kingback]); }


		
//		cout << "matecandicate" << endl << matecandicateBB << endl;
		//王手をかけることのできる、一手離れた、味方の効きが存在した場合
		while (matecandicateBB.isNot()) {
			 const Square to = matecandicateBB.pop();

		
//			cout << to << endl;
			//王が逃げることのできる升
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect|*/SquareBB[to]|StepEffect[us][GOLD][to]);

//			cout << "canescape" << endl << can_escape << endl;
			
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_gold; }

			set_occ256(to);
			put_piece(us, GOLD, to);
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。
			while(can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, GOLD, to); 
					goto cant_matedrop_gold;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。ここでtoが0になってしまっている！！
			remove_occ256(to);
			remove_piece(us, GOLD, to);
			//cout << "mate GOLD:"<<to<< endl;
			return make_drop(to, add_color(GOLD,us));
cant_matedrop_gold:;
			
		}
		didgolddrop = true;
	}

	//------------------------------------角
	if (num_pt(h, BISHOP)) {

		matecandicateBB = can_dropBB&StepEffect[enemy][BISHOP][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			can_escape = andnot(step_effect(enemy, KING, eksq),/* f_effect*/SquareBB[to] | bishop_effect(occ_without_eksq,to));
//			cout << "canescape" << endl << can_escape << endl;

			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_bishop; }

			set_occ256(to);
			put_piece(us, BISHOP, to);
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
	//			cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, BISHOP, to);
					goto cant_matedrop_bishop;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。
			remove_occ256(to);
			remove_piece(us, BISHOP, to);
			//cout << "mate BISHOP" << endl;
			return make_drop(to, add_color(BISHOP,us));
cant_matedrop_bishop:;
		}
		didbishopdrop = true;
	}
	//-----------------------------------銀
	/*
	金で詰まなかった場合は斜め後ろからのみを考える。
	角で詰まなかった場合は前からのみを考える
	両方で詰まなかった場合は考える必要はない。
	*/
	if (num_pt(h, SILVER) != 0&&(!didbishopdrop||!didgolddrop)) {

		matecandicateBB = can_dropBB&StepEffect[enemy][SILVER][eksq] & f_effect;
		if (didbishopdrop) { matecandicateBB=matecandicateBB & StepEffect[enemy][GOLD][eksq]; }
		else if (didgolddrop) { andnot(matecandicateBB, StepEffect[enemy][GOLD][eksq]); }

//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			can_escape = andnot(step_effect(enemy, KING, eksq),/* f_effect */SquareBB[to]| StepEffect[us][SILVER][to]);
//			cout << "canescape" << endl << can_escape << endl;
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_silver; }

			set_occ256(to);
			put_piece(us, SILVER, to);
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, SILVER, to);
					goto cant_matedrop_silver;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。
			remove_occ256(to);
			remove_piece(us, SILVER, to);
			return make_drop(to, add_color(SILVER,us));
cant_matedrop_silver:;
		}
	}
	//---------------------------------香車
	//飛車で詰まなかった場合は考える必要はない
	if (num_pt(h, LANCE) != 0&&!didrookdrop) {
		matecandicateBB = can_dropBB&StepEffect[enemy][LANCE][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect*/SquareBB[to] | lance_effect(occ_without_eksq,us,to));
//			cout << "canescape" << endl << can_escape << endl;
			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_LANCE; }

			set_occ256(to);
			put_piece(us, LANCE, to);
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, LANCE, to);
					goto cant_matedrop_LANCE;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。
			remove_occ256(to);
			remove_piece(us, LANCE, to);
			return make_drop(to, add_color(LANCE,us));
		cant_matedrop_LANCE:;
		}
	}
	//---------------------------------桂馬
	if (num_pt(h, KNIGHT) != 0) {
		//桂馬はほかの駒で支えてもらう必要はない
		matecandicateBB = can_dropBB&StepEffect[enemy][KNIGHT][eksq];
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			can_escape = andnot(step_effect(enemy, KING, eksq), /*f_effect*/SquareBB[to]);
//			cout << "canescape" << endl << can_escape << endl;

			if (cancapture_checkpiece(to) == true) { goto cant_matedrop_KNIGHT; }

			set_occ256(to);
			put_piece(us, KNIGHT, to);
			//そうでない場合は逃げ先の候補を一つ一つ検証していく。
			while (can_escape.isNot()) {
				const Square escapeto = can_escape.pop();//逃げ先。
//				cout << "escape to:" << escapeto << endl;
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					remove_occ256(to);
					remove_piece(us, KNIGHT, to);
					goto cant_matedrop_KNIGHT;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。
			}
			//goto文で飛ばされなかったということはつまされた。
			remove_occ256(to);
			remove_piece(us, KNIGHT, to);
			return make_drop(to, add_color(KNIGHT,us));
cant_matedrop_KNIGHT:;
		}

	}
#endif
movecheck:;

	//----------------------------------ここから駒の移動による王手



#ifdef matemove
	//dc_candicateとは二重王手候補つまり王への味方の効きを遮っている味方の駒。もしこれで王手をかけることができれば二重王手になりうるし、効きから外れるだけでも間接王手になる。

	//pinedとはpinされている駒この駒を効きを作っている駒の方向以外へ動かしての王手はできないし、もしその方向で王手をできたとしてもpinしてる駒をとれなかった場合は取り返される。
	//つまりpinゴマによる王手はかなり複雑で詰ませることのできる可能性は少ないので,pinゴマによる王手は考えないほうがいいのかもしれない
	//Bitboard dc_candicate[ColorALL],pinned[ColorALL];

	////slider_blockers(us, eksq, dc_candicate[us], pinned[enemy]);//攻め
	//slider_blockers(enemy, ksq(us), dc_candicate[enemy], pinned[us]);//受け ほしいのは自分のpinゴマの位置

	//駒の移動先。近接王手のみを考えるのでesqに味方のgoldを置いた時の危機がある場所、自分の駒のいない場所でなければならない
	const Bitboard movetoBB_GOLD = andnot(StepEffect[enemy][GOLD][eksq], occ(us));
	/*------------------------------------------------------------------------------------------
	王手できる範囲にいる駒を王手をかけることができるマスかつ味方の効きのあるマスに動かしたとき、
		その駒をほかの駒で取れないまたは王の逃げ先にほかの味方の効きがあれば詰み。

		考える王手は近接王手のみ

		これは先に考えた駒によって次の駒の移動を楽できるとかそういうのはないと思う。
		駒の移動によって盤上の効きが変わってしまうので
	-------------------------------------------------------------------------------------------*/


	//ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーまずは詰ませる可能性の高い金から　OKワンパスは通った。
	Bitboard matecandicate_Gold = (occ_pt(us, GOLD)|occ_pt(us,PRO_PAWN)|occ_pt(us,PRO_LANCE)|occ_pt(us,PRO_NIGHT)|occ_pt(us,PRO_SILVER))&PsuedoGivesCheckBB[us][GOLD][eksq];
	//pinゴマを動かそうとしてはいけない（まあpinをしている駒をとることで王手できる場合もあるがそれはイレギュラーなので考えないほうがいいだろう）
	matecandicate_Gold = andnot(matecandicate_Gold, st->blocker[us]);

	//std::cout << matecandicate_Gold << std::endl;

	while (matecandicate_Gold.isNot()) {

		const Square from = matecandicate_Gold.pop();
		Bitboard toBB = movetoBB_GOLD&StepEffect[us][GOLD][from];
		Bitboard cankingescape;
		//駒を取り除く.......こんなことしたくないんだけれど....
		const Piece removedpiece =piece_type( piece_on(from));
		ASSERT(removedpiece);

		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		//std::cout << toBB << endl;

		while (toBB.isNot())
		{
			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);
			//std::cout << to << endl << attackers_to(us, to, occ256) << endl;

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_mate_gold; }//もし移動先に味方の駒の効きが聞いていなければとられてしまうので罪にならない。
			if (cancapture_checkpiece(to)) { goto cant_mate_gold; }//王手をかけた駒を捕獲できた。

			//set()は^=なので,駒を捕獲しながらの王手だった場合にバグが発生してしまう！！！！！！あれっ？？そうなるとdo_moveでもおかしいことが起こってるのでは???
			//修正した
			if (!is_capture) { set_occ256(to); }

			//---------よく考えたらcaptureの場合相手の駒をoccから取り除いていることになっていないのでcancapture_checkpieceでおかしなことになってないか？？
			//----------いやtoの位置の駒がtoに効きを作ることはできないので大丈夫か...
			put_piece(us, GOLD, to);

			//ここからは玉が逃げることができるかどうか確認
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][GOLD][to]));
			//std::cout << cankingescape << endl;
			while (cankingescape.isNot()) {

				const Square escapeto = cankingescape.pop();//逃げ先。
				//std::cout << "escapeto " << escapeto << endl;
				//cout << "attackers" << endl << attackers_to(us, escapeto, occ256) << endl;

				if (!attackers_to(us, escapeto, occ256).isNot()) { 
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, GOLD, to);
					goto cant_mate_gold;
				}//逃げ先に攻撃側の効きがない。  逃げるとこができたので次のtoを考える。

			}

			//このwhileを抜けてこれたということはつまされてしまったということ。
			//こんな実装方法で大丈夫だろうか.........
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//捕獲だった場合はここで取り除いてはいけない！！！

			remove_piece(us, GOLD, to);
			return make_move(from,to, add_color(removedpiece,us));

cant_mate_gold:;

		}
		//取り除いた駒を戻す
		//絶対に戻すのを忘れないようにする！！！！！！！！！！！！
		set_occ256(from);
		put_piece(us, removedpiece, from);
		
	}
#if 0
	//-----------------------------------------竜
	const Bitboard movetoBB_DRAGON = andnot(StepEffect[us][DRAGON][eksq],occ(us));//近接王手ができる縦横4マス　離れた王手は考えない　
	Bitboard mate_candicate_Dragon = occ_pt(us, DRAGON);//飛車の場合Psuedogives checkは作成できない
	while (mate_candicate_Dragon.isNot())
	{
		const Square from = mate_candicate_Dragon.pop();
		Bitboard toBB = movetoBB_DRAGON&(rook_effect(occ256, from) | StepEffect[us][KING][from]);
		Bitboard cankingescape;
		const Piece removedpiece = DRAGON;
		ASSERT(removedpiece == piece_type(piece_on(from)));
		//駒を取り除く
		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		while (toBB.isNot()) {
		
			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_matemove_dragon; }//もし移動先に味方の駒の効きが聞いていなければとられてしまうので罪にならない。
			if (cancapture_checkpiece(to)) { goto cant_matemove_dragon; }//王手をかけた駒を捕獲できた。

			//駒をtoに置く
			if (!is_capture) { set_occ256(to); }
			put_piece(us, DRAGON, to);
			//逃げ場
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | rook_effect(occ256,to) | StepEffect[us][KING][to]));

			while (cankingescape.isNot())
			{
				const Square escapeto = cankingescape.pop();

				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, DRAGON, to);
					goto cant_matemove_dragon;
				}
			}
			//このwhileを抜けてこれたということはつまされてしまったということ。
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//捕獲だった場合はここで取り除いてはいけない！！！

			remove_piece(us, DRAGON, to);
			return make_move(from, to, add_color(removedpiece, us));


cant_matemove_dragon:;
		}
		set_occ256(from);
		put_piece(us, removedpiece, from);


	}

#endif
#if 0
	//-----------ユニコーン
	const Bitboard movetoBB_UNICORN = andnot(StepEffect[us][UNICORN][eksq], occ(us));//近接王手ができる縦横4マス　離れた王手は考えない　
	Bitboard mate_candicate_UNICORN = occ_pt(us, UNICORN);//飛車の場合Psuedogives checkは作成できない
	while (mate_candicate_UNICORN.isNot())
	{
		const Square from = mate_candicate_UNICORN.pop();
		Bitboard toBB = movetoBB_UNICORN&(bishop_effect(occ256, from) | StepEffect[us][KING][from]);
		Bitboard cankingescape;
		const Piece removedpiece = UNICORN;
		ASSERT(removedpiece == piece_type(piece_on(from)));
		//駒を取り除く
		remove_occ256(from);
		remove_piece(us, removedpiece, from);

		while (toBB.isNot()) {

			const Square to = toBB.pop();
			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_matemove_UNICORN; }//もし移動先に味方の駒の効きが聞いていなければとられてしまうので罪にならない。
			if (cancapture_checkpiece(to)) { goto cant_matemove_UNICORN; }//王手をかけた駒を捕獲できた。

																		 //駒をtoに置く
			if (!is_capture) { set_occ256(to); }
			put_piece(us, UNICORN, to);
			//逃げ場
			cankingescape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | bishop_effect(occ256, to) | StepEffect[us][KING][to]));

			while (cankingescape.isNot())
			{
				const Square escapeto = cankingescape.pop();

				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, UNICORN, to);
					goto cant_matemove_UNICORN;
				}
			}
			//このwhileを抜けてこれたということはつまされてしまったということ。
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//捕獲だった場合はここで取り除いてはいけない！！！

			remove_piece(us, UNICORN, to);
			return make_move(from, to, add_color(removedpiece, us));


		cant_matemove_UNICORN:;
		}
		set_occ256(from);
		put_piece(us, removedpiece, from);


	}
#endif

#if 0
	//------------------------------銀（成り、成らずがあるため、かなり複雑そう..............）
	//先に竜馬を調べたほうがいいかもしれない
	Bitboard matecandicate_silver = (occ_pt(us,SILVER))&PsuedoGivesCheckBB[us][SILVER][eksq];
	//pinゴマを動かそうとしてはいけない（まあpinをしている駒をとることで王手できる場合もあるがそれはイレギュラーなので考えないほうがいいだろう）
	matecandicate_silver = andnot(matecandicate_silver, st->blocker[us]);
	const Bitboard movetoBB_SILVER = andnot(StepEffect[enemy][SILVER][eksq], occ(us));

	while (matecandicate_silver.isNot()) {

		bool canpromte_from = false;
		const Square from = matecandicate_silver.pop();
		if ((SquareBB[from] & canPromoteBB[us]).isNot()) { canpromte_from = true; }//fromが3段目以内なので成ることができる
		Bitboard toBB = movetoBB_SILVER&StepEffect[us][SILVER][from];
		Bitboard canking_escape;
		ASSERT(piece_type(piece_on(from)) == SILVER); 
		ASSERT(piece_color(piece_on(from)) == us);
		//まだ成っていないので取り除く駒は銀でいい。
		const Piece removedpiece = SILVER;
		remove_occ256(from);
		remove_piece(us,SILVER, from);
//		cout <<"toBB"<<endl<< toBB << endl;
		while (toBB.isNot())
		{
			const Square to = toBB.pop();//銀を移動させる。
			bool canpromote_to;
			((SquareBB[to] & canPromoteBB[us]).isNot()) ? canpromote_to=true: canpromote_to=false;//移動先で成れるか

			if (!attackers_to(us, to, occ256).isNot()) { goto cant_mate_silver; }//移動先に味方の駒の効きが聞いていなければとられてしまう
			if (cancapture_checkpiece(to)) { goto cant_mate_silver; }//toに駒を置くと捕獲されてしまう

			const bool is_capture = (piece_on(to) != NO_PIECE);

			if (!is_capture) { set_occ256(to); }
			put_piece(us, SILVER, to);
			canking_escape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][SILVER][to]));//王の逃げ場候補
//			cout << "cankingescape" << endl << canking_escape << endl;
			while (canking_escape.isNot()) {
				const Square escapeto = canking_escape.pop();//逃げ先。
				if (!attackers_to(us, escapeto, occ256).isNot()) {
					if (!is_capture) { remove_occ256(to); }
					remove_piece(us, SILVER, to);
					goto mate_progold;
				}
			}
			//抜けてきたということは詰ませることができた
			set_occ256(from);
			put_piece(us, removedpiece, from);

			if (!is_capture) { remove_occ256(to); }//捕獲だった場合はここで取り除いてはいけない！！！

			remove_piece(us, SILVER, to);
			return make_move(from, to, add_color(removedpiece, us));

		mate_progold:;
			//なることができる場合(こんな方法じゃ遅いよなぁ..)
			if (canpromte_from || canpromote_to) {
				if ((StepEffect[us][GOLD][to] & SquareBB[eksq]).isNot() == false) { goto cant_mate_silver; }//王手をできていない
				if (!is_capture) { set_occ256(to); }
				put_piece(us, GOLD, to);

				
				canking_escape = andnot(StepEffect[us][KING][eksq], (occ(enemy) | SquareBB[to] | StepEffect[us][GOLD][to]));//王の逃げ場候補
//				cout << "cankingescape pro" << endl << canking_escape << endl;
				while (canking_escape.isNot()) {
					const Square escapeto = canking_escape.pop();//逃げ先。
					if (!attackers_to(us, escapeto, occ256).isNot()) {
						if (!is_capture) { remove_occ256(to); }
						remove_piece(us, GOLD, to);
						goto cant_mate_silver;
					}
				}

				//抜けてきたということは詰ませることができた
				set_occ256(from);
				put_piece(us, removedpiece, from);

				if (!is_capture) { remove_occ256(to); }//捕獲だった場合はここで取り除いてはいけない！！！

				remove_piece(us, GOLD, to);
				return make_movepromote(from, to, add_color(removedpiece, us));
			}
		cant_mate_silver:;
		}//while toBB
		//取り除いた駒を戻す
		set_occ256(from);
		put_piece(us, removedpiece, from);
	}
#endif





#endif
	return MOVE_NONE;
}
//Move Position::mate1ply() {
//	return sidetomove() == BLACK ? mate1ply<BLACK>() : mate1ply<WHITE>();
//}

/*
王手をかけている駒をとれるかどうか
そしてとったときに玉に王手がかからないか
*/
bool Position::cancapture_checkpiece(Square to) {

	const Color us = sidetomove();//攻め側
	const Color enemy = opposite(sidetomove());//受け側
	const Square eksq = ksq(enemy);//詰ませたい相手玉の位置。

	//受け側の王手駒へ危機のある駒の位置
	Bitboard enemygurder = effect_toBB_withouteffectking(enemy, to);

//	cout <<"gurader"<<endl<< enemygurder << endl;

	while (enemygurder.isNot()) {
		Square from = enemygurder.pop();
		Occ_256 occ_movedgurad = occ256|SquareBB256[to] ^ SquareBB256[from];//ここも捕獲の移動だった場合は^Squarebb256じゃだめ。
		//駒を動かした後,王に効きがかからなかったのでこれは王を守れた
		if (attackers_to(us, eksq, occ_movedgurad).isNot()==false) {
			return true;
		}
	}

	//王手駒をほかの駒で取れなかった。
	return false;
}

/*
sliderblockersは　sをとび機器から守っている駒の位置（pinゴマの位置）を返す。
もしbloclerを盤上から取り除けばｓは攻撃されてしまう。

stmには攻撃側の色　、sに受け側の玉の位置が格納される。
*/
void Position::slider_blockers(const Color stm, const Square s,Bitboard& dc_candicate,Bitboard& pinned) const
{
	dc_candicate = ZeroBB;
	pinned = ZeroBB;
	Bitboard betweengurad,betweenattack, pinners;

	//sから飛び効きビームを放って攻撃側のとびゴマに当たればその升はその駒によって攻撃を受けている可能性がある。
	//pinnersは攻撃ゴマ
	pinners = (
		(RookPsuedoAttack[s] & (occ_pt(stm, ROOK) | occ_pt(stm, DRAGON)))
		| BishopPsuedoAttack[s] & (occ_pt(stm, BISHOP) | occ_pt(stm, UNICORN))
		| (LancePsuedoAttack[opposite(stm)][s]&occ_pt(stm,LANCE))
		);

	while (pinners.isNot()) {

		Square to = pinners.pop();
		betweengurad = BetweenBB[s][to] & occ(opposite(stm));//これがpinされている駒候補
		betweenattack = BetweenBB[s][to] & occ(stm);//これが二重王手候補

		//間に一つしか駒がなかった(orそこに駒がなかった)のでその駒はpinされている。
		//そこに駒がなかった場合でもresultには何も足されないので大丈夫
		if (!more_than_one(betweenattack)) {dc_candicate |= betweenattack;}
		if (!more_than_one(betweengurad)) { pinned |= betweengurad; }
	}

	return;
}
