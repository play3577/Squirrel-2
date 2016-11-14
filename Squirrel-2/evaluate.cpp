#include "evaluate.h"
#include "position.h"


namespace Eval {

	int16_t piece_value[PC_ALL] = {
		Value_Zero,

		PawnValue,
		LanceValue,
		KnightValue,
		SilverValue,
		BishopValue,
		RookValue,
		GoldValue,
		KingValue,
		ProPawnValue,
		ProLanceValue,
		ProKnightValue,
		ProSilverValue,
		UnicornValue,
		DragonValue,
		0,0,

		-PawnValue,
		-LanceValue,
		-KnightValue,
		-SilverValue,
		-BishopValue,
		-RookValue,
		-GoldValue,
		-KingValue,
		-ProPawnValue,
		-ProLanceValue,
		-ProKnightValue,
		-ProSilverValue,
		-UnicornValue,
		-DragonValue,
	};

	int16_t capture_value[PC_ALL] = {
		Value_Zero,

		2*PawnValue,
		2*LanceValue,
		2*KnightValue,
		2*SilverValue,
		2*BishopValue,
		2*RookValue,
		2*GoldValue,
		2*KingValue,
		ProPawnValue+PawnValue,
		ProLanceValue+LanceValue,
		ProKnightValue+KnightValue,
		ProSilverValue+SilverValue,
		UnicornValue+BishopValue,
		DragonValue+RookValue,
		0,0,
		2 * PawnValue,
		2 * LanceValue,
		2 * KnightValue,
		2 * SilverValue,
		2 * BishopValue,
		2 * RookValue,
		2 * GoldValue,
		2 * KingValue,
		ProPawnValue + PawnValue,
		ProLanceValue + LanceValue,
		ProKnightValue + KnightValue,
		ProSilverValue + SilverValue,
		UnicornValue + BishopValue,
		DragonValue + RookValue,
	};

	int16_t diff_promote[GOLD] = {
		0,
		ProPawnValue-PawnValue,
		ProLanceValue-LanceValue,
		ProKnightValue-KnightValue,
		ProSilverValue-SilverValue,
		UnicornValue-BishopValue,
		DragonValue-RookValue,
	};



	Value eval_material(const Position & pos)
	{
		int16_t v = 0;

		//盤上
		for (Square sq = SQ_ZERO; sq < SQ_NUM; sq++) {
			v += piece_value[pos.piece_on(sq)];
		}

		//手駒
		for (Color c = BLACK; c <= WHITE; c++) {


			const auto hands = pos.hand(c);

			for (Piece pt = PAWN; pt <= GOLD; pt++) {
				v += (c == BLACK ? 1:-1)*num_pt(hands, pt)*piece_value[pt];
			}

		}
		return Value(v);
	}

	Value eval(const Position & pos)
	{
		Value value = pos.state()->material;

		Value  full = eval_material(pos);
		//駒得の差分計算ができているかチェック
		if (value != full) {
			cout << pos << endl;
			ASSERT(0);
		}


		return (pos.sidetomove() == BLACK) ? value : -value;

		/*Value value = eval_material(pos);
		return (pos.sidetomove() == BLACK) ? value : -value;*/
	}

}