/**
 * convert a text stream to a Scheme expression
 */

static int ch { ' ' };
static bool last_is_hash { false };

int get(std::istream &in) {
	last_is_hash = (ch == '#');
	return ch = in.get();
}

Obj *read_expression(std::istream &in);

void read_block_comment(std::istream &in) {
	int nesting = 0;
	bool last_is_bar { false };
	while (ch != EOF) {
		if (ch == '|') {
			if (last_is_hash) { ++nesting; }
		} else if (ch == '#') {
			if (last_is_bar) {
				if (! --nesting) { return; }
			}
		}
		last_is_bar = (ch == '|');
		get(in);

	}
	err("parser", "unclosed block comment");
}

void eat_space(std::istream &in) {
	for (;;) {
		switch (ch) {
			case EOF:
				 return;
			case ';':
				 if (! last_is_hash) {
				 	while (ch != EOF && ch != '\n') { get(in); }
				 } else {
				 	return;
				 }
				 break;
			case '#':
				 get(in);
				 break;
			case '|':
				 if (last_is_hash) { read_block_comment(in); } else { return; }
			default:
				 if (ch <= ' ') { get(in); } else { return; }
				 break;
		}
	}
}

Obj *read_list(std::istream &in, int closing) {
	eat_space(in);
	if (ch == EOF) {
		err("read_list", "incomplete_list");
	}
	if (ch == closing) {
		get(in);
		return nullptr;
	}
	if (ch == ')' || ch == ']') {
		err("read_list", "unmatched closing");
	}

	auto exp { read_expression(in) };
	auto sym { as_symbol(exp) };
	if (sym && sym->value() == ".") {
		auto result { read_expression(in) };
		eat_space(in);
		ASSERT(ch == closing, "read_list");
		get(in);
		return result;
	}
	return cons(exp, read_list(in, closing));
}

double float_value(const std::string &v) {
	return std::stod(v);
}

#include <sstream>

bool is_limiter(int ch) {
	switch (ch) {
		case EOF:
		case ';':
		case '(':
		case ')':
		case '[':
		case ']':
		case '"':
		case '#':
			return true;
		default:
			return ch <= ' ';
	}
}

std::string read_token(std::istream &in) {
	std::ostringstream result;
	for (;;) {
		if (is_limiter(ch)) { break; }
		result << static_cast<char>(ch);
		get(in);
	}
	return result.str();
}

Obj *create_number(const std::string &value) {
	bool digits { false };
	bool dots { false };
	bool fraction { false };
	bool first { true };
	bool exact_complex { false };
	bool inexact_complex { false };
	bool assert_last { false };
	for (auto ch : value) {
		if (assert_last) {
			return nullptr;
		} else if (first && (ch == '+' || ch == '-')) {
		} else if (! exact_complex && ! inexact_complex && (ch == '+' || ch == '-')) {
			inexact_complex = dots;
			exact_complex = ! dots;
			digits = dots = fraction = false;	
		} else if ((ch == 'i' || ch == 'I') && (exact_complex || inexact_complex)) {
			assert_last = true;
		} else if (ch == 'i' || ch == 'I') {
			inexact_complex = dots;
			exact_complex = ! dots;
			assert_last = true;
		} else if (ch >= '0' && ch <= '9') {
			digits = true;
		} else if (ch == '/') {
			if (digits && ! dots && ! fraction) {
				digits = false;
				fraction = true;
			} else { return nullptr; }
		} else if (ch == '.') {
			if (digits && ! dots && ! fraction) {
				digits = false;
				dots = true;
			} else { return nullptr; }
		} else { return nullptr; }
		first = false;
	}

	if (exact_complex) {
		if (digits && ! dots) {
			return Exact_Complex::create(value);
		}
	} else if (inexact_complex) {
		if (digits && ! fraction) {
			return Inexact_Complex::create(value);
		}
	} else {
		if (digits && fraction && ! dots) { return Fraction::create(value); }
		if (digits && ! dots) { return Integer::create(value); }
		if (digits && dots) {
			return new Float { float_value(value) };
		}
	}
	return nullptr;
}

int skip_expressions { 0 };

Obj *parse_expression(std::istream &in) {
	eat_space(in);
	if (ch == EOF) { return nullptr; }
	if (ch == '(') { get(in); return read_list(in, ')'); }
	if (ch == '[') { get(in); return read_list(in, ']'); }
	if (ch == ')' || ch == ']') { get(in); return nullptr; }
	if (ch == '\'') {
		ch = get(in);
		return build_list(Symbol::get("quote"), read_expression(in));
	}
	if (ch == '"') {
		std::ostringstream result;
		get(in);
		while (ch != EOF && ch != '"') {
			result << static_cast<char>(ch);
			get(in);
		}
		ASSERT(ch == '"', "read");
		get(in);
		return new String { result.str() };
	}

	if (last_is_hash) {
		if (ch == ';') {
			get(in);
			skip_expressions += 2;
			return false_obj;
		} else {
			auto val { read_token(in) };
			if (val == "f" || val == "F") {
				return false_obj;
			} else if (val == "t" || val == "T") {
				return true_obj;
			} else {
				err("parser", "unknown special", Symbol::get(val));
			}
		}
	}

	auto tok { read_token(in) };
	auto result { create_number(tok) };
	return result ?: Symbol::get(tok);
}

Obj *parse_expression(const std::string &in) {
	std::istringstream iss { in };
	get(iss);
	return parse_expression(iss);
}

Obj *read_expression(std::istream &in) {
	Obj *result;
	for (;;) {
		result = parse_expression(in);
		if (skip_expressions) {
			--skip_expressions;
		} else { break; };
	}
	return result;
}
