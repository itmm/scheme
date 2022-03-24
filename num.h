/**
 * numeric types
 */

#include <algorithm>

using Float = Value_Element<double>;

class Integer : public Obj {
	public:
		using Digits = std::vector<unsigned short>;
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
		static Obj *create(const std::string &digits);
		static Obj *create(unsigned value);
		const Digits &digits() const { return digits_; }
		double float_value() const { 
			double result { 0.0 };
			for (auto it { digits_.rbegin() }; it != digits_.rend(); ++it) {
				result = result * 10000.0 + *it;
			}
			return result;
		}
		bool is_zero() const { return digits_.empty(); }
		virtual bool is_negative() const { return false; }
		Integer *negate() const;
		std::ostream &write(std::ostream &out) override {
			if (is_negative()) { out << '-'; }
			if (digits_.empty()) { return out << '0'; }
			bool first { true };
			for (auto i { digits_.rbegin() }; i != digits_.rend(); ++i) {
				unsigned val { *i };
				unsigned d0 { val % 10 }; val /= 10;
				unsigned d1 { val % 10 }; val /= 10;
				unsigned d2 { val % 10 }; val /= 10;
				unsigned d3 { val % 10 };
				if (d3 || ! first) {
					out << static_cast<char>('0' + d3);
					first = false;
				}
				if (d2 || ! first) {
					out << static_cast<char>('0' + d2);
					first = false;
				}
				if (d1 || ! first) {
					out << static_cast<char>('0' + d1);
					first = false;
				}
				if (d0 || ! first) {
					out << static_cast<char>('0' + d0);
					first = false;
				}
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

Obj *Integer::create(const std::string &digits) {
	Integer::Digits result;
	bool negative { false };
	unsigned short v { 0 };
	unsigned short mult { 1 };
	for (auto it { digits.rbegin() }; it != digits.rend(); ++it) {
		if (*it == '+') { continue; }
		if (*it == '-') { negative = ! negative; continue; }
		int digit { *it - '0' };
		if (digit < 0 || digit > 9) {
			return err("integer", "invalid digits", new String { digits });
		}
		v += digit * mult;
		if (mult == 1000) {
			result.push_back(v); v = 0; mult = 1;
		} else {
			mult *= 10;
		}
	}
	if (v) { result.push_back(v); }
	if (negative) {
		return new Negative_Integer { std::move(result) };
	} else {
		return new Integer { std::move(result) };
	}
}

Obj *Integer::create(unsigned value) {
	Integer::Digits result;
	for (; value; value /= 10000) {
		result.push_back(value % 10000);
	}
	return new Integer { std::move(result) };
}

auto one { Integer::create(1) };
auto two { Integer::create(2) };
auto zero { Integer::create(0) };

class Fraction : public Obj {
		Integer *num_;
		Integer *denom_;
		Fraction(Integer *num, Integer *denom): num_ { num }, denom_ { denom } { }

	public:
		static Fraction *create_forced(Integer *num, Integer *denom);
		static Obj *create(Obj *num, Obj *denom);
		Integer *num() const { return num_; }
		Integer *denom() const { return denom_; }
		std::ostream &write(std::ostream &out) override {
			return out << num_ << '/' << denom_;
		}
		Fraction *negate() { return new Fraction { num_->negate(), denom_ }; }
};

Obj *negate(Obj *a) {
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

bool is_negative(Obj *a) {
	auto ai { dynamic_cast<Integer *>(a) };
	if (ai) { return ai->is_negative(); };
	auto afr { dynamic_cast<Fraction *>(a) };
	if (afr) { return is_negative(afr->num()); };
	auto afl { dynamic_cast<Float *>(a) };
	if (afl) { return afl->value() < 0.0; }
	err("is_negative", "no_number", a);
	return false;
}

bool is_zero(Obj *a) {
	auto ai { dynamic_cast<Integer *>(a) };
	if (ai) { return ai->is_zero(); };
	auto afr { dynamic_cast<Fraction *>(a) };
	if (afr) { return is_zero(afr->num()); };
	auto afl { dynamic_cast<Float *>(a) };
	if (afl) { return afl->value() == 0.0; }
	err("is_zero", "no_number", a);
	return false;
}

class Propagate {
	protected:
		virtual Obj *apply_int(Integer *a, Integer *b) = 0;
		virtual Obj *apply_fract(Fraction *a, Fraction *b) = 0;
		virtual Obj *apply_float(Float *a, Float *b) = 0;
	public:
		Obj *propagate(Obj *a, Obj *b);
};

Obj *Propagate::propagate(Obj *a, Obj *b) {
	auto ai { dynamic_cast<Integer *>(a) };
	auto bi { dynamic_cast<Integer *>(b) };
	if (ai && bi) { return apply_int(ai, bi); }
	auto afr { dynamic_cast<Fraction *>(a) };
	auto bfr { dynamic_cast<Fraction *>(b) };
	if (afr || bfr) {
		if (! afr && ai) { afr = Fraction::create_forced(ai, dynamic_cast<Integer *>(one)); }
		if (! bfr && bi) { bfr = Fraction::create_forced(bi, dynamic_cast<Integer *>(one)); }
			if (afr && bfr) { return apply_fract(afr, bfr); }
	}
	auto afl { dynamic_cast<Float *>(a) };
	auto bfl { dynamic_cast<Float *>(b) };
	if (afl || bfl) {
		if (! afl) {
			if (ai) { afl = new Float { ai->float_value() }; }
			else if (afr) { afl = new Float { afr->num()->float_value() / afr->denom()->float_value() }; }
		}
		if (! bfl) {
			if (bi) { bfl = new Float { bi->float_value() }; }
			else if (bfr) { bfl = new Float { bfr->num()->float_value() / bfr->denom()->float_value() }; }
		}
		if (afl && bfl) { return apply_float(afl, bfl); }
	}

	return err("propagate", "can't propagate", a, b);
}

Obj *add(Obj *a, Obj *b);
Obj *mult(Obj *a, Obj *b);
Obj *sub(Obj *a, Obj *b);
Obj *div(Obj *a, Obj *b);

class Add_Propagate : public Propagate {
	protected:
		Obj *apply_int(Integer *a, Integer *b) {
			Integer::Digits digits;
			auto a_i { a->digits().begin() };
			auto b_i { b->digits().begin() };
			int carry { 0 };
			for (;;) {
				if (a_i == a->digits().end() && b_i == b->digits().end() && ! carry) { break; }
				int v { carry };
				if (a_i != a->digits().end()) { v += *a_i++; }
				if (b_i != b->digits().end()) { v += *b_i++; }
				if (v >= 10000) { v -= 10000; carry = 1; } else { carry = 0; }
				digits.push_back(v);
			}

			return new Integer { std::move(digits) };
		}
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				add(mult(a->num(), b->denom()), mult(b->num(), a->denom())),
				mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) {
			return new Float { a->value() + b->value() };
		}
};

Obj *add(Obj *a, Obj *b) {
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
		Obj *apply_int(Integer *a, Integer *b) {
			Integer::Digits digits;
			auto a_i { a->digits().begin() };
			auto b_i { b->digits().begin() };
			int carry { 0 };
			for (;;) {
				if (a_i == a->digits().end()) { break; }
				int v = { *a_i++ + carry };
				if (b_i != b->digits().end()) { v -= *b_i++; }
				if (v < 0) { v += 10000; carry = -1; } else { carry = 0; }
				digits.push_back(v);
			}
			return new Integer { std::move(digits) };
		}
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				sub(mult(a->num(), b->denom()), mult(b->num(), a->denom())),
				mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) {
			return new Float { a->value() - b->value() };
		}
};

Obj *to_bool(bool cond);
bool is_true(Obj *value);
bool is_false(Obj *value);

Obj *less(Obj *a, Obj *b);

Obj *sub(Obj *a, Obj *b) {
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
		Obj *apply_int(Integer *a, Integer *b) {
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
					carry = value / 10000;
					digits[idx] = value % 10000;
				}
			}

			return new Integer { std::move(digits) };
		}
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				mult(a->num(), b->num()), mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) {
			return new Float { a->value() * b->value() };
		}
};

Obj *mult(Obj *a, Obj *b) {
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
		Obj *apply_int(Integer *a, Integer *b) {
			return Fraction::create(a, b);
		}
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return Fraction::create(
				mult(a->num(), b->denom()), mult(a->denom(), b->num())
			);
		}
		Obj *apply_float(Float *a, Float *b) {
			return new Float { a->value() / b->value() };
		}
};

Obj *div(Obj *a, Obj *b) {
	if (is_zero(a)) { return zero; }
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
		Obj *apply_int(Integer *a, Integer *b) {
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
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return less(mult(a->num(), b->denom()), mult(b->num(), a->denom()));
		}
		Obj *apply_float(Float *a, Float *b) {
			return to_bool(a->value() < b->value());
		}
};

Obj *less(Obj *a, Obj *b) {
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

Obj *is_equal_num(Obj *a, Obj *b);

class Equal_Propagate : public Propagate {
	protected:
		Obj *apply_int(Integer *a, Integer *b) {
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
		Obj *apply_fract(Fraction *a, Fraction *b) {
			return is_equal_num(mult(a->num(), b->denom()), mult(b->num(), a->denom()));
		}
		Obj *apply_float(Float *a, Float *b) {
			return to_bool(a->value() == b->value());
		}
};

Obj *is_equal_num(Obj *a, Obj *b) {
	if (is_zero(a) && is_zero(b)) { return to_bool(true); }
	if (is_negative(a) != is_negative(b)) { return to_bool(false); }

	return Equal_Propagate{}.propagate(a, b);

}

static Obj *half(Obj *num) {
	auto in { dynamic_cast<Integer *>(num) };
	ASSERT(in, "half");
	Integer::Digits result;
	result.resize(in->digits().size());
	int carry { 0 };
	for (int i { static_cast<int>(in->digits().size()) - 1}; i >= 0; --i) {
		int v { carry + in->digits()[i] };
		if (v % 2) { carry = 10000; v -= 1; } else { carry = 0; }
		result[i] = v/2;
	}
	return new Integer { std::move(result) };
}

bool is_int(Obj *a) {
	return dynamic_cast<Integer *>(a);
}

Integer *div_int(Integer *a, Integer *b) {
	if (! a || ! b) { err("div_int", "no int"); return nullptr; }
	if (b->is_negative()) { return div_int(a->negate(), b->negate()); }
	if (a->is_negative()) {
		auto res { div_int(a->negate(), b) };
		return res ? res->negate() : nullptr;
	}

	Obj *min { one };
	Obj *max { two };

	for (;;) {
		auto prod { mult(max, b) };
		if (is_true(is_equal_num(prod, a))) { return dynamic_cast<Integer *>(max); }
		if (is_false(less(prod, a))) { break; }
		max = mult(max, max);
	}

	for (;;) {
		auto diff { sub(max, min) };
		if (is_false(less(one, diff))) { break; }
		auto mid { half(add(max, min)) };
		auto prod { mult(mid, b) };
		if (is_true(less(prod, a))) { min = mid; }
		else if (is_true(less(a, prod))) { max = mid; }
		else { min = mid; break; }
	}
	return dynamic_cast<Integer *>(min);
}

Obj *remainder(Obj *a, Obj *b) {
	ASSERT(is_int(a) && is_int(b), "remainder");
	if (is_true(is_equal_num(b, one))) { return zero; }
	if (is_true(less(a, b))) { return a; }

	return sub(a, mult(div_int(dynamic_cast<Integer *>(a), dynamic_cast<Integer *>(b)), b));
}

Obj *gcd(Obj *a, Obj *b) {
	if (is_zero(b)) { return a; }
	return gcd(b, remainder(a, b));
}

Fraction *Fraction::create_forced(Integer *num, Integer *denom) {
	if (! num || ! denom) { err("fraction", "setup"); return nullptr; }
	if (is_negative(denom)) {
		return create_forced(num->negate(), denom->negate());
	}
	if (is_negative(num)) {
		return create_forced(num->negate(), denom)->negate();
	}

	auto g { dynamic_cast<Integer *>(gcd(num, denom)) };
	if (num && denom && g && is_false(is_equal_num(g, one))) {
		num = div_int(num, g);
		denom = div_int(denom, g);
		if (! num || ! denom) { err("fraction", "gcd"); return nullptr; }
	}
	return new Fraction { num, denom };
}

Obj *Fraction::create(Obj *num, Obj *denom) {
	if (is_negative(denom)) {
		return create(::negate(num), ::negate(denom));
	}
	if (is_negative(num)) {
		return ::negate(create(::negate(num), denom));
	}

	auto ni { dynamic_cast<Integer *>(num) };
	auto di { dynamic_cast<Integer *>(denom) };
	ASSERT(ni && di, "fraction");
	auto g { dynamic_cast<Integer *>(gcd(ni, di)) };
	if (g && is_false(is_equal_num(g, one))) {
		ni = div_int(ni, g);
		di = div_int(di, g);
		ASSERT(ni && di, "fraction");
	}
	if (is_true(is_equal_num(one, di))) {
		return ni;
	}
	return new Fraction { ni, di };
}

