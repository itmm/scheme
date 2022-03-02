#include <iostream>
#include <string>
#include <sstream>

class Element;
Element *all_elements { nullptr };
bool current_mark { true };

class Element {
		Element *next_;
		bool mark_;

	protected:
		virtual void propagate_mark() { }
			
	public:
		Element(): next_ { all_elements}, mark_ { current_mark } {
			all_elements = this;
		}
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) const = 0;
		void mark() {
			if (mark_ != current_mark) {
				mark_ = current_mark;
				propagate_mark();
			}
		}
		bool get_mark() const { return mark_; }
		Element *next() { return next_; }
		void set_next(Element *n) { next_ = n; }
};

inline std::ostream &operator<<(std::ostream &out, Element *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "#null";
	}
}

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

Element *err(const std::string fn, const std::string msg, Element *exp = nullptr) {
	std::cerr << fn << ": " << msg;
	if (exp) { std::cerr << " (" << exp << ')'; }
	std::cerr << '\n';
	return nullptr;
}

#define ASSERT(CND, FN) if (! (CND)) { err((FN), "no " #CND); return nullptr; }

#include <vector>

class Integer : public Element {
	public:
		using Digits = std::vector<char>;
	private:
		Digits digits_;
		void push_digits(unsigned value) {
			if (value) {
				digits_.push_back(value % 10);
				push_digits(value / 10);
			}
		}
		void normalize() {
			while (! digits_.empty() && digits_.back() == 0) {
				digits_.pop_back();
			}
		}
	public:
		Integer(unsigned value) {
			push_digits(value);
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
		bool zero() const { return digits_.empty(); }
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

Integer *operator-(const Integer &a);
Integer *operator-(const Integer &a, const Integer &b);

Integer *operator+(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (! a_neg && b_neg) {
		return a - *(-b);
	} else if (a_neg && b_neg) {
		return -*(*(-a) + *(-b));
	} else if (a_neg && ! b_neg) {
		return -*(*(-a) - b);
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

Integer *operator-(const Integer &a) {
	if (a.negative()) {
		return new Integer { a.digits() };
	} else {
		return new Negative_Integer(a.digits());
	}
}

Integer *operator*(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		return *(-a) * *(-b);
	} else if (a_neg && ! b_neg) {
		return -*(*(-a) * b);
	} else if (! a_neg && b_neg) {
		return -*(a * *(-b));
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

class Pair : public Element {
		Element *head_;
		Element *rest_;
	protected:
		void propagate_mark() override {
			if (head_) { head_->mark(); }
			if (rest_) { rest_->mark(); }
		}
	public:
		Pair(Element *head, Element *rest): head_ { head }, rest_ { rest } { }
		Element *head() const { return head_; }
		Element *rest() const { return rest_; }
		std::ostream &write(std::ostream &out) const override;
};

Pair Null { &Null, &Null };
#define False Null
Integer One { 1 };
#define True One
Integer Zero { 0 };

Element *to_bool(bool cond) {
	return cond ?
		dynamic_cast<Element *>(&True) :
		dynamic_cast<Element *>(&False);
}

bool is_true(Element *value) {
	if (! value) { err("is_true", "no value"); }
	return value && value != &Null;
}

bool is_false(Element *value) {
	if (! value) { err("is_false", "no value"); }
	return value == &Null;
}

Element *operator<(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		return *(-b) < *(-a);
	} else if (a_neg && ! b_neg) {
		return to_bool(! a.zero() || ! b.zero());
	} else if (! a_neg && b_neg) {
		return to_bool(false);
	}
	if (a.digits().size() > b.digits().size()) { return to_bool(false); }
	if (a.digits().size() < b.digits().size()) { return to_bool(true); }

	auto a_i { a.digits().rbegin() };
	auto b_i { b.digits().rbegin() };
	for (; a_i != a.digits().rend(); ++a_i, ++b_i) {
		if (*a_i < *b_i) { return to_bool(true); }
		if (*a_i > *b_i) { return to_bool(false); }
	}
	return to_bool(false);
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

Integer *operator/(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (a_neg && b_neg) {
		return *(-a) / *(-b);
	} else if (a_neg && ! b_neg) {
		return -*(*(-a) / b);
	} else if (! a_neg && b_neg) {
		return -*(a / *(-b));
	}
	ASSERT(! b.zero(), "int/");
	if (is_true(a < b)) { return &Zero; }
	Integer *min { &One };
	Integer *max { b * b };
	Integer *two { new Integer { 2 } };
	while (is_true(*(*max * b) < a)) { max = *max * *max; }
	while (is_true(One < *(*max - *min))) {
		Integer *mid { half(*(*max + *min)) };
		Integer *prod { *mid * b };
		if (is_true(*prod < a)) { min = mid; }
		else if (is_true(a < *prod)) { max = mid; }
		else { return mid; }
	}
	return min;
}

Integer *operator-(const Integer &a, const Integer &b) {
	auto a_neg { a.negative() };
	auto b_neg { b.negative() };
	if (! a_neg && b_neg) {
		return a + *(-b);
	} else if (a_neg && b_neg) {
		return -*(*(-a) - *(-b));
	} else if (a_neg && ! b_neg) {
		return -*(*(-a) + b);
	}
	if (is_true(a < b)) {
		auto diff { b - a };
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

Element *operator==(const Integer &a, const Integer &b) {
	if (a.negative() != b.negative()) {
		return to_bool(a.zero() && b.zero());
	}
	if (a.digits().size() != b.digits().size()) {
		return to_bool(false);
	}
	auto a_i { a.digits().begin() };
	auto b_i { b.digits().begin() };
	for (; a_i != a.digits().end(); ++a_i, ++b_i) {
		if (*a_i != *b_i) { return to_bool(false); }
	}
	return to_bool(true);
}

std::ostream &Pair::write(std::ostream &out) const {
	auto sym { dynamic_cast<Symbol *>(head_) };
	if (sym && sym->value() == "quote") {
		out << "'";
		return rest_->write(out);
	}

	out << '(';
	bool first { true };
	const Pair *cur { this };
	while (cur && cur != &Null) {
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

static int ch { ' ' };

Element *read_expression(std::istream &in);

void eat_space(std::istream &in) {
	while (ch != EOF && ch <= ' ') { ch = in.get(); }
}

Element *read_list(std::istream &in) {
	eat_space(in);
	if (ch == EOF) {
		std::cerr << "incomplete list\n";
		return nullptr;
	}
	if (ch == ')') {
		ch = in.get();
		return &Null;
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
				v.second->mark();
			}
			if (next_) { next_->mark(); }
		}
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Element *value);
		Element *get(const std::string &key) const;
		std::ostream &write(std::ostream &out) const override {
			return out << "#frame";
		}
};

void Frame::insert(const std::string &key, Element *value) {
	elements_[key] = value;
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
			if (args_) { args_->mark(); }
			if (body_) { body_->mark(); }
			if (env_) { env_->mark(); }
		}
	public:
		Procedure(Element *args, Element *body, Frame *env):
			args_ { args }, body_ { body }, env_ { env }
		{ }

		Element *apply(Element *arg_values);
		std::ostream &write(std::ostream &out) const override;
};

Element *eval(Element *exp, Frame *env);

Element *car(Element *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair, "car");
	return pair->head();
}

Element *cdr(Element *lst) {
	auto pair { dynamic_cast<Pair *>(lst) };
	ASSERT(pair, "cdr");
	return pair->rest();
}

std::ostream &Procedure::write(std::ostream &out) const {
	out << "(lambda " << args_;
	for (Element *cur { body_ }; cur && cur != &Null; cur = cdr(cur)) {
		auto v { car(cur) };
		out << ' ' << v;
	}
	out << ')';
	return out;
}

bool is_null(Element *element) {
	if (! element) { err("is_null", "no argument"); }
	return element == &Null;
}

Element *Procedure::apply(Element *arg_values) {
	auto new_env { new Frame { env_ } };

	Element *cur { args_ };
	auto cur_pair { dynamic_cast<Pair *>(cur) };
	for (; cur_pair && cur_pair != &Null; cur = cdr(cur),
		cur_pair = dynamic_cast<Pair *>(cur)
	) {
		auto sym { dynamic_cast<Symbol *>(car(cur)) };
		ASSERT(sym, "procedure");
		auto value { car(arg_values) };
		ASSERT(value, "procedure");
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	ASSERT(cur, "procedure");
	if (cur != &Null) {
		auto sym { dynamic_cast<Symbol *>(cur) };
		ASSERT(sym, "procedure");
		ASSERT(arg_values, "procedure");
		new_env->insert(sym->value(), arg_values);
	}

	cur = body_;
	cur_pair = dynamic_cast<Pair *>(cur);
	Element *value = &Null;
	for (; cur_pair && cur_pair != &Null; cur = cdr(cur),
		cur_pair = dynamic_cast<Pair *>(cur)
	) {
		Element *statement { car(cur_pair) };
		ASSERT(statement, "procedure");
		value = eval(statement, new_env);
		ASSERT(value, "procedure");
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

Pair *eval_list(Pair *exp, Frame *env) {
	if (! exp || exp == &Null) { return exp; }
	auto head { eval(exp->head(), env) };
	auto rest { dynamic_cast<Pair *>(exp->rest()) };
	if (rest) {
		return new Pair { head, eval_list(rest, env) };
	} else {
		return new Pair { head, eval(exp->rest(), env) };
	}
}

bool is_tagged_list(Pair *lst, const std::string &tag) {
	auto sym { dynamic_cast<Symbol *>(car(lst)) };
	return sym != nullptr && sym->value() == tag;
}

inline bool is_define_special(Pair *lst) {
	return is_tagged_list(lst, "define");
}

Element *cadr(Element *lst) {
	return car(cdr(lst));
}

inline Symbol *define_key(Pair *lst) {
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

Element *cddr(Element *lst) {
	return cdr(cdr(lst));
}

Element *caddr(Element *lst) {
	return car(cddr(lst));
}

Element *cdddr(Element *lst) {
	return cdr(cddr(lst));
}

Element *cadddr(Element *lst) {
	return car(cdddr(lst));
}

inline Element *define_value(Pair *lst, Frame *env) {
	auto args { dynamic_cast<Pair *>(cadr(lst)) };
	if (args) {
		return new Procedure { cdr(args), cddr(lst), env };
	} else {
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
	if (! lst || lst == &Null) { return lst; }
	auto expr { car(lst) };
	auto cond { car(expr) };
	auto cons { cdr(expr) };
	ASSERT(cond && cons, "cond");
	auto sym { dynamic_cast<Symbol *>(cond) };
	if (sym && sym->value() == "else") {
		if (cdr(lst) != &Null) {
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
				new Pair { build_cond(cdr(lst)), &Null }
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
	if (! exp || exp == &Null) { return exp; }
	auto int_value { dynamic_cast<Integer *>(exp) };
	if (int_value) { return int_value; }
	auto float_value { dynamic_cast<Float *>(exp) };
	if (float_value) { return float_value; }
	auto sym_value { dynamic_cast<Symbol *>(exp) };
	if (sym_value) {
		Element *got { env->get(sym_value->value()) };
		return got ?: exp;
	}
	auto lst_value { dynamic_cast<Pair *>(exp) };
	if (lst_value) {
		if (is_define_special(lst_value)) {
			auto key { define_key(lst_value) };
			auto value { define_value(lst_value, env) };
			ASSERT(key && value, "define");
			env->insert(key->value(), value);
			return value;
		}
		if (is_lambda_special(lst_value)) {
			auto args { lambda_args(lst_value) };
			auto body { lambda_body(lst_value) };
			ASSERT(args && body, "lambda");
			return new Procedure(args, body, env);
		}
		if (is_if_special(lst_value)) {
			auto condition { eval(if_condition(lst_value), env) };
			ASSERT(condition, "if");
			if (is_true(condition)) {
				return eval(if_consequence(lst_value), env);
			} else {
				return eval(if_alternative(lst_value), env);
			}
		}
		if (is_cond_special(lst_value)) {
			auto expr { build_cond(cdr(lst_value)) };
			ASSERT(expr, "cond");
			return eval(expr, env);
		}
		if (is_begin_special(lst_value)) {
			Element *result;
			auto cur { cdr(lst_value) };
			for (; cur && cur != &Null; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(result, "begin");
			}
			return result;
		}
		if (is_and_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { &True };
			for (; cur && cur != &Null; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(result, "and");
				if (is_false(result)) { break; }
			}
			return result;
		}
		if (is_or_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { &False };
			for (; cur && cur != &Null; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(result, "or");
				if (is_true(result)) { break; }
			}
			return result;
		}
		if (is_quote_special(lst_value)) {
			return cdr(lst_value);
		}
		auto lst { eval_list(lst_value, env) };
		return apply(lst->head(), lst->rest());
	}
	ASSERT(false, "eval");
}

class Car_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Car_Primitive::apply(Element *args) {
	ASSERT(is_null(cdr(args)), "car");
	return car(car(args));
}

class Cdr_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Cdr_Primitive::apply(Element *args) {
	ASSERT(is_null(cdr(args)), "cdr");
	return cdr(car(args));
}

class List_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *List_Primitive::apply(Element *args) {
	return args;
}

class Cons_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Cons_Primitive::apply(Element *args) {
	Element *first { car(args) };
	Element *nxt { cdr(args) };
	Element *second { car(nxt) };
	nxt = cdr(nxt);
	ASSERT(first && second && is_null(nxt), "cons");
	return new Pair { first, second };
}

class Null_Primitive: public Primitive {
	public:
		Element *apply(Element *args) override {
			Element *obj { car(args) };
			ASSERT(obj, "null?");
			return to_bool(is_null(obj));
		}
};

class Apply_Primitive: public Primitive {
	public:
		Element *apply(Element *args) override {
			Element *fn { car(args) };
			Element *fn_args { cadr(args) };
			ASSERT(fn && fn_args && is_null(cddr(args)), "apply");
			return ::apply(fn, fn_args);
		}
};

class Numeric_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
	
	protected:
		virtual Element *do_int(const Integer &a, const Integer &b) = 0;
		virtual Element *do_float(double a, double b) = 0;
};

Element *Numeric_Primitive::apply(Element *args) {
	Element *a { car(args) };
	Element *b { cadr(args) };
	ASSERT(a && b && is_null(cddr(args)), "numeric");
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
	ASSERT(false, "numeric");
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
			return a < b;
		}
		Element *do_float(double a, double b) override {
			return to_bool(a < b);
		}
};

class Equal_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a == b;
		}
		Element *do_float(double a, double b) override {
			return to_bool(a == b);
		}
};

Frame initial_frame { nullptr };

class Garbage_Collect_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Garbage_Collect_Primitive::apply(Element *args) {
	current_mark = ! current_mark;
	initial_frame.mark();
	unsigned kept { 0 };
	unsigned collected { 0 };
	Element *prev { nullptr };
	Element *cur { all_elements };
	while (cur) {
		if (cur != &Null && cur != &True && cur != &Zero && cur->get_mark() != current_mark) {
			++collected;
			auto tmp { cur->next() };
			delete cur;
			cur = tmp;
			if (prev) {
				prev->set_next(cur);	
			} else { all_elements = cur; }
		} else {
			++kept;
			prev = cur;
			cur = cur->next();
		}
	}
	return new Pair {
		new Symbol { "collected" },
		new Pair {
			new Integer { collected },
			new Pair {
				new Symbol { "kept" },
				new Pair {
					new Integer { kept },
					&Null
				}
			}
		}
	};
}

void process_stream(std::istream &in, std::ostream *out, bool prompt) {
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
