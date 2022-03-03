/**
 * basic types
 * Integers can be as long as memory permits
 * nullptr is treated as the empty pair
 */

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
			if (digits_.empty()) { return out << '0'; }
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
	unsigned offset { 0 };
	for (; a_i != a.digits().end(); ++a_i, ++offset) {
		auto b_i { b.digits().begin() };
		int mult { *a_i };
		int carry { 0 };
		unsigned i { 0 };
		while (b_i != b.digits().end() || carry) {
			unsigned idx { offset + i++ };
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
