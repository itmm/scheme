/**
 * primitive functions
 */

class One_Primitive : public Primitive {
	protected:
		virtual Obj *apply_one(Obj *arg) = 0;
	public:
		Obj *apply(Obj *args) override {
			ASSERT(is_pair(args), "one primitive");
			ASSERT(is_null(cdr(args)), "one primitive");
			return apply_one(car(args));
		}
};

class Car_Primitive : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override { return car(arg); }
};


class Cdr_Primitive : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override { return cdr(arg); }
};

class Null_Primitive : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override { return to_bool(is_null(arg)); }
};

class Two_Primitive : public Primitive {
	protected:
		virtual Obj *apply_two(Obj *first, Obj *second) = 0;
	public:
		Obj *apply(Obj *args) override {
			ASSERT(is_pair(args), "two primitive");
			auto nxt { cdr(args) };
			ASSERT(is_pair(nxt), "two primitive");
			ASSERT(is_null(cdr(nxt)), "two primitive");
			return apply_two(car(args), car(nxt));
		}
};

class Cons_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return new Pair { first, second };
		}
};

class Apply_Primitive: public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return ::apply(first, second);
		}
};

class Numeric_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *a, Obj *b) override;
		virtual Obj *do_int(Integer &a, Integer &b) = 0;
		virtual Obj *do_float(double a, double b) = 0;
		virtual Obj *do_fract(Fraction &a, Fraction &b) = 0;
};

class Add_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return add(first, second);
		}
};

class Sub_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return sub(first, second);
		}
};

class Mul_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return mult(first, second);
		}
};

class Div_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return div(first, second);
		}
};

class Remainder_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			auto a { dynamic_cast<Integer *>(first) };
			auto b { dynamic_cast<Integer *>(second) };
			ASSERT(a && b, "remainder");
			return remainder(a, b);
		}
};

class Less_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return less(first, second);
		}
};

class Equal_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return is_equal_num(first, second);
		}
};

class Zero_Primitive : public Primitive {
	protected:
		virtual Obj *apply_zero() = 0;
	public:
		Obj *apply(Obj *args) override {
			ASSERT(is_null(args), "zero primitive");
			return apply_zero();
		}
};

class Garbage_Collect_Primitive : public Zero_Primitive {
	protected:
		Obj *apply_zero() override {
			auto result { Obj::garbage_collect() };
			return new Pair {
				Symbol::get("collected"),
				new Pair {
					Integer::create(result.first),
					new Pair {
						Symbol::get("kept"),
						new Pair {
							Integer::create(result.second),
							nullptr
						}
					}
				}
			};
		}
};

class Eq_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return to_bool(first == second);
		}
};

std::ostream *out { &std::cout };

class Newline_Primitive : public Zero_Primitive {
	protected:
		Obj *apply_zero() override {
			if (out) { *out << '\n'; }
			return nullptr;
		}
};

class Print_Primitive : public Primitive {
	public:
		Obj *apply(Obj *args) override {
			if (out) {
				bool first { true };
				for (; ! is_null(args); args = cdr(args)) {
					if (first) { first = false; } else { *out << ' '; }
					*out << car(args);
				}
			}
			return nullptr;
		}
};

class Negate_Primitive: public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override {
			return negate(arg);
		}
};

class Is_Negative_Primitive: public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override {
			return to_bool(is_negative(arg));
		}
};

Frame initial_frame { nullptr };

void setup_primitives() {
	initial_frame.insert("car", new Car_Primitive());
	initial_frame.insert("cdr", new Cdr_Primitive());
	initial_frame.insert("cons", new Cons_Primitive());
	initial_frame.insert("null?", new Null_Primitive());
	initial_frame.insert("#binary+", new Add_Primitive());
	initial_frame.insert("#binary-", new Sub_Primitive());
	initial_frame.insert("#binary*", new Mul_Primitive());
	initial_frame.insert("#binary/", new Div_Primitive());
	initial_frame.insert("#negate", new Negate_Primitive());
	initial_frame.insert("#negative?", new Is_Negative_Primitive());
	initial_frame.insert("<", new Less_Primitive());
	initial_frame.insert("=", new Equal_Primitive());
	initial_frame.insert("apply", new Apply_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());
	initial_frame.insert("eq?", new Eq_Primitive());
	initial_frame.insert("remainder", new Remainder_Primitive());
	initial_frame.insert("newline", new Newline_Primitive());
	initial_frame.insert("print", new Print_Primitive());
}
