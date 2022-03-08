/**
 * errors are special objects
 */
#include <string>

std::ostream *err_stream { &std::cerr };

class Error : public Element {
		std::string raiser_;
		std::string message_;
		Element *data1_;
		Element *data2_;
	protected:
		void propagate_mark() override {
			mark(data1_);
			mark(data2_);
		}
	public:
		Error(
			const std::string &raiser, const std::string &message,
			Element *data1, Element *data2
		):
			raiser_ { raiser }, message_ { message },
			data1_ { data1 }, data2_ { data2 }
		{ }
		std::ostream &write(std::ostream &out) const override {
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

Element *err(const std::string fn, const std::string msg, Element *exp1 = nullptr, Element *exp2 = nullptr) {
	auto er { new Error { fn, msg, exp1, exp2 } };
	if (err_stream) { *err_stream << er << '\n'; }
	return er;
}

#define ASSERT(CND, FN) if (! (CND)) { return err((FN), "no " #CND); }

bool is_err(Element *element) {
	return dynamic_cast<Error *>(element);
}

bool is_good(Element *element) { return ! is_err(element); }
