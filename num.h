/**
 * numeric types
 */

#pragma once

#include "num_types.h"
#include "value.h"
#include "int.h"

using Float = Value_Element<double, Inexact_Numeric>;

constexpr auto as_float = Dynamic::as<Float>;
constexpr auto is_float = Dynamic::is<Float>;

class Fraction : public Exact_Numeric {
		Integer *num_;
		Integer *denom_;
		Fraction(Integer *num, Integer *denom): num_ { num }, denom_ { denom } { }

	public:
		static Fraction *create_forced(Integer *num, Integer *denom);
		static Obj *create(Obj *num, Obj *denom);
		static Obj *create(const std::string &value);
		Integer *num() const { return num_; }
		Integer *denom() const { return denom_; }
		std::ostream &write(std::ostream &out) override;
		Fraction *negate() { return new Fraction { num_->negate(), denom_ }; }
};

constexpr auto as_fraction = Dynamic::as<Fraction>;
constexpr auto is_fraction = Dynamic::is<Fraction>;

Obj *negate(Obj *a);
bool is_zero(Obj *a);
bool is_negative(Obj *a);

class Exact_Complex : public Exact_Numeric, public Complex_Numeric {
		Obj *real_;
		Obj *imag_;
		Exact_Complex(Obj *real, Obj *imag): real_ { real }, imag_ { imag } { }
	public:
		static Obj *create(Obj *real, Obj *imag);
		static Obj *create(const std::string &value);
		static Exact_Complex *create_forced(Obj *real, Obj *imag);
		Obj *real() const { return real_; }
		Obj *imag() const { return imag_; }
		std::ostream &write(std::ostream &out) override;
		Obj *negate() { return create(::negate(real_), ::negate(imag_)); }
};

constexpr auto as_exact_complex = Dynamic::as<Exact_Complex>;
constexpr auto is_exact_complex = Dynamic::is<Exact_Complex>;

#include <complex>

class Inexact_Complex : public Inexact_Numeric, public Complex_Numeric {
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

		std::ostream &write(std::ostream &out) override;
		Obj *negate() { return create(-value_); }
};

constexpr auto as_inexact_complex = Dynamic::as<Inexact_Complex>;
constexpr auto is_inexact_complex = Dynamic::is<Inexact_Complex>;

Obj *add(Obj *a, Obj *b);
Obj *mult(Obj *a, Obj *b);
Obj *sub(Obj *a, Obj *b);
Obj *div(Obj *a, Obj *b);

Obj *to_bool(bool cond);
bool is_true(Obj *value);
bool is_false(Obj *value);

Obj *less(Obj *a, Obj *b);

Obj *is_equal_num(Obj *a, Obj *b);

