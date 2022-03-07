/**
 * errors are special objects
 */
#include <string>

std::ostream *err_stream { &std::cerr };

class Error : public Element {
		std::string raiser_;
		std::string message_;
		Element *data_;
	protected:
		void propagate_mark() override {
			mark(data_);
		}
	public:
		Error(
			const std::string &raiser, const std::string &message,
			Element *data
		):
			raiser_ { raiser }, message_ { message }, data_ { data }
		{ }
		std::ostream &write(std::ostream &out) const override {
			out << "(#error " << raiser_ << ": " << message_;
			if (data_) {
				out << ": " << data_;
			}
			return out << ')';
		}
};

Element *err(const std::string fn, const std::string msg, Element *exp = nullptr) {
	auto er { new Error { fn, msg, exp } };
	if (err_stream) { *err_stream << er << '\n'; }
	return er;
}

#define ASSERT(CND, FN) if (! (CND)) { return err((FN), "no " #CND); }

bool is_err(Element *element) {
	return dynamic_cast<Error *>(element);
}

bool is_good(Element *element) { return ! is_err(element); }
