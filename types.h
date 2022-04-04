/**
 * basic types
 * Integers can be as long as memory permits
 * nullptr is treated as the empty pair
 */

template<typename VALUE_TYPE, typename BASE = Obj>
class Value_Element : public BASE {
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

namespace Dynamic {
	template<typename C> C *as(Obj *obj) { return dynamic_cast<C *>(obj); }
	template<typename C> bool is(Obj *obj) { return as<C>(obj); }
}

constexpr auto as_string = Dynamic::as<String>;
constexpr auto is_string = Dynamic::is<String>;

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

constexpr auto as_symbol = Dynamic::as<Symbol>;
constexpr auto is_symbol = Dynamic::is<Symbol>;

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

bool is_false(Obj *value) {
	return value == false_obj;
}

bool is_true(Obj *value) {
	return ! is_false(value);
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

constexpr auto as_pair = Dynamic::as<Pair>;
constexpr auto is_pair = Dynamic::is<Pair>;
inline bool is_null(Obj *element) { return ! element; }

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

Obj *cadr(Obj *obj) { return car(cdr(obj)); }
Obj *cddr(Obj *obj) { return cdr(cdr(obj)); }
Obj *caddr(Obj *obj) { return car(cddr(obj)); }
Obj *cdddr(Obj *obj) { return cdr(cddr(obj)); }
Obj *cadddr(Obj *obj) { return car(cdddr(obj)); }
Obj *cddddr(Obj *obj) { return cdr(cdddr(obj)); }

bool is_complicated(Obj *elm) {
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
		auto nxt_pair { as_pair(nxt) };
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

void write_complex_pair(std::ostream &out, Pair *pair, std::string indent) {
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

