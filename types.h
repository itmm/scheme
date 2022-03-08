/**
 * basic types
 * Integers can be as long as memory permits
 * nullptr is treated as the empty pair
 */

template<typename VALUE_TYPE>
class Value_Element : public Element {
		VALUE_TYPE value_;
	public:
		Value_Element(const VALUE_TYPE &value): value_ { value } { }
		const VALUE_TYPE &value() const { return value_; }
		std::ostream &write(std::ostream &out) const override {
			return out << value_;
		}
};

using Float = Value_Element<double>;
class String : public Value_Element<std::string> {
	public:
		String(const std::string &value) : Value_Element(value) { }
		std::ostream &write(std::ostream &out) const override {
			return out << '"' << value() << '"';
		}
};

#include <map>

class Symbol : public Element {
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
		std::ostream &write(std::ostream &out) const override {
			return out << value_;
		}
};

std::map<std::string, Symbol *> Symbol::symbols_;

#include <vector>
#include <algorithm>

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
		Integer(const Digits &digits): digits_ { digits } {
			normalize();
		}
		Integer(Digits &&digits): digits_ { std::move(digits) } {
			normalize();
		}
		static Element *create(const std::string &digits);
		static Element *create(unsigned value);
		const Digits &digits() const { return digits_; }
		double float_value() const { 
			double result { 0.0 };
			for (auto it { digits_.rbegin() }; it != digits_.rend(); ++it) {
				result = result * 10.0 + *it;
			}
			return result;
		}
		bool is_zero() const { return digits_.empty(); }
		virtual bool is_negative() const { return false; }
		Integer *negate() const;
		std::ostream &write(std::ostream &out) const override {
			if (is_negative()) { out << '-'; }
			if (digits_.empty()) { return out << '0'; }
			for (auto i { digits_.rbegin() }; i != digits_.rend(); ++i) {
				out << static_cast<char>('0' + *i);
			}
			return out;
		}
};

class Negative_Integer : public Integer {
	public:
		Negative_Integer(const Digits &digits): Integer { digits } { }
		Negative_Integer(Digits &&digits): Integer { std::move(digits) } { }
		virtual bool is_negative() const { return ! is_zero(); }
};

Integer *Integer::negate() const { 
	if (is_negative() || is_zero()) {
		return new Integer { digits_ };
	} else {
		return new Negative_Integer { digits_ };
	}
}

Element *Integer::create(const std::string &digits) {
	Integer::Digits result;
	bool negative { false };
	for (auto it { digits.rbegin() }; it != digits.rend(); ++it) {
		if (*it == '+') { continue; }
		if (*it == '-') { negative = ! negative; continue; }
		int digit { *it - '0' };
		if (digit < 0 || digit > 9) {
			return err("integer", "invalid digits", new String { digits });
		}
		result.push_back(digit);
	}
	if (negative) {
		return new Negative_Integer { std::move(result) };
	} else {
		return new Integer { std::move(result) };
	}
}

Element *Integer::create(unsigned value) {
	Integer::Digits result;
	for (; value; value /= 10) {
		result.push_back(value % 10);
	}
	return new Integer { std::move(result) };
}

auto One { Integer::create("1") };
auto Zero { Integer::create("0") };

class Fraction : public Element {
		Integer *nom_;
		Integer *denum_;
		Fraction(Integer *nom, Integer *denum): nom_ { nom }, denum_ { denum } { }

	public:
		static Fraction *create_forced(Integer *nom, Integer *denum);
		static Element *create(Element *nom, Element *denum);
		Integer *nom() const { return nom_; }
		Integer *denum() const { return denum_; }
		std::ostream &write(std::ostream &out) const override {
			return out << nom_ << '/' << denum_;
		}
		Fraction *negate() { return new Fraction { nom_->negate(), denum_ }; }
};

Element *negate(Element *a) {
	auto ai { dynamic_cast<Integer *>(a) };
	if (ai) {
		return ai->negate();
	}
	auto afr { dynamic_cast<Fraction *>(a) };
	if (afr) {
		return afr->negate();
	}
	auto afl { dynamic_cast<Float *>(a) };
	if (afl) {
		return new Float { -afl->value() };
	}
	ASSERT(false, "negate");
}

bool is_negative(Element *a) {
	auto ai { dynamic_cast<Integer *>(a) };
	if (ai) { return ai->is_negative(); };
	auto afr { dynamic_cast<Fraction *>(a) };
	if (afr) { return is_negative(afr->nom()); };
	auto afl { dynamic_cast<Float *>(a) };
	if (afl) { return afl->value() < 0.0; }
	err("is_negative", "no_number", a);
	return false;
}

bool is_zero(Element *a) {
	auto ai { dynamic_cast<Integer *>(a) };
	if (ai) { return ai->is_zero(); };
	auto afr { dynamic_cast<Fraction *>(a) };
	if (afr) { return is_zero(afr->nom()); };
	auto afl { dynamic_cast<Float *>(a) };
	if (afl) { return afl->value() == 0.0; }
	err("is_zero", "no_number", a);
	return false;
}

class Propagate {
	protected:
		virtual Element *apply_int(Integer *a, Integer *b) = 0;
		virtual Element *apply_fract(Fraction *a, Fraction *b) = 0;
		virtual Element *apply_float(Float *a, Float *b) = 0;
	public:
		Element *propagate(Element *a, Element *b);
};

Element *Propagate::propagate(Element *a, Element *b) {
	auto ai { dynamic_cast<Integer *>(a) };
	auto bi { dynamic_cast<Integer *>(b) };
	if (ai && bi) { return apply_int(ai, bi); }
	auto afr { dynamic_cast<Fraction *>(a) };
	auto bfr { dynamic_cast<Fraction *>(b) };
	if (afr || bfr) {
		if (! afr && ai) { afr = Fraction::create_forced(ai, dynamic_cast<Integer *>(One)); }
		if (! bfr && bi) { bfr = Fraction::create_forced(bi, dynamic_cast<Integer *>(One)); }
			if (afr && bfr) { return apply_fract(afr, bfr); }
	}
	auto afl { dynamic_cast<Float *>(a) };
	auto bfl { dynamic_cast<Float *>(b) };
	if (afl || bfl) {
		if (! afl) {
			if (ai) { afl = new Float { ai->float_value() }; }
			else if (afr) { afl = new Float { afr->nom()->float_value() / afr->denum()->float_value() }; }
		}
		if (! bfl) {
			if (bi) { bfl = new Float { bi->float_value() }; }
			else if (bfr) { bfl = new Float { bfr->nom()->float_value() / bfr->denum()->float_value() }; }
		}
		if (afl && bfl) { return apply_float(afl, bfl); }
	}

	return err("propagate", "can't propagate", a, b);
}

Element *add(Element *a, Element *b);
Element *mult(Element *a, Element *b);
Element *sub(Element *a, Element *b);
Element *div(Element *a, Element *b);

class Add_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			Integer::Digits digits;
			auto a_i { a->digits().begin() };
			auto b_i { b->digits().begin() };
			int carry { 0 };
			for (;;) {
				if (a_i == a->digits().end() && b_i == b->digits().end() && ! carry) { break; }
				int v { carry };
				if (a_i != a->digits().end()) { v += *a_i++; }
				if (b_i != b->digits().end()) { v += *b_i++; }
				if (v >= 10) { v -= 10; carry = 1; } else { carry = 0; }
				digits.push_back(v);
			}

			return new Integer { std::move(digits) };
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				add(mult(a->nom(), b->denum()), mult(b->nom(), a->denum())),
				mult(a->denum(), b->denum())
			);
		}
		Element *apply_float(Float *a, Float *b) {
			return new Float { a->value() + b->value() };
		}
};

Element *add(Element *a, Element *b) {
	auto a_neg { is_negative(a) };
	auto b_neg { is_negative(b) };
	if (! a_neg && b_neg) {
		return sub(a, negate(b));
	} else if (a_neg && b_neg) {
		return negate(add(negate(a), negate(b)));
	} else if (a_neg && ! b_neg) {
		return negate(sub(negate(a), b));
	}
	return Add_Propagate{}.propagate(a, b);
}

class Sub_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			Integer::Digits digits;
			auto a_i { a->digits().begin() };
			auto b_i { b->digits().begin() };
			int carry { 0 };
			for (;;) {
				if (a_i == a->digits().end()) { break; }
				int v = { *a_i++ + carry };
				if (b_i != b->digits().end()) { v -= *b_i++; }
				if (v < 0) { v += 10; carry = -1; } else { carry = 0; }
				digits.push_back(v);
			}
			return new Integer { std::move(digits) };
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				sub(mult(a->nom(), b->denum()), mult(b->nom(), a->denum())),
				mult(a->denum(), b->denum())
			);
		}
		Element *apply_float(Float *a, Float *b) {
			return new Float { a->value() - b->value() };
		}
};

Element *to_bool(bool cond);
bool is_true(Element *value);
bool is_false(Element *value);

Element *less(Element *a, Element *b);

Element *sub(Element *a, Element *b) {
	auto a_neg { is_negative(a) };
	auto b_neg { is_negative(b) };
	if (! a_neg && b_neg) {
		return add(a, negate(b));
	} else if (a_neg && b_neg) {
		return negate(sub(negate(a), negate(b)));
	} else if (a_neg && ! b_neg) {
		return negate(add(negate(a), b));
	}
	if (is_true(less(a, b))) {
		return negate(sub(b, a));
	}
	return Sub_Propagate{}.propagate(a, b);
}

class Mul_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			Integer::Digits digits;
			auto a_i { a->digits().begin() };
			unsigned offset { 0 };
			for (; a_i != a->digits().end(); ++a_i, ++offset) {
				auto b_i { b->digits().begin() };
				int mult { *a_i };
				int carry { 0 };
				unsigned i { 0 };
				while (b_i != b->digits().end() || carry) {
					unsigned idx { offset + i++ };
					while (idx >= digits.size()) { digits.push_back(0); }
					int value { carry + digits[idx] };
					if (b_i != b->digits().end()) { value += (*b_i++) * mult; }
					carry = value / 10;
					digits[idx] = value % 10;
				}
			}

			return new Integer { std::move(digits) };
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				mult(a->nom(), b->nom()), mult(a->denum(), b->denum())
			);
		}
		Element *apply_float(Float *a, Float *b) {
			return new Float { a->value() * b->value() };
		}
};

Element *mult(Element *a, Element *b) {
	auto a_neg { is_negative(a) };
	auto b_neg { is_negative(b) };
	if (a_neg && b_neg) {
		return mult(negate(a), negate(b));
	} else if (a_neg && ! b_neg) {
		return negate(mult(negate(a), b));
	} else if (! a_neg && b_neg) {
		return negate(mult(a, negate(b)));
	}

	return Mul_Propagate{}.propagate(a, b);
}

class Div_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			return Fraction::create(a, b);
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				mult(a->nom(), b->denum()), mult(a->denum(), b->nom())
			);
		}
		Element *apply_float(Float *a, Float *b) {
			return new Float { a->value() / b->value() };
		}
};

Element *div(Element *a, Element *b) {
	auto a_neg { is_negative(a) };
	auto b_neg { is_negative(b) };
	if (a_neg && b_neg) {
		return div(negate(a), negate(b));
	} else if (a_neg && ! b_neg) {
		return negate(div(negate(a), b));
	} else if (! a_neg && b_neg) {
		return negate(div(a, negate(b)));
	}

	ASSERT(! is_zero(b), "div");

	return Div_Propagate{}.propagate(a, b);
}

class Less_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			if (a->digits().size() > b->digits().size()) { return to_bool(false); }
			if (a->digits().size() < b->digits().size()) { return to_bool(true); }

			auto a_i { a->digits().rbegin() };
			auto b_i { b->digits().rbegin() };
			for (; a_i != a->digits().rend(); ++a_i, ++b_i) {
				if (*a_i < *b_i) { return to_bool(true); }
				if (*a_i > *b_i) { return to_bool(false); }
			}
			return to_bool(false);
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return less(mult(a->nom(), b->denum()), mult(b->nom(), a->denum()));
		}
		Element *apply_float(Float *a, Float *b) {
			return to_bool(a->value() < b->value());
		}
};

Element *less(Element *a, Element *b) {
	auto a_neg { is_negative(a) };
	auto b_neg { is_negative(b) };
	if (a_neg && b_neg) {
		return less(negate(b), negate(a));
	} else if (a_neg && ! b_neg) {
		return to_bool(! is_zero(a) || ! is_zero(b));
	} else if (! a_neg && b_neg) {
		return to_bool(false);
	}

	return Less_Propagate{}.propagate(a, b);
}

Element *is_equal_num(Element *a, Element *b);

class Equal_Propagate : public Propagate {
	protected:
		Element *apply_int(Integer *a, Integer *b) {
			if (a->digits().size() != b->digits().size()) {
				return to_bool(false);
			}
			auto a_i { a->digits().begin() };
			auto b_i { b->digits().begin() };
			for (; a_i != a->digits().end(); ++a_i, ++b_i) {
				if (*a_i != *b_i) { return to_bool(false); }
			}
			return to_bool(true);
		}
		Element *apply_fract(Fraction *a, Fraction *b) {
			return is_equal_num(mult(a->nom(), b->denum()), mult(b->nom(), a->denum()));
		}
		Element *apply_float(Float *a, Float *b) {
			return to_bool(a->value() == b->value());
		}
};

Element *is_equal_num(Element *a, Element *b) {
	if (is_zero(a) && is_zero(b)) { return to_bool(true); }
	if (is_negative(a) != is_negative(b)) { return to_bool(false); }

	return Equal_Propagate{}.propagate(a, b);

}

static Element *half(Element *num) {
	auto in { dynamic_cast<Integer *>(num) };
	ASSERT(in, "half");
	Integer::Digits result;
	result.resize(in->digits().size());
	int carry { 0 };
	for (int i { static_cast<int>(in->digits().size()) - 1}; i >= 0; --i) {
		int v { carry + in->digits()[i] };
		if (v % 2) { carry = 10; v -= 1; } else { carry = 0; }
		result[i] = v/2;
	}
	return new Integer { std::move(result) };
}

bool is_int(Element *a) {
	return dynamic_cast<Integer *>(a);
}

Element *remainder(Element *a, Element *b) {
	ASSERT(is_int(a) && is_int(b), "remainder");
	if (is_true(is_equal_num(b, One))) { return Zero; }
	if (is_true(less(a, b))) { return a; }

	Element *min { One };
	Element *max { Integer::create("2") };

	for (;;) {
		auto prod { mult(max, b) };
		if (is_false(less(prod, a))) { break; }
		max = mult(max, max);
	}

	for (;;) {
		auto diff { sub(max, min) };
		if (is_false(less(One, diff))) { break; }
		auto mid { half(add(max, min)) };
		auto prod { mult(mid, b) };
		if (is_true(less(prod, a))) { min = mid; }
		else if (is_true(less(a, prod))) { max = mid; }
		else { min = mid; break; }
	}
	return sub(a, mult(min, b));
}

Integer *div_int(Integer *a, Integer *b) {
	if (! a || ! b) { err("div_int", "no int"); return nullptr; }
	if (b->is_negative()) { return div_int(a->negate(), b->negate()); }
	if (a->is_negative()) {
		auto res { div_int(a->negate(), b) };
		return res ? res->negate() : nullptr;
	}

	Element *min { One };
	Element *max { Integer::create("2") };

	for (;;) {
		auto prod { mult(max, b) };
		if (is_false(less(prod, a))) { break; }
		max = mult(max, max);
	}

	for (;;) {
		auto diff { sub(max, min) };
		if (is_false(less(One, diff))) { break; }
		auto mid { half(add(max, min)) };
		auto prod { mult(mid, b) };
		if (is_true(less(prod, a))) { min = mid; }
		else if (is_true(less(a, prod))) { max = mid; }
		else { min = mid; break; }
	}
	return dynamic_cast<Integer *>(min);
}

Element *gcd(Element *a, Element *b) {
	if (is_zero(b)) { return a; }
	return gcd(b, remainder(a, b));
}

Fraction *Fraction::create_forced(Integer *nom, Integer *denum) {
	if (! nom || ! denum) { err("fraction", "setup"); return nullptr; }
	if (is_negative(denum)) {
		return create_forced(nom->negate(), denum->negate());
	}
	if (is_negative(nom)) {
		return create_forced(nom->negate(), denum)->negate();
	}

	auto g { dynamic_cast<Integer *>(gcd(nom, denum)) };
	if (nom && denum && g && is_false(is_equal_num(g, One))) {
		nom = div_int(nom, g);
		denum = div_int(denum, g);
		if (! nom || ! denum) { err("fraction", "gcd"); return nullptr; }
	}
	return new Fraction { nom, denum };
}

Element *Fraction::create(Element *nom, Element *denum) {
	if (is_negative(denum)) {
		return create(::negate(nom), ::negate(denum));
	}
	if (is_negative(nom)) {
		return ::negate(create(::negate(nom), denum));
	}

	auto ni { dynamic_cast<Integer *>(nom) };
	auto di { dynamic_cast<Integer *>(denum) };
	ASSERT(ni && di, "fraction");
	auto g { dynamic_cast<Integer *>(gcd(ni, di)) };
	if (g && is_false(is_equal_num(g, One))) {
		ni = div_int(ni, g);
		di = div_int(di, g);
		ASSERT(ni && di, "fraction");
	}
	if (is_true(is_equal_num(One, di))) {
		return ni;
	}
	return new Fraction { ni, di };
}

#define True One

Element *to_bool(bool cond) {
	return cond ?  True : nullptr;
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
