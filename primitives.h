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

bool is_symbol(Obj *obj) {
	return dynamic_cast<Symbol *>(obj) != nullptr;
}

class Symbol_Primitive : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override { 
			return to_bool(is_symbol(arg));
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

class Apply_Primitive: public Primitive {
		Obj *build_arg_lst(Obj *args) {
			ASSERT(is_pair(args), "apply");
			if (! cdr(args)) {
				ASSERT(! car(args) || is_pair(car(args)), "apply");
				return car(args);
			} else {
				return new Pair {
					car(args),
					build_arg_lst(cdr(args))
				};
			}
		}
	public:
		Obj *apply(Obj *args) override {
			ASSERT(is_pair(args), "apply");
			auto proc { car(args) };
			ASSERT(dynamic_cast<Procedure *>(proc) || dynamic_cast<Primitive *>(proc), "apply");
			auto lst { build_arg_lst(cdr(args)) };
			ASSERT(is_pair(lst), "apply");
			return ::apply(proc, lst);
		}
};

bool is_numeric(Obj *arg) {
	return dynamic_cast<Numeric *>(arg) != nullptr;
}

class Numeric_Primitive : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override { 
			return to_bool(is_numeric(arg));
		}
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

class Eqv_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			if (first == second) { return to_bool(true); }
			if (is_true(is_equal_num(first, second))) {
				return to_bool(true);
			}
			auto as { dynamic_cast<String *>(first) };
			auto bs { dynamic_cast<String *>(second) };
			if (as && bs && as->value() == bs->value()) {
				return to_bool(true);
			}
			return to_bool(false);
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

class Set_Car_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			auto pair { dynamic_cast<Pair *>(first) };
			ASSERT(pair, "set-car!");
			pair->set_head(second);
			return second;
		}
};

class Set_Cdr_Primitive : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			auto pair { dynamic_cast<Pair *>(first) };
			ASSERT(pair, "set-cdr!");
			pair->set_rest(second);
			return second;
		}
};

Frame initial_frame { nullptr };

void setup_primitives() {
	initial_frame.insert("symbol?", new Symbol_Primitive());
	initial_frame.insert("car", new Car_Primitive());
	initial_frame.insert("cdr", new Cdr_Primitive());
	initial_frame.insert("cons", new Cons_Primitive());
	initial_frame.insert("@binary+", new Add_Primitive());
	initial_frame.insert("@binary-", new Sub_Primitive());
	initial_frame.insert("@binary*", new Mul_Primitive());
	initial_frame.insert("@binary/", new Div_Primitive());
	initial_frame.insert("@negate", new Negate_Primitive());
	initial_frame.insert("@negative?", new Is_Negative_Primitive());
	initial_frame.insert("<", new Less_Primitive());
	initial_frame.insert("=", new Equal_Primitive());
	initial_frame.insert("apply", new Apply_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());
	initial_frame.insert("eq?", new Eq_Primitive());
	initial_frame.insert("eqv?", new Eqv_Primitive());
	initial_frame.insert("remainder", new Remainder_Primitive());
	initial_frame.insert("newline", new Newline_Primitive());
	initial_frame.insert("print", new Print_Primitive());
	initial_frame.insert("set-car!", new Set_Car_Primitive());
	initial_frame.insert("set-cdr!", new Set_Cdr_Primitive());
	initial_frame.insert("numeric?", new Numeric_Primitive());

}
