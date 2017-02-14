#include "position.h"

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
bool Position::mate1ply()
{
	const Color us = sidetomove();
	const Color enemy = opposite(sidetomove());
	const Square eksq = ksq(enemy);//詰ませたい相手玉の位置。
	const Hand h = hand(sidetomove());//手番側の持ち駒。


	//---------今のところ駒うちだけ見る
	if (h == (Hand)0) { return false; }


	const Occ_256 occ_without_eksq = occ256^SquareBB256[eksq];//相手の王の場所を除いたoccupied
	Bitboard can_dropBB = andnot(ALLBB, occ(BLACK) | occ(WHITE));//駒のいない場所BB

	//玉を移動させる前の王の周り8マスで味方の駒の効きがある場所のBB
	//この方法はどうかと思うけれど....(´･ω･｀)
	Bitboard friend_effectBB[8] = {ZeroBB};
	Bitboard f_effect=ZeroBB;
	/*
	641
	7k2
	853
	*/
	//int dx[8] = { -9,-9,-9,0,0,9,9,9 }, dy[8] = { -1,0,1,-1,1,-1,0,1 };
	int d[8] = { -10,-9,-8,-1,1,8,9,10 };
	for (int i = 0; i < 8; i++) {
		Square around_ksq = eksq + d[i];
		if (is_ok(around_ksq) && abs(sqtorank(around_ksq) - sqtorank(eksq)) < 2 && abs(sqtofile(around_ksq) - sqtofile(eksq)) < 2) {
			
			//こうした方がその升にどこから駒が聞いているかわかるのでいいか？(とび機器の原因がわかる。)
			friend_effectBB[i] = attackers_to(us, around_ksq, occ256);
			if (friend_effectBB[i].isNot()) {
				f_effect |= SquareBB[around_ksq];
			}
		}
	}
	//まずはここまで動いているか確認するか(´･ω･｀)　→OK
	//cout <<"feffect"<<endl<< f_effect << endl;

	//-----------------------------------------------------------------駒うち
	bool didrookdrop = false;
	//----------------------------------------------飛車
	if (num_pt(h, ROOK) != 0) {
		//近接王手の駒を打てる味方の効きの聞いている場所のみを考える(飛車のstepeffectなんて作ってもしゃーないと思っていたけれどこんなところで役に立つとはなぁ(´･ω･｀))
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

		//cout <<"matecandicate"<<endl<< matecandicateBB << endl;

		//王手をかけることのできる、一手離れた、味方の効きが存在した場合
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
		//	cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape =andnot( step_effect(enemy,KING,eksq),f_effect| rook_effect(occ_without_eksq, to));

			//cout <<"canescape"<<endl<< can_escape << endl;
			//王が逃げられないまたはほかの駒で王手駒を捕獲できない　。。。。。。。。。王手駒を捕獲しようとした駒がpinされていた場合はそれは詰みになってしまうな
			//この方法はかなり遅いと思う
			//駒を移動させてpinゴマの効きが通らないかチェックするのは簡単ではないな.......
			if (can_escape.isNot()==false&& cancapture_checkpiece(to)==false) { return true; }//これで詰まされた。
		}

		//(これで詰まなければ金銀角の斜めと桂の駒うちのみを考える)いや周囲八升に発生する効きのある位置が変わってくるのでこれではだめ！！
		//これで詰まないということからわかるのは香車と金の後ろ打ちでは詰まないというくらいのものか....厳しいな...
		didrookdrop = true;
	}

	//ここまでOK

	//---------------------------------金
	bool didgolddrop = false;

	Square kingback = ((us == BLACK) ? eksq - Square(1) : eksq + Square(1));
	//kingが端っこにいれば後ろには打てないのでerrorSqにしておく
	if ((us == BLACK&&sqtorank(eksq) == RankA) || (us == WHITE&&sqtorank(eksq) == RankI)) {kingback = Error_SQ;}

	if (num_pt(h, GOLD) != 0) {

		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][GOLD][eksq] & f_effect;
		
		//もし飛車うって詰まないことが確認されていて、玉が端っこにいなければ、後ろから金を打つのをやめる必要がある
		if (didgolddrop&&kingback != Error_SQ) { andnot(matecandicateBB, SquareBB[kingback]); }

//		cout << "matecandicate" << endl << matecandicateBB << endl;
		//王手をかけることのできる、一手離れた、味方の効きが存在した場合
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape = andnot(step_effect(enemy, KING, eksq), f_effect | StepEffect[us][GOLD][to]);

//			cout << "canescape" << endl << can_escape << endl;
			//逃げられないかつほかの駒で取れない場合はつまされてしまっている。
			if (can_escape.isNot() == false && cancapture_checkpiece(to) == false) { return true; }//これで詰まされた。
		}
		didgolddrop = true;
	}

	//------------------------------------角
	bool didbishopdrop = false;
	if (num_pt(h, BISHOP)) {

		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][BISHOP][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape = andnot(step_effect(enemy, KING, eksq), f_effect | bishop_effect(occ_without_eksq,to));
//			cout << "canescape" << endl << can_escape << endl;
			if (can_escape.isNot() == false && cancapture_checkpiece(to) == false) { return true; }//これで詰まされた。
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

		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][SILVER][eksq] & f_effect;
		if (didbishopdrop) { matecandicateBB=matecandicateBB & StepEffect[enemy][GOLD][eksq]; }
		else if (didgolddrop) { andnot(matecandicateBB, StepEffect[enemy][GOLD][eksq]); }

//		cout << "matecandicate" << endl << matecandicateBB << endl;
		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape = andnot(step_effect(enemy, KING, eksq), f_effect | StepEffect[us][SILVER][to]);
//			cout << "canescape" << endl << can_escape << endl;
			if (can_escape.isNot() == false && cancapture_checkpiece(to) == false) { return true; }//これで詰まされた。
		}
	}
	//---------------------------------香車
	//飛車で詰まなかった場合は考える必要はない
	if (num_pt(h, LANCE) != 0&&!didrookdrop) {
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][LANCE][eksq] & f_effect;
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape = andnot(step_effect(enemy, KING, eksq), f_effect | lance_effect(occ_without_eksq,us,to));
//			cout << "canescape" << endl << can_escape << endl;
			if (can_escape.isNot() == false && cancapture_checkpiece(to) == false) { return true; }//これで詰まされた。
		}
	}
	//---------------------------------桂馬
	if (num_pt(h, KNIGHT) != 0) {
		//桂馬はほかの駒で支えてもらう必要はない
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][KNIGHT][eksq];
//		cout << "matecandicate" << endl << matecandicateBB << endl;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
//			cout << to << endl;
			//王が逃げることのできる升
			Bitboard can_escape = andnot(step_effect(enemy, KING, eksq), f_effect);
//			cout << "canescape" << endl << can_escape << endl;
			if (can_escape.isNot() == false && cancapture_checkpiece(to) == false) { return true; }//これで詰まされた。
		}

	}

	return false;
	//----------------------------------ここから駒の移動による王手
	//dc_candicateとは二重王手候補つまり王への味方の効きを遮っている味方の駒。もしこれで王手をかけることができれば二重王手になりうるし、効きから外れるだけでも間接王手になる。

	//pinedとはpinされている駒この駒を効きを作っている駒の方向以外へ動かしての王手はできないし、もしその方向で王手をできたとしてもpinしてる駒をとれなかった場合は取り返される。
	//つまりpinゴマによる王手はかなり複雑で詰ませることのできる可能性は少ないので,pinゴマによる王手は考えないほうがいいのかもしれない
	Bitboard dc_candicate[ColorALL],pinned[ColorALL];

	slider_blockers(us, eksq, dc_candicate[us], pinned[enemy]);//攻め
	slider_blockers(enemy, ksq(us), dc_candicate[enemy], pinned[us]);//受け








	
}
/*
王手をかけている駒をとれるかどうか
そしてとったときに玉に王手がかからないか
*/
bool Position::cancapture_checkpiece(Square to) {

	const Color us = sidetomove();
	const Color enemy = opposite(sidetomove());
	const Square eksq = ksq(enemy);//詰ませたい相手玉の位置。

	//受け側の王手駒へ危機のある駒の位置
	Bitboard enemygurder = effect_toBB_withouteffectking(enemy, to);

//	cout <<"gurader"<<endl<< enemygurder << endl;

	while (enemygurder.isNot()) {
		Square from = enemygurder.pop();
		Occ_256 occ_movedgurad = occ256^SquareBB256[to] ^ SquareBB256[from];
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
