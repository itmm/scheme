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

template<typename R>
class Single_Propagate {
	protected:
		virtual R apply_int(Integer *a) = 0;
		virtual R apply_fract(Fraction *a) = 0;
		virtual R apply_real(Float *a) = 0;
		virtual R apply_else(Obj *a) = 0;
	public:
		R propagate(Obj *a) {
			auto ai { dynamic_cast<Integer *>(a) };
			if (ai) { return apply_int(ai); }
			auto af { dynamic_cast<Fraction *>(a) };
			if (af) { return apply_fract(af); }
			auto ar { dynamic_cast<Float *>(a) };
			if (ar) { return apply_real(ar); }
			return apply_else(a);
		}
};

class Negate_Propagate : public Single_Propagate<Obj *> {
	protected:
		Obj *apply_int(Integer *a) override { return a->negate(); }
		Obj *apply_fract(Fraction *a) override { return a->negate(); }
		Obj *apply_real(Float *a) override { return new Float { - a->value() }; }
		Obj *apply_else(Obj *a) { ASSERT(false, "negate"); }
};

Obj *negate(Obj *a) { return Negate_Propagate{}.propagate(a); }

bool is_negative(Obj *a);

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
};

bool is_negative(Obj *a) { return Is_Negative_Propagate{}.propagate(a); }

bool is_zero(Obj *a);

class Is_Zero_Propagate : public Single_Bool_Propagate {
	protected:
		std::string name() override { return "is_zero"; }
		bool apply_int(Integer *a) override { return a->is_zero(); }
		bool apply_fract(Fraction *a) override { return is_zero(a->num()); }
		bool apply_real(Float *a) override { return a->value() == 0.0; }
};

bool is_zero(Obj *a) { return Is_Zero_Propagate{}.propagate(a); }

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
			return int_add(a, b);
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
			return int_sub(a, b);
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
			return int_mult(a, b);
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
			return to_bool(int_less(a, b));
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
			return to_bool(int_eq(a, b));
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
