/**
 * evaluate expressions
 * and call functions
 * also special forms are handled
 */

#pragma once

#include "type_base.h"
#include "frame.h"

class Function : public Obj {
	public:
		virtual Obj *apply(Obj *args) = 0;
};

constexpr auto as_function = Dynamic::as<Function>;
constexpr auto is_function = Dynamic::is<Function>;

class Primitive : public Function {
	public:
		std::ostream &write(std::ostream &out) override;

};

struct Procedure_Case {
	Procedure_Case(Obj *a, Obj *b): args { a }, body { b } { }
	Obj *args;
	Obj *body;
};

class Procedure : public Function {
		Frame *env_;
	protected:
		void propagate_mark() override {
			mark(env_);
			for (auto &c : cases_) {
				mark(c.args);
				mark(c.body);
			}
		}
	public:
		std::vector<Procedure_Case> cases_;
		Procedure(Frame *env): env_ { env } { }

		void add_case(Obj *args, Obj *body);

		Procedure(Obj *args, Obj *body, Frame *env): env_ { env }
		{
			add_case(args, body);
	       	}

		Frame *build_env(Procedure_Case &c, Obj *arg_values);
		Obj *get_body(Procedure_Case &c);

		Obj *apply(Obj *arg_values) override;
		std::ostream &write(std::ostream &out) override;
};

constexpr auto as_procedure = Dynamic::as<Procedure>;
constexpr auto is_procedure = Dynamic::is<Procedure>;

Obj *eval(Obj *exp, Frame *env);

extern std::vector<Frame *> active_frames;

Obj *apply(Obj *op, Obj *operands);

#include <functional>

void foreach_syntax_extension(std::function<void(Obj *)> fn);

void syntax_tests();
