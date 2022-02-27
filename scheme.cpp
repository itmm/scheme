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
using Integer = Value_Element<int>;
using Float = Value_Element<double>;

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

std::ostream &Pair::write(std::ostream &out) const {
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
	std::ostringstream result;
	bool numeric { true };
	int value { 0 };
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
	auto cur { dynamic_cast<Pair *>(lst) };
	if (! cur) {
		std::cerr << "car: no list: " << lst << "\n";
		return nullptr;
	}
	return cur->head();
}

Element *cdr(Element *lst) {
	auto cur { dynamic_cast<Pair *>(lst) };
	if (! cur) {
		std::cerr << "cdr: no list: " << lst << "\n";
		return nullptr;
	}
	return cur->rest();
}

std::ostream &Procedure::write(std::ostream &out) const {
	out << "(lambda " << args_;
	for (Element *cur { body_ }; cur && cur != &Null; cur = cdr(cur)) {
		auto v { car(cur) };
		if (v) { out << ' ' << v; } else { out << " #invalid"; }
	}
	out << ')';
	return out;
}

Element *Procedure::apply(Element *arg_values) {
	auto new_env { new Frame { env_ } };

	Element *cur { args_ };
	auto cur_pair { dynamic_cast<Pair *>(cur) };
	for (; cur_pair && cur_pair != &Null; cur = cdr(cur), cur_pair = dynamic_cast<Pair *>(cur)) {
		auto sym { dynamic_cast<Symbol *>(car(cur)) };
		if (! sym) {
			std::cerr << "proc: no argument symbol " << cur << "\n";
			return nullptr;
		}
		auto value { car(arg_values) };
		if (! value) {
			std::cerr << "proc: no argument value for " << cur << "\n";
			return nullptr;
		}
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	if (! cur) {
		std::cerr << "proc: error parsing arguments " << args_ << "\n";
		return nullptr;
	}
	if (cur != &Null) {
		auto sym { dynamic_cast<Symbol *>(cur) };
		if (! sym) {
			std::cerr << "proc: no argument symbol " << cur << "\n";
			return nullptr;
		}
		if (! arg_values) {
			std::cerr << "proc: no fill arg values\n";
			return nullptr;
		}
		new_env->insert(sym->value(), arg_values);
	}

	cur = body_;
	cur_pair = dynamic_cast<Pair *>(cur);
	Element *value = &Null;
	for (; cur_pair && cur_pair != &Null; cur = cdr(cur), cur_pair = dynamic_cast<Pair *>(cur)) {
		Element *exp { car(cur_pair) };
		if (! exp) {
			std::cerr << "proc: no statement " << cur_pair << "\n";
			return nullptr;
		}
		value = eval(exp, new_env);
		if (! value) {
			std::cerr << "proc: can't eval " << exp << "\n";
		}
	}
	if (cur != &Null) {
		std::cerr << "proc: error in body " << body_ << "\n";
		return nullptr;
	}
	return value;
}

Element *apply(Element *op, Element *operands) {
	auto prim { dynamic_cast<Primitive *>(op) };
	if (prim) {
		return prim->apply(operands);
	}
	auto proc { dynamic_cast<Procedure *>(op) };
	if (proc) {
		return proc->apply(operands);
	}
	std::cerr << "unknown operator " << op << "\n";
	return nullptr;
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
		if (! name) {
			std::cerr << "define: no function name " << first << "\n";
		}
		return name;
	}
	std::cerr << "define: no key symbol " << lst << "\n";
	return nullptr;
}

Element *caddr(Element *lst) {
	return car(cdr(cdr(lst)));
}

Element *cadddr(Element *lst) {
	return car(cdr(cdr(cdr(lst))));
}

inline Element *define_value(Pair *lst, Frame *env) {
	auto args { dynamic_cast<Pair *>(cadr(lst)) };
	if (args) {
		return new Procedure {
			cdr(args), cdr(cdr(lst)), env
		};
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
	return cdr(cdr(lst));
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
	if (! cond || ! cons) {
		std::cerr << "cond: invalid pair " << lst << "\n";
		return nullptr;
	}
	auto sym { dynamic_cast<Symbol *>(cond) };
	if (sym && sym->value() == "else") {
		if (cdr(lst) != &Null) {
			std::cerr << "cond: else not in last case\n";
			return nullptr;
		}
		return new Pair {
			new Symbol { "begin" },
			cons
		};
	}
	return new Pair {
		new Symbol { "if" },
		new Pair {
			cond,
			new Pair {
				new Pair {
					new Symbol { "begin" },
					cons
				},
				new Pair {
					build_cond(cdr(lst)),
					&Null
				}
			}
		}
	};
}

inline bool is_begin_special(Pair *lst) {
	return is_tagged_list(lst, "begin");
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
			if (! key || ! value) {
				std::cerr << "incomplete define " << lst_value << "\n";
				return nullptr;
			}
			env->insert(key->value(), value);
			return value;
		}
		if (is_lambda_special(lst_value)) {
			auto args { lambda_args(lst_value) };
			auto body { lambda_body(lst_value) };
			if (! args || ! body) {
				std::cerr << "incomplete lambda " << lst_value << "\n";
				return nullptr;
			}
			return new Procedure(args, body, env);
		}
		if (is_if_special(lst_value)) {
			auto condition { eval(if_condition(lst_value), env) };
			if (! condition) {
				std::cerr << "if: invalid condition " << lst_value << "\n";
				return nullptr;
			}
			if (condition != &Null) {
				return eval(if_consequence(lst_value), env);
			} else {
				return eval(if_alternative(lst_value), env);
			}
		}
		if (is_cond_special(lst_value)) {
			auto expr { build_cond(cdr(lst_value)) };
			if (! expr) {
				std::cerr << "cond: invalid expression " << lst_value << "\n";
				return nullptr;
			}
			std::cerr << expr << "\n";
			return eval(expr, env);
		}
		if (is_begin_special(lst_value)) {
			Element *result;
			auto cur { cdr(lst_value) };
			for (; cur && cur != &Null; cur = cdr(cur)) {
				result = eval(car(cur), env);
				if (! result) {
					std::cerr << "begin: can't eval " << car(cur) << "\n";
					break;
				}
			}
			return result;
		}
		auto lst { eval_list(lst_value, env) };
		return apply(lst->head(), lst->rest());
	}
	std::cerr << "unknown expression " << exp << "\n";
	return nullptr;
}

class Car_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Car_Primitive::apply(Element *args) {
	auto cur { dynamic_cast<Pair *>(args) };
	if (! cur) {
		std::cerr << "car: no args: " << args << "\n";
		return nullptr;
	}
	return car(cur->head());
}

class Cdr_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Cdr_Primitive::apply(Element *args) {
	auto cur { dynamic_cast<Pair *>(args) };
	if (! cur) {
		std::cerr << "cdr: no args: " << args << "\n";
		return nullptr;
	}
	return cdr(cur->head());
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
	if (! first || ! second || nxt != &Null) {
		std::cerr << "cons: wrong arguments: " << args << "\n";
		return nullptr;
	}
	return new Pair { first, second };
}

class Plus_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};


Element *Plus_Primitive::apply(Element *args) {
	int i_sum { 0 };
	double f_sum { 0.0 };
	bool is_float { false };

	Element *cur { args };
	for (; cur && cur != &Null; cur = cdr(cur)) {
		if (is_float) {
			auto vi { dynamic_cast<Integer *>(car(cur)) };
			if (vi) {
				f_sum += vi->value();
				continue;
			}
			auto vf { dynamic_cast<Float *>(car(cur)) };
			if (vf) {
				f_sum += vf->value();
				continue;
			}
		} else {
			auto vi { dynamic_cast<Integer *>(car(cur)) };
			if (vi) {
				i_sum += vi->value();
				continue;
			}
			auto vf { dynamic_cast<Float *>(car(cur)) };
			if (vf) {
				is_float = true;
				f_sum = i_sum + vf->value();
				continue;
			}
		}
		std::cerr << "+: no number: " << car(cur) << "\n";
		return nullptr;
	}
	if (! cur) {
		std::cerr << "+: wrong arguments: " << args << "\n";
		return nullptr;
	}
	if (is_float) {
		return new Float { f_sum };
	}
	return new Integer { i_sum };
}

class Minus_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Minus_Primitive::apply(Element *args) {
	int sum { 0 };
	
	Element *cur { args };
	if (cur == &Null) { return new Integer(sum); }
	auto first { dynamic_cast<Integer *>(car(cur)) };
	if (! first) {
		std::cerr << "-: no number: " << car(cur) << "\n";
		return nullptr;
	}
	cur = cdr(cur);
	if (cur == &Null) { return new Integer(-first->value()); }
	sum = first->value();
	for (; cur && cur != &Null; cur = cdr(cur)) {
		auto vv { dynamic_cast<Integer *>(car(cur)) };
		if (! vv) {
			std::cerr << "-: no number: " << car(cur) << "\n";
			return nullptr;
		}
		sum -= vv->value();
	}
	if (! cur) {
		std::cerr << "-: wrong arguments: " << args << "\n";
		return nullptr;
	}
	return new Integer { sum };
}

class Times_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};


Element *Times_Primitive::apply(Element *args) {
	int i_product { 1 };
	double f_product { 1.0 };
	bool is_float { false };

	Element *cur { args };
	for (; cur && cur != &Null; cur = cdr(cur)) {
		if (is_float) {
			auto vi { dynamic_cast<Integer *>(car(cur)) };
			if (vi) {
				f_product *= vi->value();
				continue;
			}
			auto vf { dynamic_cast<Float *>(car(cur)) };
			if (vf) {
				is_float = true;
				f_product *= vf->value();
				continue;
			}
		} else {
			auto vi { dynamic_cast<Integer *>(car(cur)) };
			if (vi) {
				i_product *= vi->value();
				continue;
			}
			auto vf { dynamic_cast<Float *>(car(cur)) };
			if (vf) {
				is_float = true;
				f_product = i_product * vf->value();
				continue;
			}
		}
		std::cerr << "*: no number: " << car(cur) << "\n";
		return nullptr;
	}
	if (! cur) {
		std::cerr << "*: wrong arguments: " << args << "\n";
		return nullptr;
	}
	if (is_float) {
		return new Float { f_product };
	}
	return new Integer { i_product };
}

class Divide_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Divide_Primitive::apply(Element *args) {
	int product { 1 };
	
	Element *cur { args };
	auto first { dynamic_cast<Integer *>(car(cur)) };
	if (! first) {
		std::cerr << "/: no number: " << car(cur) << "\n";
		return nullptr;
	}
	product = first->value();
	cur = cdr(cur);
	if (cur == &Null) { 
		if (product == 0) {
			std::cerr << "/: divide by 0 " << first << "\n";
			return nullptr;
		}
		return new Integer(1/product);
	}
	for (; cur && cur != &Null; cur = cdr(cur)) {
		auto vv { dynamic_cast<Integer *>(car(cur)) };
		if (! vv) {
			std::cerr << "-: no number: " << car(cur) << "\n";
			return nullptr;
		}
		product /= vv->value();
	}
	if (! cur) {
		std::cerr << "/: wrong arguments: " << args << "\n";
		return nullptr;
	}
	return new Integer { product };
}

class Less_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Less_Primitive::apply(Element *args) {
	Element *first { car(args) };
	Element *second { cadr(args) };
	if (cdr(cdr(args)) != &Null || ! first || ! second) {
		std::cerr << "<: argument error: " << args << "\n";
		return nullptr;
	}
	auto first_i { dynamic_cast<Integer *>(first) };
	auto second_i { dynamic_cast<Integer *>(second) };
	if (first_i && second_i) {
		if (first_i->value() < second_i->value()) {
			return new Integer { 1 };
		} else {
			return &Null;
		}
	}
	std::cerr << "<: invalid argument types: " << args << "\n";
	return nullptr;
}

Frame initial_frame { nullptr };

class Garbage_Collect_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *Garbage_Collect_Primitive::apply(Element *args) {
	current_mark = ! current_mark;
	initial_frame.mark();
	int kept { 0 };
	int collected { 0 };
	Element *prev { nullptr };
	Element *cur { all_elements };
	while (cur) {
		if (cur != &Null && cur->get_mark() != current_mark) {
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

static const char setup[] =
	"(define nil ())\n"
	"(define (cadr l) (car (cdr l)))\n"
	"(define (cddr l) (cdr (cdr l)))\n"
	"(define (caddr l) (car (cddr l)))\n"
	"(define (cdddr l) (cdr (cddr l)))\n"
	"(define #t 1)\n"
	"(define #f nil)\n"
	"(define true #t)\n"
	"(define false #f)\n";

int main(int argc, const char *argv[]) {
	initial_frame.insert("car", new Car_Primitive());
	initial_frame.insert("cdr", new Cdr_Primitive());
	initial_frame.insert("list", new List_Primitive());
	initial_frame.insert("cons", new Cons_Primitive());
	initial_frame.insert("+", new Plus_Primitive());
	initial_frame.insert("-", new Minus_Primitive());
	initial_frame.insert("*", new Times_Primitive());
	initial_frame.insert("/", new Divide_Primitive());
	initial_frame.insert("<", new Less_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());

	{
		std::istringstream s { setup };
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
