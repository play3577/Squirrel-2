#define _CRT_SECURE_NO_WARNINGS

#include "evaluate.h"
#include "usi.h"
#include "position.h"
#ifdef EVAL_KPP

namespace Eval {

	int16_t kpp[82][fe_end][fe_end];
	int32_t kkp[82][82][fe_end + 1];




	void read_KPP() {
		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "rb");
		if (fp == NULL) { ASSERT(0); }
		std::fread(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "rb");
		if (fp == NULL) { ASSERT(0); }
		std::fread(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);


	}

	void write_KPP() {

		FILE* fp = std::fopen(Options["KPP"].str().c_str(), "wb");
		std::fwrite(&kpp, sizeof(kpp), 1, fp);
		std::fclose(fp);

		fp = std::fopen(Options["KKP"].str().c_str(), "wb");
		std::fwrite(&kkp, sizeof(kkp), 1, fp);
		std::fclose(fp);

	}

	Value eval_KPP(const Position& pos) {



	}
	Value eval_diff_KPP(const Position& pos) {

	}

	Value Eval::eval(const Position& pos) {

		Value material= pos.state()->material;
		
		Value value = material;

		return (pos.sidetomove() == BLACK) ? value : -value;
	}

}

#endif