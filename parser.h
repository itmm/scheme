static int ch { ' ' };

Element *read_expression(std::istream &in);

void eat_space(std::istream &in) {
	while (ch != EOF && ch <= ' ') { ch = in.get(); }
}

Element *read_list(std::istream &in) {
	eat_space(in);
	if (ch == EOF) {
		return err("read_list", "incomplete_list");
	}
	if (ch == ')') {
		ch = in.get();
		return nullptr;
	}
	auto exp { read_expression(in) };
	auto sym { dynamic_cast<Symbol *>(exp) };
	if (sym && sym->value() == ".") {
		auto result { read_expression(in) };
		eat_space(in);
		ASSERT(ch == ')', "read_list");
		ch = in.get();
		return result;
	}
	return new Pair { exp, read_list(in) };
}

bool is_float(const std::string &v) {
	auto i = v.begin();
	if (i == v.end()) { return false; }
	if (*i == '+' || *i == '-') { ++i; }
	bool digits { false };
	bool dot { false };
	for (; i != v.end(); ++i) {
		if (*i >= '0' && *i <= '9') {
			digits = true;
		} else if (*i == '.') {
			if (dot) { return false; }
			if (! digits) { return false; }
			digits = false;
			dot = true;
		} else { 
			return false;
		}
	}
	return dot && digits;
}

double float_value(const std::string &v) {
	return std::stod(v);
}

#include <sstream>

Element *read_expression(std::istream &in) {
	eat_space(in);
	if (ch == EOF) { return nullptr; }
	if (ch == '(') { ch = in.get(); return read_list(in); }
	if (ch == '\'') {
		ch = in.get();
		return new Pair {
			new Symbol { "quote" },
			read_expression(in)
		};
	}
	std::ostringstream result;
	bool numeric { true };
	unsigned value { 0 };
	for (;;) {
		if (ch >= '0' && ch <= '9') {
			value = value * 10 + (ch - '0');
		} else {
			numeric = false;
		}
		result << static_cast<char>(ch);
		ch = in.get();
		if (ch == EOF || ch <= ' ' || ch == '(' || ch == ')') { break; }
	}
	if (numeric) { return new Integer { value }; }
	if (is_float(result.str())) {
		return new Float { float_value(result.str()) };
	}
	return new Symbol { result.str() };
}
