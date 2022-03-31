/**
 * numeric types
 */

#include "int.h"

using Float = Value_Element<double>;

class Fraction : public Obj {
		Integer *num_;
		Integer *denom_;
		Fraction(Integer *num, Integer *denom): num_ { num }, denom_ { denom } { }

	public:
		static Fraction *create_forced(Integer *num, Integer *denom);
		static Obj *create(Obj *num, Obj *denom);
		static Obj *create(const std::string &value);
		Integer *num() const { return num_; }
		Integer *denom() const { return denom_; }
		std::ostream &write(std::ostream &out) override {
			return out << num_ << '/' << denom_;
		}
		Fraction *negate() { return new Fraction { num_->negate(), denom_ }; }
};

Obj *negate(Obj *a);
bool is_zero(Obj *a);
bool is_negative(Obj *a);

bool is_fraction(const std::string &value) {
	return value.find('/') != std::string::npos;
}

bool is_real(const std::string &value) {
	return value.find('.') != std::string::npos;
}

double float_value(const std::string &v);

Obj *create_uncomplex(const std::string &value) {
	if (is_fraction(value)) {
		return Fraction::create(value);
	} else if (is_real(value)) {
		return new Float(float_value(value));
	} else {
		return Integer::create(value);
	}
}

std::string::size_type last_idx(std::string::size_type a, std::string::size_type b) {
	if (a == std::string::npos) {
		return b;
	} else if (b == std::string::npos) {
		return a;
	} else if (a < b) {
		return b;
	} else {
		return a;
	}
}

std::pair<Obj *, Obj *> create_complex_pair(const std::string &value) {
	auto last_i { last_idx(value.rfind('i'), value.rfind('I')) };

	if (last_i != value.size() - 1) {
		return { nullptr, nullptr };
	}

	auto last_op { last_idx(value.rfind('+'), value.rfind('-')) };
	if (last_op == std::string::npos || last_op == 0) {
		return { zero, create_uncomplex(value.substr(0, last_i)) };
	}

	return { 
		create_uncomplex(value.substr(0, last_op)),
		create_uncomplex(value.substr(last_op, last_i - last_op))
	};
}

class Exact_Complex : public Obj {
		Obj *real_;
		Obj *imag_;
		Exact_Complex(Obj *real, Obj *imag): real_ { real }, imag_ { imag } { }
	public:
		static Obj *create(Obj *real, Obj *imag);
		static Obj *create(const std::string &value);
		static Exact_Complex *create_forced(Obj *real, Obj *imag);
		Obj *real() const { return real_; }
		Obj *imag() const { return imag_; }
		std::ostream &write(std::ostream &out) override {
			if (! is_zero(real_)) {
				out << real_;
				if (! is_zero(imag_)) {
					out << (is_negative(imag_) ? "" : "+") << imag_ << "i";
				}
			} else if (! is_zero(imag_)) {
				out << imag_ << "i";
			} else { out << "0"; }
			return out;
		}
		Obj *negate() {
			return create(::negate(real_), ::negate(imag_));
		}
};

Obj *Exact_Complex::create(Obj *real, Obj *imag) {
	if (is_zero(imag)) { return real; }
	return create_forced(real, imag);
}

Obj *Exact_Complex::create(const std::string &value) {
	auto pair { create_complex_pair(value) };
	return create(pair.first, pair.second);
}

Exact_Complex *Exact_Complex::create_forced(Obj *real, Obj *imag) {
	return new Exact_Complex(real, imag);
}

#include <complex>

class Inexact_Complex : public Obj {
		using num_type = std::complex<double>;
		num_type value_;
		Inexact_Complex(const num_type &value): value_ { value } { }
	public:
		static Obj *create(const num_type &value);
		static Obj *create(const std::string &value);
		static Inexact_Complex *create_forced(const num_type &value);
		const num_type &value() const { return value_; }
		double real() const { return value_.real(); }
		double imag() const { return value_.imag(); }

		std::ostream &write(std::ostream &out) override {
			if (real()) {
				out << real();
				if (imag()) {
					out << (imag() < 0.0 ? "" : "+") << imag() << "i";
				}
			} else if (imag()) {
				out << imag() << "i";
			} else { out << "0"; }
			return out;
		}
		Obj *negate() {
			return create(-value_);
		}
};

Obj *Inexact_Complex::create(const num_type &value) {
	if (value.imag() == 0.0) {
		return new Float { value.real() };
	}
	return create_forced(value);
}

Obj *Inexact_Complex::create(const std::string &value) {
	auto pair { create_complex_pair(value) };
	auto real { dynamic_cast<Float *>(pair.first) };
	auto imag { dynamic_cast<Float *>(pair.second) };
	ASSERT(real && imag, "create complex");
	return create(num_type { real->value(), imag->value() });
}

Inexact_Complex *Inexact_Complex::create_forced(const num_type &value) {
	return new Inexact_Complex { value };
}

template<typename R>
class Single_Propagate {
	protected:
		virtual R apply_int(Integer *a) = 0;
		virtual R apply_fract(Fraction *a) = 0;
		virtual R apply_real(Float *a) = 0;
		virtual R apply_exact_complex(Exact_Complex *a) = 0;
		virtual R apply_inexact_complex(Inexact_Complex *a) = 0;
		virtual R apply_else(Obj *a) = 0;
	public:
		R propagate(Obj *a) {
			auto ai { dynamic_cast<Integer *>(a) };
			if (ai) { return apply_int(ai); }
			auto af { dynamic_cast<Fraction *>(a) };
			if (af) { return apply_fract(af); }
			auto ar { dynamic_cast<Float *>(a) };
			if (ar) { return apply_real(ar); }
			auto aec { dynamic_cast<Exact_Complex *>(a) };
			if (aec) { return apply_exact_complex(aec); }
			auto aic { dynamic_cast<Inexact_Complex *>(a) };
			if (aic) { return apply_inexact_complex(aic); }
			return apply_else(a);
		}
};

class Negate_Propagate : public Single_Propagate<Obj *> {
	protected:
		Obj *apply_int(Integer *a) override { return a->negate(); }
		Obj *apply_fract(Fraction *a) override { return a->negate(); }
		Obj *apply_real(Float *a) override { return new Float { - a->value() }; }
		Obj *apply_exact_complex(Exact_Complex *a) override { return a->negate(); }
		Obj *apply_inexact_complex(Inexact_Complex *a) override { return a->negate(); }
		Obj *apply_else(Obj *a) { ASSERT(false, "negate"); }
};

Obj *negate(Obj *a) { return Negate_Propagate{}.propagate(a); }


class Single_Bool_Propagate : public Single_Propagate<bool> {
	protected:
		virtual std::string name() = 0;
		bool apply_else(Obj *a) { err(name(), "no number", a); return false; }
};

class Is_Negative_Propagate : public Single_Bool_Propagate {
	protected:
		std::string name() override { return "is_negative"; }
		bool apply_int(Integer *a) override { return a->is_negative(); }
		bool apply_fract(Fraction *a) override { return is_negative(a->num()); }
		bool apply_real(Float *a) override { return a->value() < 0.0; }
		bool apply_exact_complex(Exact_Complex *a) override { 
			bool rn { is_negative(a->real()) };
			bool rz { is_zero(a->real()) };
			bool in { is_negative(a->imag()) };
			bool iz { is_zero(a->imag()) };
			return (rn && in) || (rn && iz) || (rz && in);
		}
		bool apply_inexact_complex(Inexact_Complex *a) override {
			return a->value().real() < 0.0;
	       	}
};

bool is_negative(Obj *a) { return Is_Negative_Propagate{}.propagate(a); }


class Is_Zero_Propagate : public Single_Bool_Propagate {
	protected:
		std::string name() override { return "is_zero"; }
		bool apply_int(Integer *a) override { return a->is_zero(); }
		bool apply_fract(Fraction *a) override { return is_zero(a->num()); }
		bool apply_real(Float *a) override { return a->value() == 0.0; }
		bool apply_exact_complex(Exact_Complex *a) override { return is_zero(a->real()) && is_zero(a->imag()); }
		bool apply_inexact_complex(Inexact_Complex *a) override { return a->value() == 0.0; }
};

bool is_zero(Obj *a) { return Is_Zero_Propagate{}.propagate(a); }

class Propagate {
	protected:
		virtual Obj *apply_int(Integer *a, Integer *b) = 0;
		virtual Obj *apply_fract(Fraction *a, Fraction *b) = 0;
		virtual Obj *apply_float(Float *a, Float *b) = 0;
		virtual Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) = 0;
		virtual Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) = 0;
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
	auto aec { dynamic_cast<Exact_Complex *>(a) };
	auto bec { dynamic_cast<Exact_Complex *>(b) };
	if (aec || bec) {
		if (! aec) {
			if (ai) { aec = Exact_Complex::create_forced(ai, zero); }
			else if (afr) { aec = Exact_Complex::create_forced(afr, zero); }
		}
		if (! bec) {
			if (bi) { bec = Exact_Complex::create_forced(bi, zero); }
			else if (bfr) { bec = Exact_Complex::create_forced(bfr, zero); }
		}
		if (aec && bec) { return apply_exact_complex(aec, bec); }
	}
	auto aic { dynamic_cast<Inexact_Complex *>(a) };
	auto bic { dynamic_cast<Inexact_Complex *>(b) };
	if (aic || bic) {
		if (! aic) {
			if (ai) { aic = Inexact_Complex::create_forced({ ai->float_value(), 0.0 }); }
			else if (afl) { aic = Inexact_Complex::create_forced({ afl->value(), 0.0 }); }
		}
		if (! bic) {
			if (bi) { bic = Inexact_Complex::create_forced({ bi->float_value(), 0.0 }); }
			else if (bfl) { bic = Inexact_Complex::create_forced({ bfl->value(), 0.0 }); }
		}
		if (aic && bic) { return apply_inexact_complex(aic, bic); }
	}
	return err("propagate", "can't propagate", a, b);
}

Obj *add(Obj *a, Obj *b);
Obj *mult(Obj *a, Obj *b);
Obj *sub(Obj *a, Obj *b);
Obj *div(Obj *a, Obj *b);

class Add_Propagate : public Propagate {
	protected:
		Obj *apply_int(Integer *a, Integer *b) override {
			return int_add(a, b);
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return Fraction::create(
				add(mult(a->num(), b->denom()), mult(b->num(), a->denom())),
				mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) override {
			return new Float { a->value() + b->value() };
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			return Exact_Complex::create(
				add(a->real(), b->real()),
				add(a->imag(), b->imag())
			);
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			return Inexact_Complex::create(a->value() + b->value());
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
		Obj *apply_int(Integer *a, Integer *b) override {
			return int_sub(a, b);
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return Fraction::create(
				sub(mult(a->num(), b->denom()), mult(b->num(), a->denom())),
				mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) override {
			return new Float { a->value() - b->value() };
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			return Exact_Complex::create(
				sub(a->real(), b->real()),
				sub(a->imag(), b->imag())
			);
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			return Inexact_Complex::create(a->value() - b->value());
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
		Obj *apply_int(Integer *a, Integer *b) override {
			return int_mult(a, b);
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return Fraction::create(
				mult(a->num(), b->num()), mult(a->denom(), b->denom())
			);
		}
		Obj *apply_float(Float *a, Float *b) override{
			return new Float { a->value() * b->value() };
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			return Exact_Complex::create(
				sub(mult(a->real(), b->real()), mult(a->imag(), b->imag())),
				add(mult(a->real(), b->imag()), mult(a->imag(), b->real()))
			);
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			return Inexact_Complex::create(a->value() * b->value());
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
		Obj *apply_int(Integer *a, Integer *b) override {
			return Fraction::create(a, b);
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return Fraction::create(
				mult(a->num(), b->denom()), mult(a->denom(), b->num())
			);
		}
		Obj *apply_float(Float *a, Float *b) override {
			return new Float { a->value() / b->value() };
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			auto t1 { mult(a->real(), b->real()) };
			auto t2 { mult(a->imag(), b->imag()) };
			auto denum { sub(mult(b->real(), b->real()), mult(b->imag(), b->imag())) };
			return Exact_Complex::create(
				div(add(t1, t2), denum),
				div(sub(t1, t2), denum)
			);
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			return Inexact_Complex::create(a->value() / b->value());
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
		Obj *apply_int(Integer *a, Integer *b) override {
			return to_bool(int_less(a, b));
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return less(mult(a->num(), b->denom()), mult(b->num(), a->denom()));
		}
		Obj *apply_float(Float *a, Float *b) override {
			return to_bool(a->value() < b->value());
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			ASSERT(false, "complex less");
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			ASSERT(false, "complex less");
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
		Obj *apply_int(Integer *a, Integer *b) override {
			return to_bool(int_eq(a, b));
		}
		Obj *apply_fract(Fraction *a, Fraction *b) override {
			return is_equal_num(mult(a->num(), b->denom()), mult(b->num(), a->denom()));
		}
		Obj *apply_float(Float *a, Float *b) {
			return to_bool(a->value() == b->value());
		}
		Obj *apply_exact_complex(Exact_Complex *a, Exact_Complex *b) override {
			return to_bool(
				is_true(is_equal_num(a->real(), b->real())) &&
				is_true(is_equal_num(a->imag(), b->imag()))
			);
		}
		Obj *apply_inexact_complex(Inexact_Complex *a, Inexact_Complex *b) override {
			return to_bool(
				a->real() == b->real() && a->imag() == b->imag()
			);
		}
};

Obj *is_equal_num(Obj *a, Obj *b) {
	if (is_zero(a) && is_zero(b)) { return to_bool(true); }
	if (is_negative(a) != is_negative(b)) { return to_bool(false); }

	return Equal_Propagate{}.propagate(a, b);

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
		num = int_div(num, g);
		denom = int_div(denom, g);
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
		ni = int_div(ni, g);
		di = int_div(di, g);
		ASSERT(ni && di, "fraction");
	}
	if (is_true(is_equal_num(one, di))) {
		return ni;
	}
	return new Fraction { ni, di };
}

#include <sstream>

Obj *Fraction::create(const std::string &value) {
	std::string num;
	std::string denom;
	std::istringstream in { value };
	ASSERT(std::getline(in, num, '/'), "fraction create num");
	ASSERT(std::getline(in, denom), "fraction create denom");
	return create(Integer::create(num), Integer::create(denom));
}
