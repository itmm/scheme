#include "types.h"

Symbol::~Symbol() {
	auto it { symbols_.find(value_) };
	if (it != symbols_.end()) { symbols_.erase(it); }
}

Symbol *Symbol::get(const std::string &value) {
	auto it { symbols_.find(value) };
	if (it != symbols_.end()) { return it->second; }
	auto sym { new Symbol { value } };
	symbols_[value] = sym;
	return sym;
}

std::map<std::string, Symbol *> Symbol::symbols_;

False *false_obj = new False {};
True *true_obj = new True {};

Obj *to_bool(bool cond) {
	return cond ?  static_cast<Obj *>(true_obj) : false_obj;
}

Obj *car(Obj *obj) {
	auto pair { as_pair(obj) };
	ASSERT(pair, "car");
	return pair->head();
}

Obj *cdr(Obj *obj) {
	auto pair { as_pair(obj) };
	ASSERT(pair, "cdr");
	return pair->rest();
}

static bool is_complicated(Obj *elm) {
	int i { 0 };
	for (Obj *cur { elm }; is_pair(cur); cur = cdr(cur), ++i) {
		auto val { car(cur) };
		if (i > 4 || is_null(val) || is_pair(val)) { return true; }
	}
	return false;
}

static void write_simple_pair(std::ostream &out, Pair *pair) {
	out << '(';
	bool first { true };
	Pair *cur { pair };
	while (cur) {
		if (first) { first = false; } else { out << ' '; }
		out << car(cur);
		auto nxt { cdr(cur) };
		auto nxt_pair { as_pair(nxt) };
		if (nxt && ! nxt_pair) {
			out << " . " << nxt;
		}
		cur = nxt_pair;
	}
	out << ')';
}

static void write_complex_pair(std::ostream &out, Pair *pair, std::string indent);

void write_inner_complex_pair(std::ostream &out, Pair *pair, std::string indent) {
	auto first { car(pair) };
	out << first;
	auto sym { as_symbol(first) };
	bool no_newline { false };
	if (sym) {
		for (unsigned i { 0 }; i <= sym->value().length(); ++i) {
			indent += ' ';
		}
		no_newline = true;
		out << ' ';
	}
	indent += ' ';
	Obj *cur { cdr(pair) };
	while (is_pair(cur)) {
		if (no_newline) {
			no_newline = false;
		} else {
			out << '\n' << indent;
		}
		auto value { car(cur) };
		auto val_pair { as_pair(value) };
		if (! value) {
			out << "()";
		} else if (val_pair) {
			if (is_complicated(val_pair)) {
				write_complex_pair(out, val_pair, indent);
			} else {
				write_simple_pair(out, val_pair);
			}
		} else { out << value; }
		cur = cdr(cur);
	}
	if (cur) {
		out << " . " << cur;
	}
}

static void write_complex_pair(std::ostream &out, Pair *pair, std::string indent) {
	out << '('; write_inner_complex_pair(out, pair,indent); out << ')';
}

std::ostream &Pair::write(std::ostream &out) {
	auto sym { as_symbol(head_) };
	if (sym && sym->value() == "quote") {
		out << "'";
		return car(rest_)->write(out);
	}

	if (is_complicated(this)) {
		write_complex_pair(out, this, "");
	} else {
		write_simple_pair(out, this);
	}
	return out;
}

