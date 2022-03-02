#include <iostream>

class Element {
		Element *next_;
		bool mark_;

		static Element *all_elements;
		static bool current_mark;

		void mark() {
			if (mark_ != current_mark) {
				mark_ = current_mark;
				propagate_mark();
			}
		}

	protected:
		virtual void propagate_mark() { }
			
		void mark(Element *elm) { if (elm) { elm->mark(); } }

	public:
		Element(): next_ { all_elements }, mark_ { current_mark } {
			all_elements = this;
		}
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) const = 0;
		static std::pair<unsigned, unsigned> garbage_collect();
};

inline std::ostream &operator<<(std::ostream &out, Element *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}

#include <string>

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
	std::cerr << er << '\n';
	return er;
}

#define ASSERT(CND, FN) if (! (CND)) { return err((FN), "no " #CND); }

bool is_err(Element *element) {
	return dynamic_cast<Error *>(element);
}

bool is_good(Element *element) { return ! is_err(element); }

template<typename VALUE_TYPE>
class Value_Element : public Element {
		VALUE_TYPE value_;
	public:
		Value_Element(VALUE_TYPE value): value_ { value } { }
		VALUE_TYPE value() const { return value_; }
		std::ostream &write(std::ostream &out) const override {
			return out << value_;
		}
};

using Symbol = Value_Element<std::string>;
using Float = Value_Element<double>;

#include <vector>

class Integer : public Element {
	public:
		using Digits = std::vector<char>;
	private:
		Digits digits_;
		void normalize() {
			while (! digits_.empty() && digits_.back() == 0) {
				digits_.pop_back();
			}
		}
	public:
		Integer(unsigned value) {
			while (value) {
				digits_.push_back(value % 10);
				value /= 10;
			}
		}
		Integer(const Digits &digits): digits_ { digits } {
			normalize();
		}
		Integer(Digits &&digits): digits_ { std::move(digits) } {
			normalize();
		}
		const Digits &digits() const { return digits_; }
		double float_value() const { 
			double result { 0.0 };
			for (const auto &c: digits_) {
				result = result * 10.0 + c;
			}
			return result;
		}
		bool is_zero() const { return digits_.empty(); }
		std::ostream &write(std::ostream &out) const override {
			for (auto i { digits_.rbegin() }; i != digits_.rend(); ++i) {
				out << static_cast<char>('0' + *i);
			}
			return out;
		}
		virtual bool negative() const { return false; }
};

class Negative_Integer : public Integer {
	public:
		Negative_Integer(unsigned value): Integer { value } { }
		Negative_Integer(const Digits &digits): Integer { digits } { }
		Negative_Integer(Digits &&digits): Integer { std::move(digits) } { }
		virtual bool negative() const { return true; }
		std::ostream &write(std::ostream &out) const override {
			return Integer::write(out << "(- ") << ')';
		}
};

Element *operator-(const Integer &a);
Element *operator-(const Integer &a, const Integer &b);

Element *operator+(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (! a_neg && b_neg) {
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_b, "int+");
		return a - *neg_b;
	} else if (a_neg && b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_a && neg_b, "int+");
		auto neg_sum { dynamic_cast<Integer *>(*neg_a + *neg_b) };
		ASSERT(neg_sum, "int+");
		return -*neg_sum;
	} else if (a_neg && ! b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		ASSERT(neg_a, "int+");
		auto neg_sum { dynamic_cast<Integer *>(*neg_a - b) };
		ASSERT(neg_sum, "int+");
		return -*neg_sum;
	}

	Integer::Digits digits;
	auto a_i { a.digits().begin() };
	auto b_i { b.digits().begin() };
	int carry { 0 };
	for (;;) {
		if (a_i == a.digits().end() && b_i == b.digits().end() && ! carry) { break; }
		int v { carry };
		if (a_i != a.digits().end()) { v += *a_i++; }
		if (b_i != b.digits().end()) { v += *b_i++; }
		if (v >= 10) { v -= 10; carry = 1; } else { carry = 0; }
		digits.push_back(v);
	}

	return new Integer { std::move(digits) };
}

Element *operator-(const Integer &a) {
	if (a.negative()) {
		return new Integer { a.digits() };
	} else {
		return new Negative_Integer(a.digits());
	}
}

Element *operator*(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_a && neg_b, "int*");
		return *neg_a * *neg_b;
	} else if (a_neg && ! b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		ASSERT(neg_a, "int*");
		auto prod { dynamic_cast<Integer *>(*neg_a * b) };
		ASSERT(prod, "int*");
		return -*prod;
	} else if (! a_neg && b_neg) {
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_b, "int*");
		auto prod { dynamic_cast<Integer *>(a * *neg_b) };
		ASSERT(prod, "int*");
		return -*prod;
	}
	Integer::Digits digits;
	auto a_i { a.digits().begin() };
	int offset { 0 };
	for (; a_i != a.digits().end(); ++a_i, ++offset) {
		auto b_i { b.digits().begin() };
		int mult { *a_i };
		int carry { 0 };
		int i { 0 };
		while (b_i != b.digits().end() || carry) {
			int idx { offset + i++ };
			while (idx >= digits.size()) { digits.push_back(0); }
			int value { carry + digits[idx] };
			if (b_i != b.digits().end()) { value += (*b_i++) * mult; }
			carry = value / 10;
			digits[idx] = value % 10;
		}
	}

	return new Integer { std::move(digits) };
}

bool operator<(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_a && neg_b, "<");
		return *neg_b < *neg_a;
	} else if (a_neg && ! b_neg) {
		return ! a.is_zero() || ! b.is_zero();
	} else if (! a_neg && b_neg) {
		return false;
	}
	if (a.digits().size() > b.digits().size()) { return false; }
	if (a.digits().size() < b.digits().size()) { return true; }

	auto a_i { a.digits().rbegin() };
	auto b_i { b.digits().rbegin() };
	for (; a_i != a.digits().rend(); ++a_i, ++b_i) {
		if (*a_i < *b_i) { return true; }
		if (*a_i > *b_i) { return false; }
	}
	return false;
}

static Integer *half(const Integer &num) {
	Integer::Digits result;
	result.resize(num.digits().size());
	int carry { 0 };
	for (int i { static_cast<int>(num.digits().size()) - 1}; i >= 0; --i) {
		int v { carry + num.digits()[i] };
		if (v % 2) { carry = 10; v -= 1; } else { carry = 0; }
		result[i] = v/2;
	}
	return new Integer { std::move(result) };
}

Integer One { 1 };
Integer Zero { 0 };

Element *operator/(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_a && neg_b, "int/");
		return *neg_a / *neg_b;
	} else if (a_neg && ! b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		ASSERT(neg_a, "int/");
		auto neg_div { dynamic_cast<Integer *>(*neg_a / b) };
		ASSERT(neg_div, "int/");
		return -*neg_div;
	} else if (! a_neg && b_neg) {
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_b, "int/");
		auto neg_div { dynamic_cast<Integer *>(a / *neg_b) };
		ASSERT(neg_div, "int/");
		return -*neg_div;
	}
	ASSERT(! b.is_zero(), "int/");
	if (a < b) { return &Zero; }
	Integer *min { &One };
	auto max { dynamic_cast<Integer *>(b * b) };
	ASSERT(max, "int/");
	Integer *two { new Integer { 2 } };

	for (;;) {
		auto prod { dynamic_cast<Integer *>(*max * b) };
		ASSERT(prod, "int/");
		if (! (*prod < a)) { break; }
		max = dynamic_cast<Integer *>(*max * *max);
		ASSERT(max, "int/");
	}

	for (;;) {
		auto diff { dynamic_cast<Integer *>(*max - *min) };
		ASSERT(diff, "int/");
		if (! (One < *diff)) { break; }
		auto sum { dynamic_cast<Integer *>(*max + *min) };
		ASSERT(sum, "int/");
		auto mid { half(*sum) };
		ASSERT(mid, "int/");
		auto prod { dynamic_cast<Integer *>(*mid * b) };
		ASSERT(prod, "int/");
		if (*prod < a) { min = mid; }
		else if (a < *prod) { max = mid; }
		else { return mid; }
	}
	return min;
}

Element *operator-(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (! a_neg && b_neg) {
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_b, "int-");
		return a + *neg_b;
	} else if (a_neg && b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		auto neg_b { dynamic_cast<Integer *>(-b) };
		ASSERT(neg_a && neg_b, "int-");
		auto sum { dynamic_cast<Integer *>(*neg_a - *neg_b) };
		ASSERT(sum, "int-");
		return -*sum;
	} else if (a_neg && ! b_neg) {
		auto neg_a { dynamic_cast<Integer *>(-a) };
		ASSERT(neg_a, "int-");
		auto sum { dynamic_cast<Integer*>(*neg_a + b) };
		ASSERT(sum, "int-");
		return -*sum;
	}
	if (a < b) {
		auto diff { dynamic_cast<Integer *>(b - a) };
		ASSERT(diff, "int-");
		return new Negative_Integer { diff->digits() };
	}

	Integer::Digits digits;
	auto a_i { a.digits().begin() };
	auto b_i { b.digits().begin() };
	int carry { 0 };
	for (;;) {
		if (a_i == a.digits().end()) { break; }
		int v = { *a_i++ + carry };
		if (b_i != b.digits().end()) { v -= *b_i++; }
		if (v < 0) { v += 10; carry = -1; } else { carry = 0; }
		digits.push_back(v);
	}

	return new Integer { std::move(digits) };
}

bool operator==(const Integer &a, const Integer &b) {
	if (a.negative() != b.negative()) {
		return a.is_zero() && b.is_zero();
	}
	if (a.digits().size() != b.digits().size()) {
		return false;
	}
	auto a_i { a.digits().begin() };
	auto b_i { b.digits().begin() };
	for (; a_i != a.digits().end(); ++a_i, ++b_i) {
		if (*a_i != *b_i) { return false; }
	}
	return true;
}

#define True One

Element *to_bool(bool cond) {
	return cond ?  &True : nullptr;
}

bool is_true(Element *value) {
	if (is_err(value)) { err("is_true", "no value"); }
	return is_good(value) && value;
}

bool is_false(Element *value) {
	if (is_err(value)) { err("is_false", "no value"); }
	return ! value;
}

class Pair : public Element {
		Element *head_;
		Element *rest_;
	protected:
		void propagate_mark() override {
			mark(head_); mark(rest_);
		}
	public:
		Pair(Element *head, Element *rest): head_ { head }, rest_ { rest } { }
		Element *head() const { return head_; }
		Element *rest() const { return rest_; }
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Pair::write(std::ostream &out) const {
	auto sym { dynamic_cast<Symbol *>(head_) };
	if (sym && sym->value() == "quote") {
		out << "'";
		return rest_->write(out);
	}

	out << '(';
	bool first { true };
	const Pair *cur { this };
	while (cur) {
		if (first) { first = false; } else { out << ' '; }
		out << cur->head_;
		auto nxt { dynamic_cast<Pair *>(cur->rest_) };
		if (cur->rest_ && ! nxt) {
			out << " . " << cur->rest_;
		}
		cur = nxt;
	}
	out << ')';
	return out;
}

Element *car(Element *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair || !lst, "car");
	return pair ? pair->head() : nullptr;
}

Element *cdr(Element *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair || !lst, "cdr");
	return pair ? pair->rest() : nullptr;
}

Element *cadr(Element *lst) { return car(cdr(lst)); }

Element *cddr(Element *lst) { return cdr(cdr(lst)); }

Element *caddr(Element *lst) { return car(cddr(lst)); }

Element *cdddr(Element *lst) { return cdr(cddr(lst)); }

Element *cadddr(Element *lst) { return car(cdddr(lst)); }

Element *cddddr(Element *lst) { return cdr(cdddr(lst)); }

bool is_null(Element *element) {
	return ! element;
}

bool is_pair(Element *elm) {
	return dynamic_cast<Pair *>(elm);
}

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

#include <map>

class Frame : public Element {
		Frame *next_;
		std::map<std::string, Element *> elements_;
	protected:
		void propagate_mark() override {
			for (auto &v : elements_) {
				mark(v.second);
			}
			mark(next_);
		}
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Element *value);
		bool has(const std::string &key) const;
		Element *get(const std::string &key) const;
		std::ostream &write(std::ostream &out) const override {
			return out << "#frame";
		}
};

void Frame::insert(const std::string &key, Element *value) {
	elements_[key] = value;
}

bool Frame::has(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? true :
		next_ ? next_->has(key) : false;
}

Element *Frame::get(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? it->second :
		next_ ? next_->get(key) : nullptr;
}

class Primitive : public Element {
	public:
		virtual Element *apply(Element *args) = 0;
		std::ostream &write(std::ostream &out) const override;

};

std::ostream &Primitive::write(std::ostream &out) const {
	return out << "#primitive";
}

class Procedure : public Element {
		Element *args_;
		Element *body_;
		Frame *env_;
	protected:
		void propagate_mark() override {
			mark(args_); mark(body_); mark(env_);
		}
	public:
		Procedure(Element *args, Element *body, Frame *env):
			args_ { args }, body_ { body }, env_ { env }
		{ }

		Element *apply(Element *arg_values);
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Procedure::write(std::ostream &out) const {
	out << "(lambda " << args_;
	for (Element *cur { body_ }; cur; cur = cdr(cur)) {
		auto v { car(cur) };
		out << ' ' << v;
	}
	out << ')';
	return out;
}

Symbol *assert_sym(Element *elm) {
	auto sym { dynamic_cast<Symbol *>(elm) };
	if (! sym) { err("assert_sym", "no symbol", elm); }
	return sym;
}

Element *eval(Element *exp, Frame *env);

std::vector<Frame *> active_frames;

class Frame_Guard {
	public:
		Frame_Guard(Frame *frame) { active_frames.push_back(frame); }
		~Frame_Guard() { active_frames.pop_back(); }
};

Element *Procedure::apply(Element *arg_values) {
	auto new_env { new Frame { env_ } };
	Frame_Guard fg { new_env };

	Element *cur { args_ };
	ASSERT(is_good(cur), "procedure");
	for (; is_pair(cur); cur = cdr(cur)) {
		auto sym { assert_sym(car(cur)) };
		ASSERT(sym, "procedure");
		auto value { car(arg_values) };
		ASSERT(is_good(value), "procedure");
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	ASSERT(is_good(cur), "procedure");
	if (cur) {
		auto sym { assert_sym(cur) };
		ASSERT(sym, "procedure");
		ASSERT(is_good(arg_values), "procedure");
		new_env->insert(sym->value(), arg_values);
	}

	cur = body_;
	Element *value { nullptr };
	for (; is_pair(cur); cur = cdr(cur)) {
		Element *statement { car(cur) };
		ASSERT(is_good(statement), "procedure");
		value = eval(statement, new_env);
	}
	ASSERT(is_null(cur), "procedure");
	return value;
}

Element *apply(Element *op, Element *operands) {
	auto prim { dynamic_cast<Primitive *>(op) };
	if (prim) { return prim->apply(operands); }
	auto proc { dynamic_cast<Procedure *>(op) };
	if (proc) { return proc->apply(operands); }
	return err("apply", "unknown operation", op);
}

Element *eval_list(Element *exp, Frame *env) {
	if (is_err(exp) || ! exp) { return exp; }
	auto head { eval(car(exp), env) };
	auto rest { dynamic_cast<Pair *>(cdr(exp)) };
	if (rest) {
		return new Pair { head, eval_list(rest, env) };
	} else {
		return new Pair { head, eval(cdr(exp), env) };
	}
}

bool is_tagged_list(Pair *lst, const std::string &tag) {
	auto sym { dynamic_cast<Symbol *>(car(lst)) };
	return sym && sym->value() == tag;
}

inline bool is_define_special(Pair *lst) {
	return is_tagged_list(lst, "define");
}

inline Element *define_key(Pair *lst) {
	auto first { cadr(lst) };
	auto sym { dynamic_cast<Symbol *>(first) };
	if (sym) { return sym; }
	auto args { dynamic_cast<Pair *>(first) };
	if (args) {
		auto name { dynamic_cast<Symbol *>(car(args)) };
		ASSERT(name, "define_key");
		return name;
	}
	ASSERT(false, "define key");
}

inline Element *define_value(Pair *lst, Frame *env) {
	auto args { dynamic_cast<Pair *>(cadr(lst)) };
	if (args) {
		return new Procedure { cdr(args), cddr(lst), env };
	} else {
		ASSERT(is_null(cdddr(lst)), "define");
		return eval(caddr(lst), env);
	}
}

inline bool is_lambda_special(Pair *lst) {
	return is_tagged_list(lst, "lambda");
}

inline Element *lambda_args(Pair *lst) {
	return cadr(lst);
}

inline Element *lambda_body(Pair *lst) {
	return cddr(lst);
}

inline bool is_if_special(Pair *lst) {
	return is_tagged_list(lst, "if");
}

inline Element *if_condition(Pair *lst) {
	return cadr(lst);
}

inline Element *if_consequence(Pair *lst) {
	return caddr(lst);
}

inline Element *if_alternative(Pair *lst) {
	return cadddr(lst);
}

inline bool is_cond_special(Pair *lst) {
	return is_tagged_list(lst, "cond");
}

Element *build_cond(Element *lst) {
	if (is_err(lst) || is_null(lst)) { return lst; }
	auto expr { car(lst) };
	auto cond { car(expr) };
	auto cons { cdr(expr) };
	ASSERT(is_good(cond) && is_good(cons), "cond");
	auto sym { dynamic_cast<Symbol *>(cond) };
	if (sym && sym->value() == "else") {
		if (cdr(lst)) {
			return err("cond", "else not in last case");
		}
		return new Pair { new Symbol { "begin" }, cons };
	}
	return new Pair {
		new Symbol { "if" },
		new Pair {
			cond,
			new Pair {
				new Pair { new Symbol { "begin" }, cons },
				new Pair { build_cond(cdr(lst)), nullptr }
			}
		}
	};
}

inline bool is_begin_special(Pair *lst) {
	return is_tagged_list(lst, "begin");
}

inline bool is_and_special(Pair *lst) {
	return is_tagged_list(lst, "and");
}

inline bool is_or_special(Pair *lst) {
	return is_tagged_list(lst, "or");
}

inline bool is_quote_special(Pair *lst) {
	return is_tagged_list(lst, "quote");
}

Element *eval(Element *exp, Frame *env) {
	if (is_err(exp) || ! exp) { return exp; }
	auto int_value { dynamic_cast<Integer *>(exp) };
	if (int_value) { return int_value; }
	auto float_value { dynamic_cast<Float *>(exp) };
	if (float_value) { return float_value; }
	auto sym_value { dynamic_cast<Symbol *>(exp) };
	if (sym_value) {
		return env->has(sym_value->value()) ? env->get(sym_value->value()) : exp;
	}
	auto lst_value { dynamic_cast<Pair *>(exp) };
	if (lst_value) {
		if (is_define_special(lst_value)) {
			auto key { dynamic_cast<Symbol *>(define_key(lst_value)) };
			auto value { define_value(lst_value, env) };
			ASSERT(key && is_good(value), "define");
			env->insert(key->value(), value);
			return value;
		}
		if (is_lambda_special(lst_value)) {
			auto args { lambda_args(lst_value) };
			auto body { lambda_body(lst_value) };
			ASSERT(is_good(args) && is_good(body), "lambda");
			return new Procedure(args, body, env);
		}
		if (is_if_special(lst_value)) {
			auto condition { eval(if_condition(lst_value), env) };
			ASSERT(is_good(condition), "if");
			ASSERT(is_null(cddddr(lst_value)), "if");
			if (is_true(condition)) {
				return eval(if_consequence(lst_value), env);
			} else {
				return eval(if_alternative(lst_value), env);
			}
		}
		if (is_cond_special(lst_value)) {
			auto expr { build_cond(cdr(lst_value)) };
			ASSERT(is_good(expr), "cond");
			return eval(expr, env);
		}
		if (is_begin_special(lst_value)) {
			Element *result { nullptr };
			auto cur { cdr(lst_value) };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "begin");
			}
			return result;
		}
		if (is_and_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { &True };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "and");
				if (is_false(result)) { break; }
			}
			return result;
		}
		if (is_or_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { nullptr };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "or");
				if (is_true(result)) { break; }
			}
			return result;
		}
		if (is_quote_special(lst_value)) {
			return cdr(lst_value);
		}
		auto lst { eval_list(lst_value, env) };
		return apply(car(lst), cdr(lst));
	}
	ASSERT(false, "eval");
}

class One_Primitive : public Primitive {
	protected:
		virtual Element *apply_one(Element *arg) = 0;
	public:
		Element *apply(Element *args) override {
			ASSERT(is_pair(args), "one primitive");
			ASSERT(is_null(cdr(args)), "one primitive");
			return apply_one(car(args));
		}
};

class Car_Primitive : public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override { return car(arg); }
};


class Cdr_Primitive : public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override { return cdr(arg); }
};

class List_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *List_Primitive::apply(Element *args) {
	return args;
}

class Two_Primitive : public Primitive {
	protected:
		virtual Element *apply_two(Element *first, Element *second) = 0;
	public:
		Element *apply(Element *args) override {
			ASSERT(is_pair(args), "two primitive");
			auto nxt { cdr(args) };
			ASSERT(is_pair(nxt), "two primitive");
			ASSERT(is_null(cdr(nxt)), "two primitive");
			return apply_two(car(args), car(nxt));
		}
};

class Cons_Primitive : public Two_Primitive {
	protected:
		Element *apply_two(Element *first, Element *second) override {
			return new Pair { first, second };
		}
};

class Null_Primitive: public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override {
			return to_bool(is_null(arg));
		}
};

class Apply_Primitive: public Two_Primitive {
	protected:
		Element *apply_two(Element *first, Element *second) override {
			return ::apply(first, second);
		}
};

class Numeric_Primitive : public Two_Primitive {
	protected:
		Element *apply_two(Element *a, Element *b) override;
		virtual Element *do_int(const Integer &a, const Integer &b) = 0;
		virtual Element *do_float(double a, double b) = 0;
};

Element *Numeric_Primitive::apply_two(Element *a, Element *b) {
	auto a_i { dynamic_cast<Integer *>(a) };
	auto b_i { dynamic_cast<Integer *>(b) };
	if (a_i && b_i) { return do_int(*a_i, *b_i); }
	Float *a_f { nullptr }; Float *b_f { nullptr };
	if (! a_i) { a_f = dynamic_cast<Float *>(a); }
	if (! b_i) { b_f = dynamic_cast<Float *>(b); }
	if ((a_i || a_f) && (b_i || b_f)) {
		return do_float(
			a_f ? a_f->value() : a_i->float_value(),
			b_f ? b_f->value() : b_i->float_value()
		);
	}
	return err("numeric", "invalid arguments", new Pair { a, b });
}

class Add_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a + b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a + b };
		}
};

class Sub_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a - b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a - b };
		}
};

class Mul_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a * b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a * b };
		}
};

class Div_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a / b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a / b };
		}
};

class Less_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return to_bool(a < b);
		}
		Element *do_float(double a, double b) override {
			return to_bool(a < b);
		}
};

class Equal_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return to_bool(a == b);
		}
		Element *do_float(double a, double b) override {
			return to_bool(a == b);
		}
};

Frame initial_frame { nullptr };

Element *Element::all_elements { nullptr };
bool Element::current_mark { true };

std::pair<unsigned, unsigned> Element::garbage_collect() {
	current_mark = ! current_mark;
	for (auto &f : active_frames) { f->mark(); }

	unsigned kept { 0 };
	unsigned collected { 0 };
	Element *prev { nullptr };
	Element *cur { all_elements };
	while (cur) {
		if (cur != &One && cur != &Zero && cur->mark_ != current_mark) {
			++collected;
			auto tmp { cur->next_ };
			delete cur;
			cur = tmp;
			if (prev) {
				prev->next_ = cur;	
			} else { all_elements = cur; }
		} else {
			++kept;
			prev = cur;
			cur = cur->next_;
		}
	}
	return { collected, kept };
}

class Garbage_Collect_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override {
			auto result { Element::garbage_collect() };
			return new Pair {
				new Symbol { "collected" },
				new Pair {
					new Integer { result.first },
					new Pair {
						new Symbol { "kept" },
						new Pair {
							new Integer { result.second },
							nullptr
						}
					}
				}
			};
		}
};

void process_stream(std::istream &in, std::ostream *out, bool prompt) {
	active_frames.clear();
	active_frames.push_back(&initial_frame);

	if (prompt && out) { *out << "? "; }
	ch = in.get();
	if (ch == '#') {
		while (ch != EOF && ch >= ' ') { ch = in.get(); }
	}
	for (;;) {
		auto exp { read_expression(in) };
		if (! exp) { break; }
		exp = eval(exp, &initial_frame);
		if (out) { *out << exp << "\n"; }
		if (prompt && out) { *out << "? "; }
	}
}

#include <fstream>

int main(int argc, const char *argv[]) {
	initial_frame.insert("car", new Car_Primitive());
	initial_frame.insert("cdr", new Cdr_Primitive());
	initial_frame.insert("list", new List_Primitive());
	initial_frame.insert("cons", new Cons_Primitive());
	initial_frame.insert("#binary+", new Add_Primitive());
	initial_frame.insert("#binary-", new Sub_Primitive());
	initial_frame.insert("#binary*", new Mul_Primitive());
	initial_frame.insert("#binary/", new Div_Primitive());
	initial_frame.insert("<", new Less_Primitive());
	initial_frame.insert("=", new Equal_Primitive());
	initial_frame.insert("null?", new Null_Primitive());
	initial_frame.insert("apply", new Apply_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());

	{
		std::ifstream s { "scheme.scm" };
		process_stream(s, nullptr, false);
	}

	if (argc > 1) {
		for (int i { 1 }; i < argc; ++i) {
			if (argv[i] == std::string { "-" }) {
				process_stream(std::cin, &std::cout, true);
			} else {
				std::ifstream in { argv[i] };
				process_stream(in, &std::cout, false);
			}
		}
	} else {
		process_stream(std::cin, &std::cout, true);
	}
}
