#include "AperyBook.h"
#include "makemove.h"



//https://ameblo.jp/nana-2007-july/entry-10104294377.html
bool AperyBook::openbook(const std::string filename)
{
	if (is_open()) close();


	ifstream::open(filename.c_str(), std::ifstream::in | std::ifstream::binary | std::ios::ate);

	if (!is_open())
		return false;

	//tellg�̓t�@�C���̃J�����g�ʒu��Ԃ��i�����ł̓t�@�C���̖����ɃV�[�N����Ă���̂Ńt�@�C���T�C�Y�j
	size_ = tellg() / sizeof(AperyBookEntry);

	//�t�@�C���̏I�[�ɒB����|�������삪�����œ��͑��삪���s����|�X�g���[���o�b�t�@�̓��o�͑��삪���s����  �ȊO�ł����good�ɂȂ�@
	if (good() == false){
		cerr << "Failed Open Apery Book" << std::endl;
		exit(0);
	}

	filename_ = filename;

	return true;
}


//zobristhash�̏�����
void AperyBook::init()
{
	for (Piece pc = NO_PIECE; pc < PC_ALL; pc++) for (Square sq = SQ_ZERO; sq < SQ_NUM;sq++) {
		BookKeypsq[pc][sq] = Bookmt();
	}

	
	//HPawn, HLance, HKnight, HSilver, HGold, HBishop, HRook, HandPieceNum
	for (HandPiece pt = HPawn; pt < HandPieceNum; pt++) for(int num=0;num<19;num++) {
		BookKeyhand[pt][num] = Bookmt();
	}
	BookKeyTurn = Bookmt();
}

//���ǖʂ�Book�̂��߂�key��Ԃ�
Key AperyBook::RetPosBookKey(const Position & pos)
{
	Key key = 0;

	Bitboard bb = pos.occ_all();

	while (bb.isNot()) {
		Square sq = bb.pop();
		key ^= BookKeypsq[pos.piece_on(sq)][sq];
	}

	const Hand h = pos.hand(pos.sidetomove());

	for (Piece pt = PAWN; pt < KING; pt++) {
		ASSERT(pt <= GOLD&&pt >= PAWN);
		key ^= BookKeyhand[Piece2AperyHandPiece[pt]][num_pt(h, pt)];
	}

	if (pos.sidetomove() == WHITE) key ^= BookKeyTurn;

	return key;
}


//book��key�̏��������ɕ���ł���I�I
void AperyBook::binary_search(const Key key)
{
	size_t ub = size_ - 1, lb = 0, mid;
	AperyBookEntry entry;

	while (lb<ub && good()) {

		mid = (ub + lb) / 2;
		ASSERT(lb <= mid&&mid <= ub);

		//beg�Ƃ�stream��begin�̂���
		seekg(mid * sizeof(AperyBookEntry), std::ios_base::beg);
		read(reinterpret_cast<char*>(&entry), sizeof(entry));

		//key==entry.key�ƂȂ�entry�������Ă��ق����͓̂���key�����W���̐擪entry�ł���̂ŒT���𑱂���
		if (key <= entry.key)
			ub = mid;
		else
			lb = mid + 1;
	}

	ASSERT(ub == lb);

	//book�𔭌������̂�seek�̂�����low�̂����ɂ���
	seekg(lb * sizeof(AperyBookEntry), std::ios_base::beg);

}



Move AperyBook::probe(const Position & pos, const std::string & filename, const bool isPickBest)
{
	AperyBookEntry entry;
	uint16_t best = 0;
	uint16_t sum = 0;
	Move m = MOVE_NONE;
	const Key key = RetPosBookKey(pos);
	const int minBookScore = -114514;
	Value value = Value_Zero;

	//�����������Ƃ��N�����Ă���΂�����return ����
	if (filename_ != filename&& openbook(filename.c_str()) == false) {
		return MOVE_NONE;
	}

	//2���T������key�����W���̐擪book�v�f��T��
	binary_search(key); //�S�R�Ⴄkey��entry�̂����ɗ��Ă��܂�...

	// ���݂̋ǖʂɂ������Վ�̐��������[�v����B
	//read������seek�͐i��ł�Ǝv���
	while (read(reinterpret_cast<char*>(&entry), sizeof(entry)), entry.key == key && good()) {

		best = std::max(best, entry.count);//�����ꂽ�񐔂������ق���best
		sum += entry.count;

		if (minBookScore <= entry.score//���Ȃ��Ƃ��Œ�score�������Ă��邱��

										 /*
										 �����_���ɑI�ԏꍇ�͗�����sum�Ŋ������Ƃ��̂��܂肪count�ȉ��ɂȂ�΁icount�̑傫�������w����₷���Ȃ�̂Ń����_���Ƃ͌���������e�͈͂Ȃ̂��낤��...�j
										 best�Ȃ���(�o���񐔂ɂ�����)��Ԃ��ꍇ��  bestcount==entry.count�ƂȂ���̂�I�ԁB�i�����while���[�v����邤���ɍX�V����Ă����j
										 */
			&& ((random_() % sum < entry.count)||(isPickBest && entry.count == best))
		)
		{

			//move���̔����o��
			const int move = entry.ToFromPromote;
			const Square to = (Square)(move & 0x7f);
			ASSERT(is_ok(to));
			const Square from = (Square)((move >> 7) & 0x7f);
			const bool ispromote = (bool)(move && (1 << 14));
			const bool isdrop = (bool)(from >= 81);


			if (isdrop) {
				//��ł�
				const Piece pt = (Piece)(from - SQ_NUM + 1);
				const Piece pc = add_color(pt, pos.sidetomove());
				ASSERT(PAWN <= pt&&pt < KING);
				m = make_drop(to, pc);
			}
			else {
				//�R�}�ړ�
				ASSERT(is_ok(from));
				const Piece pc = pos.piece_on(from);
				ASSERT(pc != NO_PIECE);
				//const Piece captured = pos.piece_on(to);
				if (ispromote) {
					m = make_move(from, to, pc);
				}
				else {
					m = make_movepromote(from, to, pc);
				}
			}

			value = entry.score;
		}
	}
	//cerr << m << " value:" << value << endl;
	return m;
}


AperyBook ABook;