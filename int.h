/**
 * big integer type
 */

class Integer : public Exact_Numeric {
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
		static Integer *create(const std::string &digits);
		static Integer *create(unsigned value);
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

constexpr auto as_integer = Dynamic::as<Integer>;
constexpr auto is_integer = Dynamic::is<Integer>;

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

Integer *Integer::create(const std::string &digits) {
	Integer::Digits result;
	bool negative { false };
	unsigned short v { 0 };
	unsigned short mult { 1 };
	for (auto it { digits.rbegin() }; it != digits.rend(); ++it) {
		if (*it == '+') { continue; }
		if (*it == '-') { negative = ! negative; continue; }
		int digit { *it - '0' };
		if (digit < 0 || digit > 9) {
			err("integer", "invalid digits", new String { digits });
			return nullptr;
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

Integer *Integer::create(unsigned value) {
	Integer::Digits result;
	for (; value; value /= 10000) {
		result.push_back(value % 10000);
	}
	return new Integer { std::move(result) };
}

auto one { Integer::create(1) };
auto two { Integer::create(2) };
auto zero { Integer::create(0) };

Integer *int_add(Integer *a, Integer *b) {
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

Integer *int_sub(Integer *a, Integer *b) {
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

Integer *int_mult(Integer *a, Integer *b) {
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

bool int_less(Integer *a, Integer *b) {
	if (a->digits().size() > b->digits().size()) { return false; }
	if (a->digits().size() < b->digits().size()) { return true; }

	auto a_i { a->digits().rbegin() };
	auto b_i { b->digits().rbegin() };
	for (; a_i != a->digits().rend(); ++a_i, ++b_i) {
		if (*a_i < *b_i) { return true; }
		if (*a_i > *b_i) { return false; }
	}
	return false;
}

bool int_eq(Integer *a, Integer *b) {
	if (a->digits().size() != b->digits().size()) {
		return false;
	}
	auto a_i { a->digits().begin() };
	auto b_i { b->digits().begin() };
	for (; a_i != a->digits().end(); ++a_i, ++b_i) {
		if (*a_i != *b_i) { return false; }
	}
	return true;
}

static Integer *int_half(Integer *num) {
	Integer::Digits result;
	result.resize(num->digits().size());
	int carry { 0 };
	for (int i { static_cast<int>(num->digits().size()) - 1}; i >= 0; --i) {
		int v { carry + num->digits()[i] };
		if (v % 2) { carry = 10000; v -= 1; } else { carry = 0; }
		result[i] = v/2;
	}
	return new Integer { std::move(result) };
}

Integer *int_div(Integer *a, Integer *b) {
	if (! a || ! b) { err("int_div", "no int"); return nullptr; }
	if (b->is_negative()) { return int_div(a->negate(), b->negate()); }
	if (a->is_negative()) {
		auto res { int_div(a->negate(), b) };
		return res ? res->negate() : nullptr;
	}

	Integer *min { one };
	Integer *max { two };

	for (;;) {
		auto prod { int_mult(max, b) };
		if (int_eq(prod, a)) { return as_integer(max); }
		if (! int_less(prod, a)) { break; }
		max = int_mult(max, max);
	}

	for (;;) {
		auto diff { int_sub(max, min) };
		if (! int_less(one, diff)) { break; }
		auto mid { int_half(int_add(max, min)) };
		auto prod { int_mult(mid, b) };
		if (int_less(prod, a)) { min = mid; }
		else if (int_less(a, prod)) { max = mid; }
		else { min = mid; break; }
	}
	return min;
}

Integer *remainder(Integer *a, Integer *b) {
	if (int_eq(b, one)) { return zero; }
	if (int_less(a, b)) { return a; }

	return int_sub(a, int_mult(int_div(a, b), b));
}

Integer *gcd(Integer *a, Integer *b) {
	if (b->is_zero()) { return a; }
	return gcd(b, remainder(a, b));
}
