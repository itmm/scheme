/**
 * errors are special objects
 */
#include <string>

std::ostream *err_stream { &std::cerr };

class Error : public Obj {
		std::string raiser_;
		std::string message_;
		Obj *data1_;
		Obj *data2_;
	protected:
		void propagate_mark() override {
			mark(data1_);
			mark(data2_);
		}
	public:
		Error(
			const std::string &raiser, const std::string &message,
			Obj *data1, Obj *data2
		):
			raiser_ { raiser }, message_ { message },
			data1_ { data1 }, data2_ { data2 }
		{ }
		std::ostream &write(std::ostream &out) override {
			out << "(#error " << raiser_ << ": " << message_;
			if (data1_) {
				out << ": " << data1_;
			}
			if (data2_) {
				out << " " << data2_;
			}
			return out << ')';
		}
};

Obj *err(const std::string fn, const std::string msg, Obj *exp1 = nullptr, Obj *exp2 = nullptr) {
	auto er { new Error { fn, msg, exp1, exp2 } };
	if (err_stream) { *err_stream << er << '\n'; }
	return er;
}

#define ASSERT(CND, FN) if (! (CND)) { return err((FN), "no " #CND); }

bool is_err(Obj *obj) {
	return dynamic_cast<Error *>(obj);
}

bool is_good(Obj *obj) { return ! is_err(obj); }
