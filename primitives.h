/**
 * primitive functions
 */

class One_Primitive : public Primitive {
	protected:
		virtual Element *apply_one(Element *arg) = 0;
	public:
		Element *apply(Element *args) override {
			ASSERT(is_pair(args), "one primitive");
			ASSERT(is_null(cdr(args)), "one primitive");
			return apply_one(car(args));
		}
};

class Car_Primitive : public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override { return car(arg); }
};


class Cdr_Primitive : public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override { return cdr(arg); }
};

class List_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override;
};

Element *List_Primitive::apply(Element *args) {
	return args;
}

class Two_Primitive : public Primitive {
	protected:
		virtual Element *apply_two(Element *first, Element *second) = 0;
	public:
		Element *apply(Element *args) override {
			ASSERT(is_pair(args), "two primitive");
			auto nxt { cdr(args) };
			ASSERT(is_pair(nxt), "two primitive");
			ASSERT(is_null(cdr(nxt)), "two primitive");
			return apply_two(car(args), car(nxt));
		}
};

class Cons_Primitive : public Two_Primitive {
	protected:
		Element *apply_two(Element *first, Element *second) override {
			return new Pair { first, second };
		}
};

class Null_Primitive: public One_Primitive {
	protected:
		Element *apply_one(Element *arg) override {
			return to_bool(is_null(arg));
		}
};

class Apply_Primitive: public Two_Primitive {
	protected:
		Element *apply_two(Element *first, Element *second) override {
			return ::apply(first, second);
		}
};

class Numeric_Primitive : public Two_Primitive {
	protected:
		Element *apply_two(Element *a, Element *b) override;
		virtual Element *do_int(const Integer &a, const Integer &b) = 0;
		virtual Element *do_float(double a, double b) = 0;
};

Element *Numeric_Primitive::apply_two(Element *a, Element *b) {
	auto a_i { dynamic_cast<Integer *>(a) };
	auto b_i { dynamic_cast<Integer *>(b) };
	if (a_i && b_i) { return do_int(*a_i, *b_i); }
	Float *a_f { nullptr }; Float *b_f { nullptr };
	if (! a_i) { a_f = dynamic_cast<Float *>(a); }
	if (! b_i) { b_f = dynamic_cast<Float *>(b); }
	if ((a_i || a_f) && (b_i || b_f)) {
		return do_float(
			a_f ? a_f->value() : a_i->float_value(),
			b_f ? b_f->value() : b_i->float_value()
		);
	}
	return err("numeric", "invalid arguments", new Pair { a, b });
}

class Add_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a + b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a + b };
		}
};

class Sub_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a - b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a - b };
		}
};

class Mul_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a * b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a * b };
		}
};

class Div_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return a / b;
		}
		Element *do_float(double a, double b) override {
			return new Float { a / b };
		}
};

class Less_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return to_bool(a < b);
		}
		Element *do_float(double a, double b) override {
			return to_bool(a < b);
		}
};

class Equal_Primitive : public Numeric_Primitive {
	protected:
		Element *do_int(const Integer &a, const Integer &b) override {
			return to_bool(a == b);
		}
		Element *do_float(double a, double b) override {
			return to_bool(a == b);
		}
};

class Garbage_Collect_Primitive : public Primitive {
	public:
		Element *apply(Element *args) override {
			auto result { Element::garbage_collect() };
			return new Pair {
				new Symbol { "collected" },
				new Pair {
					new Integer { result.first },
					new Pair {
						new Symbol { "kept" },
						new Pair {
							new Integer { result.second },
							nullptr
						}
					}
				}
			};
		}
};

Frame initial_frame { nullptr };

void setup_primitives() {
	initial_frame.insert("car", new Car_Primitive());
	initial_frame.insert("cdr", new Cdr_Primitive());
	initial_frame.insert("list", new List_Primitive());
	initial_frame.insert("cons", new Cons_Primitive());
	initial_frame.insert("#binary+", new Add_Primitive());
	initial_frame.insert("#binary-", new Sub_Primitive());
	initial_frame.insert("#binary*", new Mul_Primitive());
	initial_frame.insert("#binary/", new Div_Primitive());
	initial_frame.insert("<", new Less_Primitive());
	initial_frame.insert("=", new Equal_Primitive());
	initial_frame.insert("null?", new Null_Primitive());
	initial_frame.insert("apply", new Apply_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());
}
