/**
 * basic types
 * Integers can be as long as memory permits
 * nullptr is treated as the empty pair
 */

template<typename VALUE_TYPE>
class Value_Element : public Obj {
		VALUE_TYPE value_;
	public:
		Value_Element(const VALUE_TYPE &value): value_ { value } { }
		const VALUE_TYPE &value() const { return value_; }
		std::ostream &write(std::ostream &out) override {
			return out << value_;
		}
};

class String : public Value_Element<std::string> {
	public:
		String(const std::string &value) : Value_Element(value) { }
		std::ostream &write(std::ostream &out) override {
			return out << '"' << value() << '"';
		}
};

#include <map>

class Symbol : public Obj {
		static std::map<std::string, Symbol *>symbols_;
		std::string value_;
		Symbol(const std::string &value): value_ { value } { }
	public:
		~Symbol() {
			auto it { symbols_.find(value_) };
			if (it != symbols_.end()) { symbols_.erase(it); }
		}

		static Symbol *get(const std::string &value) {
			auto it { symbols_.find(value) };
			if (it != symbols_.end()) { return it->second; }
			auto sym { new Symbol { value } };
			symbols_[value] = sym;
			return sym;
		}
		const std::string &value() const { return value_; }
		std::ostream &write(std::ostream &out) {
			return out << value_;
		}
};

std::map<std::string, Symbol *> Symbol::symbols_;

#include "num.h"

class False : public Obj {
	public:
		False() {}
		std::ostream &write(std::ostream &out) override {
			return out << "#f";
		}
};

class True : public Obj {
	public:
		True() {}
		std::ostream &write(std::ostream &out) override {
			return out << "#t";
		}
};

False *false_obj = new False {};
True *true_obj = new True {};

Obj *to_bool(bool cond) {
	return cond ?  static_cast<Obj *>(true_obj) : false_obj;
}

bool is_true(Obj *value) {
	if (is_err(value)) { err("is_true", "no value"); }
	return is_good(value) && value != false_obj;
}

bool is_false(Obj *value) {
	if (is_err(value)) { err("is_false", "no value"); }
	return value == false_obj;
}

class Pair : public Obj {
		Obj *head_;
		Obj *rest_;
	protected:
		void propagate_mark() override {
			mark(head_); mark(rest_);
		}
	public:
		Pair(Obj *head, Obj *rest): head_ { head }, rest_ { rest } { }
		Obj *head() const { return head_; }
		Obj *rest() const { return rest_; }
		void set_head(Obj *head) { head_ = head; }
		void set_rest(Obj *rest) { rest_ = rest; }
		std::ostream &write(std::ostream &out) override;
};

Obj *car(Obj *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair, "car");
	return pair->head();
}

Obj *cdr(Obj *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair, "cdr");
	return pair->rest();
}

Obj *cadr(Obj *lst) { return car(cdr(lst)); }

Obj *cddr(Obj *lst) { return cdr(cdr(lst)); }

Obj *caddr(Obj *lst) { return car(cddr(lst)); }

Obj *cdddr(Obj *lst) { return cdr(cddr(lst)); }

Obj *cadddr(Obj *lst) { return car(cdddr(lst)); }

Obj *cddddr(Obj *lst) { return cdr(cdddr(lst)); }

bool is_null(Obj *element) {
	return ! element;
}

bool is_pair(Obj *elm) {
	return dynamic_cast<Pair *>(elm);
}

bool is_complex(Obj *elm) {
	int i { 0 };
	for (Obj *cur { elm }; is_pair(cur); cur = cdr(cur), ++i) {
		auto val { car(cur) };
		if (i > 4 || is_null(val) || is_pair(val)) { return true; }
	}
	return false;
}

void write_simple_pair(std::ostream &out, Pair *pair) {
	out << '(';
	bool first { true };
	Pair *cur { pair };
	while (cur) {
		if (first) { first = false; } else { out << ' '; }
		out << car(cur);
		auto nxt { cdr(cur) };
		auto nxt_pair { dynamic_cast<Pair *>(nxt) };
		if (nxt && ! nxt_pair) {
			out << " . " << nxt;
		}
		cur = nxt_pair;
	}
	out << ')';
}

void write_complex_pair(std::ostream &out, Pair *pair, std::string indent);

void write_inner_complex_pair(std::ostream &out, Pair *pair, std::string indent) {
	auto first { car(pair) };
	out << first;
	auto sym { dynamic_cast<Symbol *>(first) };
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
		auto val_pair { dynamic_cast<Pair *>(value) };
		if (! value) {
			out << "()";
		} else if (val_pair) {
			if (is_complex(val_pair)) {
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

void write_complex_pair(std::ostream &out, Pair *pair, std::string indent) {
	out << '('; write_inner_complex_pair(out, pair,indent); out << ')';
}

std::ostream &Pair::write(std::ostream &out) {
	auto sym { dynamic_cast<Symbol *>(head_) };
	if (sym && sym->value() == "quote") {
		out << "'";
		return car(rest_)->write(out);
	}

	if (is_complex(this)) {
		write_complex_pair(out, this, "");
	} else {
		write_simple_pair(out, this);
	}
	return out;
}

