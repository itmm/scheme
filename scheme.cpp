#include <iostream>
#include <string>
#include <sstream>

class Element {
	public:
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) const = 0;
};

std::ostream &operator<<(std::ostream &out, Element *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}

class Symbol : public Element {
		std::string value_;
	public:
		Symbol(std::string value): value_ { value } { }
		std::string value() const { return value_; }
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Symbol::write(std::ostream &out) const {
	return out << value_;
}

class Integer : public Element {
		int value_;
	public:
		Integer(int value): value_ { value } { }
		int value() const { return value_; }
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Integer::write(std::ostream &out) const {
	return out << value_;
}

class Pair : public Element {
		Element *head_;
		Element *rest_;
	public:
		Pair(Element *head, Element *rest): head_ { head }, rest_ { rest } { }
		Element *head() const { return head_; }
		Element *rest() const { return rest_; }
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Pair::write(std::ostream &out) const {
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
		return nullptr;
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
	return numeric ? static_cast<Element *>(new Integer { value }) : static_cast<Element *>(new Symbol { result.str() });
}

int main() {
	for (;;) {
		std::cout << "? ";
		auto exp { read_expression() };
		if (! exp) { break; }
		// eval(exp, env);
		std::cout << exp << "\n";
	}
}
