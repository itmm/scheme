/**
 * convert a text stream to a Scheme expression
 */

static int ch { ' ' };

Obj *read_expression(std::istream &in);

void eat_space(std::istream &in) {
	while (ch != EOF && (ch <= ' ' || ch == ';')) {
		if (ch == ';') {
			while (ch != EOF && ch != '\n') { ch = in.get(); }
		} else {
			ch = in.get();
		}
	}
}

Obj *read_list(std::istream &in, int closing) {
	eat_space(in);
	if (ch == EOF) {
		return err("read_list", "incomplete_list");
	}
	if (ch == closing) {
		ch = in.get();
		return nullptr;
	}
	if (ch == ')' || ch == ']') {
		return err("read_list", "unmatched closing");
	}

	auto exp { read_expression(in) };
	auto sym { dynamic_cast<Symbol *>(exp) };
	if (sym && sym->value() == ".") {
		auto result { read_expression(in) };
		eat_space(in);
		ASSERT(ch == closing, "read_list");
		ch = in.get();
		return result;
	}
	return new Pair { exp, read_list(in, closing) };
}

double float_value(const std::string &v) {
	return std::stod(v);
}

#include <sstream>

Obj *read_expression(std::istream &in) {
	eat_space(in);
	if (ch == EOF) { return nullptr; }
	if (ch == '(') { ch = in.get(); return read_list(in, ')'); }
	if (ch == '[') { ch = in.get(); return read_list(in, ']'); }
	if (ch == '\'') {
		ch = in.get();
		return new Pair {
			Symbol::get("quote"),
			new Pair { read_expression(in), nullptr }
		};
	}
	if (ch == '"') {
		std::ostringstream result;
		ch = in.get();
		while (ch != EOF && ch != '"') {
			result << static_cast<char>(ch);
			ch = in.get();
		}
		ASSERT(ch == '"', "read");
		ch = in.get();
		return new String { result.str() };
	}

	if (ch == '#') {
		std::ostringstream result;
		for (;;) {
			if (ch == EOF || ch <= ' ' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '"') { break; }
			result << static_cast<char>(ch);
			ch = in.get();
		}
		auto val { result.str() };
		if (val == "#f" || val == "#F") {
			return false_obj;
		} else if (val == "#t" || val == "#T") {
			return true_obj;
		} else {
			return err("parser", "unknown special", Symbol::get(val));
		}
	}

	std::ostringstream result;
	bool numeric { true };
	bool digits { false };
	bool dots { false };
	bool first { true };
	for (;;) {
		if (first && (ch == '+' || ch == '-')) {
		} else if (ch >= '0' && ch <= '9') {
			digits = true;
		} else if (ch == '.') {
			if (dots || ! digits) {
				numeric = false;
			} else {
				digits = false;
				dots = true;
			}
		} else {
			numeric = false;
		}
		first = false;
		result << static_cast<char>(ch);
		ch = in.get();
		if (ch == EOF || ch <= ' ' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '"') { break; }
	}
	if (numeric && digits && ! dots) { return Integer::create(result.str()); }
	if (numeric && digits && dots) {
		return new Float { float_value(result.str()) };
	}
	return Symbol::get(result.str());
}
