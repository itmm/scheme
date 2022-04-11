/**
 * big integer type
 */

#include "num-types.h"
#include "err.h"

class Integer : public Exact_Numeric {
	public:
		using Digits = std::vector<unsigned short>;
	private:
		Digits digits_;

		void normalize();
		bool write_digit(unsigned digit, bool first, std::ostream &out);

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
		double float_value() const;
		bool is_zero() const { return digits_.empty(); }
		virtual bool is_negative() const { return false; }
		Integer *negate() const;
		std::ostream &write(std::ostream &out) override;
};

constexpr auto as_integer = Dynamic::as<Integer>;
constexpr auto is_integer = Dynamic::is<Integer>;

extern Integer *one;
extern Integer *two;
extern Integer *zero;

Integer *int_add(Integer *a, Integer *b);
Integer *int_sub(Integer *a, Integer *b);
Integer *int_mult(Integer *a, Integer *b);
bool int_less(Integer *a, Integer *b);
bool int_eq(Integer *a, Integer *b);
Integer *int_div(Integer *a, Integer *b);
Integer *remainder(Integer *a, Integer *b);
Integer *gcd(Integer *a, Integer *b);
