/**
 * primitive functions
 */

#include "primitives.h"
#include "eval.h"
#include "string.h"

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

class To_Float: public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override {
			auto i { as_integer(arg) };
			return i ? new Float { i->float_value() } : arg;
		}
};

template<Obj *(FN)(Obj *)> class One_Primitive_Fn : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override {
			return FN(arg);
		}
};

template<bool (FN)(Obj *)> class Predicate_Fn : public One_Primitive {
	protected:
		Obj *apply_one(Obj *arg) override {
			return to_bool(FN(arg));
		}
};

template<typename C> class Dynamic_Predicate : public Predicate_Fn<Dynamic::is<C>> { };

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

template<Obj *(FN)(Obj *, Obj *)> class Two_Primitive_Fn : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return FN(first, second);
		}
};

template<bool (FN)(Obj *, Obj *)> class Binary_Predicate_Fn : public Two_Primitive {
	protected:
		Obj *apply_two(Obj *first, Obj *second) override {
			return to_bool(FN(first, second));
		}
};

class Apply_Primitive: public Primitive {
		Obj *build_arg_lst(Obj *args) {
			ASSERT(is_pair(args), "apply");
			if (! cdr(args)) {
				ASSERT(! car(args) || is_pair(car(args)), "apply");
				return car(args);
			} else {
				return cons(
					car(args),
					build_arg_lst(cdr(args))
				);
			}
		}
	public:
		Obj *apply(Obj *args) override {
			ASSERT(is_pair(args), "apply");
			auto proc { car(args) };
			ASSERT(is_function(proc), "apply");
			auto lst { build_arg_lst(cdr(args)) };
			ASSERT(is_pair(lst), "apply");
			return ::apply(proc, lst);
		}
};

Obj *remainder(Obj *first, Obj *second) {
	auto a { as_integer(first) };
	auto b { as_integer(second) };
	ASSERT(a && b, "remainder");
	return remainder(a, b);
}

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
			return build_list(
				Symbol::get("collected"),
			       	Integer::create(result.first),
				Symbol::get("kept"),
				Integer::create(result.second)
			);
		}
};

inline bool eq(Obj *first, Obj *second) { return first == second; }
inline bool eqv(Obj *first, Obj *second) {
	if (eq(first, second)) { return true; }
	if (::is_true(is_equal_num(first, second))) { return true; }
	auto as { as_string(first) };
	auto bs { as_string(second) };
	if (as && bs && as->value() == bs->value()) { return true; }
	return false;
}

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

Obj *set_car(Obj *pair, Obj *new_car) {
	auto p { as_pair(pair) };
	ASSERT(p, "set-car!");
	p->set_head(new_car);
	return new_car;
}

Obj *set_cdr(Obj *pair, Obj *new_cdr) {
	auto p { as_pair(pair) };
	ASSERT(p, "set-cdr!");
	p->set_rest(new_cdr);
	return new_cdr;
}

Frame initial_frame { nullptr };

void setup_primitives() {
	initial_frame.insert("symbol?", new Dynamic_Predicate<Symbol>());
	initial_frame.insert("numeric?", new Dynamic_Predicate<Numeric>());
	initial_frame.insert("complex?", new Dynamic_Predicate<Complex_Numeric>());
	initial_frame.insert("pair?", new Dynamic_Predicate<Pair>());
	initial_frame.insert("car", new One_Primitive_Fn<car>());
	initial_frame.insert("cdr", new One_Primitive_Fn<cdr>());
	initial_frame.insert("cons", new Two_Primitive_Fn<cons>());
	initial_frame.insert("@binary+", new Two_Primitive_Fn<add>());
	initial_frame.insert("@binary-", new Two_Primitive_Fn<sub>());
	initial_frame.insert("@binary*", new Two_Primitive_Fn<mult>());
	initial_frame.insert("@binary/", new Two_Primitive_Fn<div>());
	initial_frame.insert("@negate", new One_Primitive_Fn<negate>());
	initial_frame.insert("@negative?", new Predicate_Fn<is_negative>());
	initial_frame.insert("@binary<", new Two_Primitive_Fn<less>());
	initial_frame.insert("@binary=", new Two_Primitive_Fn<is_equal_num>());
	initial_frame.insert("apply", new Apply_Primitive());
	initial_frame.insert("garbage-collect", new Garbage_Collect_Primitive());
	initial_frame.insert("@binary-eq?", new Binary_Predicate_Fn<eq>());
	initial_frame.insert("@binary-eqv?", new Binary_Predicate_Fn<eqv>());
	initial_frame.insert("remainder", new Two_Primitive_Fn<remainder>());
	initial_frame.insert("newline", new Newline_Primitive());
	initial_frame.insert("print", new Print_Primitive());
	initial_frame.insert("set-car!", new Two_Primitive_Fn<set_car>());
	initial_frame.insert("set-cdr!", new Two_Primitive_Fn<set_cdr>());
	initial_frame.insert("int->float", new To_Float());

}
