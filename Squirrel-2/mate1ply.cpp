#include "position.h"

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
	cout << f_effect << endl;

	//-------------駒うち

	//まずは飛車
	if (num_pt(h, ROOK) != 0) {
		
		//--------------------飛車

		//近接王手の駒を打てる味方の効きの聞いている場所のみを考える(飛車のstepeffectなんて作ってもしゃーないと思っていたけれどこんなところで役に立つとはなぁ(´･ω･｀))
		Bitboard matecandicateBB = can_dropBB&StepEffect[enemy][ROOK][eksq]&f_effect;

		while (matecandicateBB.isNot()) {
			Square to = matecandicateBB.pop();
			Bitboard can_escape =andnot( step_effect(enemy,KING,eksq),f_effect| rook_effect(occ_without_eksq, to));
			if (can_escape.isNot()) { return true; }
		}

		//(これで詰まなければ金銀角の斜めと桂の駒うちのみを考える)角があれば角と桂馬だけでいい。

	}



	return false;
}
