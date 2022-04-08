#include "err.h"

std::ostream *err_stream { &std::cerr };

void Error::propagate_mark() {
	mark(data1_);
	mark(data2_);
}

Error::Error(
	const std::string &raiser, const std::string &message,
	Obj *data1, Obj *data2
):
	raiser_ { raiser }, message_ { message },
	data1_ { data1 }, data2_ { data2 }
{ }

std::ostream &Error::write(std::ostream &out) {
	out << "(#error " << raiser_ << ": " << message_;
	if (data1_) {
		out << ": " << data1_;
	}
	if (data2_) {
		out << " " << data2_;
	}
	return out << ')';
}

