#include <iostream>
#include <string>
#include <sstream>

class Element {
	public:
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) const = 0;
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

class Pair : public Element {
		Element *head_;
		Element *rest_;
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

Element *read_expression();

void eat_space() {
	while (ch != EOF && ch <= ' ') { ch = std::cin.get(); }
}

Element *read_list() {
	eat_space();
	if (ch == EOF) {
		std::cerr << "incomplete list\n";
		return nullptr;
	}
	if (ch == ')') {
		ch = std::cin.get();
		return &Null;
	}
	auto exp { read_expression() };
	return new Pair { exp, read_list() };
}

Element *read_expression() {
	eat_space();
	if (ch == EOF) { return nullptr; }
	if (ch == '(') { ch = std::cin.get(); return read_list(); }
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
		ch = std::cin.get();
		if (ch == EOF || ch <= ' ' || ch == '(' || ch == ')') { break; }
	}
	return numeric ?
		static_cast<Element *>(new Integer { value }) :
		static_cast<Element *>(new Symbol { result.str() });
}

#include <map>

class Frame {
		Frame *next_;
		std::map<std::string, Element *> elements_;
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Element *value);
		Element *get(const std::string &key) const;
};

void Frame::insert(const std::string &key, Element *value) {
	elements_.emplace(key, value);
}

Element *Frame::get(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? it->second :
		next_ ? next_->get(key) : nullptr;
}

Element *apply(Element *op, Element *operands) {
	std::cerr << "unknown operator " << op << "\n";
	return nullptr;
}

Element *eval(Element *exp, Frame *env);

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

Element *eval(Element *exp, Frame *env) {
	if (! exp || exp == &Null) { return exp; }
	auto int_value { dynamic_cast<Integer *>(exp) };
	if (int_value) { return int_value; }
	auto sym_value { dynamic_cast<Symbol *>(exp) };
	if (sym_value) {
		Element *got { env->get(sym_value->value()) };
		return got ?: exp;
	}
	auto lst_value { dynamic_cast<Pair *>(exp) };
	if (lst_value) {
		auto lst { eval_list(lst_value, env) };
		return apply(lst->head(), lst->rest());
	}
	std::cerr << "unknown expression " << exp << "\n";
	return nullptr;
}

int main() {
	Frame initial_frame { nullptr };
	initial_frame.insert("nil", &Null);

	for (;;) {
		std::cout << "? ";
		auto exp { read_expression() };
		if (! exp) { break; }
		exp = eval(exp, &initial_frame);
		std::cout << exp << "\n";
	}
}
