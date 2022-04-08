/**
 * errors are special objects
 */

#pragma once

#include "obj.h"

#include <string>

extern std::ostream *err_stream;

class Error : public Obj {
		std::string raiser_;
		std::string message_;
		Obj *data1_;
		Obj *data2_;
	protected:
		void propagate_mark() override;
	public:
		Error(
			const std::string &raiser, const std::string &message,
			Obj *data1, Obj *data2
		);
		std::ostream &write(std::ostream &out) override;
};

inline void err(const std::string fn, const std::string msg, Obj *exp1 = nullptr, Obj *exp2 = nullptr) {
	throw new Error { fn, msg, exp1, exp2 };
}

#define ASSERT(CND, FN) if (! (CND)) { err((FN), "no " #CND); }

