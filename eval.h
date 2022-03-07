/**
 * evaluate expressions
 * and call functions
 * also special forms are handled
 */

class Primitive : public Element {
	public:
		virtual Element *apply(Element *args) = 0;
		std::ostream &write(std::ostream &out) const override;

};

std::ostream &Primitive::write(std::ostream &out) const {
	return out << "#primitive";
}

class Procedure : public Element {
		Element *args_;
		Element *body_;
		Frame *env_;
	protected:
		void propagate_mark() override {
			mark(args_); mark(body_); mark(env_);
		}
	public:
		Procedure(Element *args, Element *body, Frame *env):
			args_ { args }, body_ { body }, env_ { env }
		{ }

		Element *apply(Element *arg_values);
		std::ostream &write(std::ostream &out) const override;
};

std::ostream &Procedure::write(std::ostream &out) const {
	out << "(lambda " << args_;
	for (Element *cur { body_ }; cur; cur = cdr(cur)) {
		auto v { car(cur) };
		out << ' ' << v;
	}
	out << ')';
	return out;
}

Symbol *assert_sym(Element *elm) {
	auto sym { dynamic_cast<Symbol *>(elm) };
	if (! sym) { err("assert_sym", "no symbol", elm); }
	return sym;
}

Element *eval(Element *exp, Frame *env);

std::vector<Frame *> active_frames;

class Frame_Guard {
	public:
		Frame_Guard(Frame *frame) { active_frames.push_back(frame); }
		~Frame_Guard() { active_frames.pop_back(); }
};

Element *Procedure::apply(Element *arg_values) {
	auto new_env { new Frame { env_ } };
	Frame_Guard fg { new_env };

	Element *cur { args_ };
	ASSERT(is_good(cur), "procedure");
	for (; is_pair(cur); cur = cdr(cur)) {
		auto sym { assert_sym(car(cur)) };
		ASSERT(sym, "procedure");
		auto value { car(arg_values) };
		ASSERT(is_good(value), "procedure");
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	ASSERT(is_good(cur), "procedure");
	if (cur) {
		auto sym { assert_sym(cur) };
		ASSERT(sym, "procedure");
		ASSERT(is_good(arg_values), "procedure");
		new_env->insert(sym->value(), arg_values);
	}

	cur = body_;
	Element *value { nullptr };
	for (; is_pair(cur); cur = cdr(cur)) {
		Element *statement { car(cur) };
		ASSERT(is_good(statement), "procedure");
		value = eval(statement, new_env);
	}
	ASSERT(is_null(cur), "procedure");
	return value;
}

Element *apply(Element *op, Element *operands) {
	auto prim { dynamic_cast<Primitive *>(op) };
	if (prim) { return prim->apply(operands); }
	auto proc { dynamic_cast<Procedure *>(op) };
	if (proc) { return proc->apply(operands); }
	return err("apply", "unknown operation", op);
}

Element *eval_list(Element *exp, Frame *env) {
	if (is_err(exp) || ! exp) { return exp; }
	auto head { eval(car(exp), env) };
	auto rest { dynamic_cast<Pair *>(cdr(exp)) };
	if (rest) {
		return new Pair { head, eval_list(rest, env) };
	} else {
		return new Pair { head, eval(cdr(exp), env) };
	}
}

bool is_tagged_list(Pair *lst, const std::string &tag) {
	auto sym { dynamic_cast<Symbol *>(car(lst)) };
	return sym && sym->value() == tag;
}

inline bool is_define_special(Pair *lst) {
	return is_tagged_list(lst, "define");
}

inline Element *define_key(Pair *lst) {
	auto first { cadr(lst) };
	auto sym { dynamic_cast<Symbol *>(first) };
	if (sym) { return sym; }
	auto args { dynamic_cast<Pair *>(first) };
	if (args) {
		auto name { dynamic_cast<Symbol *>(car(args)) };
		ASSERT(name, "define_key");
		return name;
	}
	ASSERT(false, "define key");
}

inline Element *define_value(Pair *lst, Frame *env) {
	auto args { dynamic_cast<Pair *>(cadr(lst)) };
	if (args) {
		return new Procedure { cdr(args), cddr(lst), env };
	} else {
		ASSERT(is_null(cdddr(lst)), "define");
		return eval(caddr(lst), env);
	}
}

inline bool is_lambda_special(Pair *lst) {
	return is_tagged_list(lst, "lambda");
}

inline Element *lambda_args(Pair *lst) {
	return cadr(lst);
}

inline Element *lambda_body(Pair *lst) {
	return cddr(lst);
}

inline bool is_if_special(Pair *lst) {
	return is_tagged_list(lst, "if");
}

inline Element *if_condition(Pair *lst) {
	return cadr(lst);
}

inline Element *if_consequence(Pair *lst) {
	return caddr(lst);
}

inline Element *if_alternative(Pair *lst) {
	return cadddr(lst);
}

inline bool is_cond_special(Pair *lst) {
	return is_tagged_list(lst, "cond");
}

Element *build_cond(Element *lst) {
	if (is_err(lst) || is_null(lst)) { return lst; }
	auto expr { car(lst) };
	auto cond { car(expr) };
	auto cons { cdr(expr) };
	ASSERT(is_good(cond) && is_good(cons), "cond");
	auto sym { dynamic_cast<Symbol *>(cond) };
	if (sym && sym->value() == "else") {
		if (cdr(lst)) {
			return err("cond", "else not in last case");
		}
		return new Pair { Symbol::get("begin"), cons };
	}
	return new Pair {
		Symbol::get("if"),
		new Pair {
			cond,
			new Pair {
				new Pair { Symbol::get("begin"), cons },
				new Pair { build_cond(cdr(lst)), nullptr }
			}
		}
	};
}

inline bool is_begin_special(Pair *lst) {
	return is_tagged_list(lst, "begin");
}

inline bool is_and_special(Pair *lst) {
	return is_tagged_list(lst, "and");
}

inline bool is_or_special(Pair *lst) {
	return is_tagged_list(lst, "or");
}

inline bool is_quote_special(Pair *lst) {
	return is_tagged_list(lst, "quote");
}

inline bool is_let_special(Pair *lst) {
	return is_tagged_list(lst, "let");
}

Element *build_let_args(Element *arg_vals, Element *args) {
	if (is_null(arg_vals)) { return args; }
	args = build_let_args(cdr(arg_vals), args);
	return new Pair { car(car(arg_vals)), args };
}

Element *build_let_vals(Element *arg_vals, Element *vals) {
	if (is_null(arg_vals)) { return vals; }
	vals = build_let_vals(cdr(arg_vals), vals);
	return new Pair { cadr(car(arg_vals)), vals };
}

Element *build_let(Element *lst) {
	auto arg_vals { cadr(lst) };
	auto block { cddr(lst) };
	ASSERT(is_good(arg_vals) && is_good(block), "let");
	auto args { build_let_args(arg_vals, nullptr) };
	auto vals { build_let_vals(arg_vals, nullptr) };
	return new Pair {
		new Pair {
			Symbol::get("lambda"),
			new Pair { args, block }
		},
		vals
	};
}

bool is_set_special(Pair *lst) {
	return is_tagged_list(lst, "set!");
}

Element *set_var(Element *lst) { return cadr(lst); }
Element *set_value(Element *lst) { return caddr(lst); }

bool is_valid_set(Pair *lst) {
	return assert_sym(set_var(lst)) && is_good(set_value(lst)) &&
		is_null(cdddr(lst));
}

Element *eval(Element *exp, Frame *env) {
	if (is_err(exp) || ! exp) { return exp; }
	auto int_value { dynamic_cast<Integer *>(exp) };
	if (int_value) { return int_value; }
	auto float_value { dynamic_cast<Float *>(exp) };
	if (float_value) { return float_value; }
	auto fract_value { dynamic_cast<Fraction *>(exp) };
	if (fract_value) { return fract_value; }
	auto str_value { dynamic_cast<String *>(exp) };
	if (str_value) { return str_value; }
	auto sym_value { dynamic_cast<Symbol *>(exp) };
	if (sym_value) {
		return env->has(sym_value->value()) ? env->get(sym_value->value()) : exp;
	}
	auto lst_value { dynamic_cast<Pair *>(exp) };
	if (lst_value) {
		if (is_define_special(lst_value)) {
			auto key { dynamic_cast<Symbol *>(define_key(lst_value)) };
			auto value { define_value(lst_value, env) };
			ASSERT(key && is_good(value), "define");
			env->insert(key->value(), value);
			return value;
		}
		if (is_lambda_special(lst_value)) {
			auto args { lambda_args(lst_value) };
			auto body { lambda_body(lst_value) };
			ASSERT(is_good(args) && is_good(body), "lambda");
			return new Procedure(args, body, env);
		}
		if (is_if_special(lst_value)) {
			auto condition { eval(if_condition(lst_value), env) };
			ASSERT(is_good(condition), "if");
			ASSERT(is_null(cddddr(lst_value)), "if");
			if (is_true(condition)) {
				return eval(if_consequence(lst_value), env);
			} else {
				return eval(if_alternative(lst_value), env);
			}
		}
		if (is_cond_special(lst_value)) {
			auto expr { build_cond(cdr(lst_value)) };
			ASSERT(is_good(expr), "cond");
			return eval(expr, env);
		}
		if (is_begin_special(lst_value)) {
			Element *result { nullptr };
			auto cur { cdr(lst_value) };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "begin");
			}
			return result;
		}
		if (is_and_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { &True };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "and");
				if (is_false(result)) { break; }
			}
			return result;
		}
		if (is_or_special(lst_value)) {
			auto cur { cdr(lst_value) };
			Element *result { nullptr };
			for (; is_good(cur) && cur; cur = cdr(cur)) {
				result = eval(car(cur), env);
				ASSERT(is_good(result), "or");
				if (is_true(result)) { break; }
			}
			return result;
		}
		if (is_quote_special(lst_value)) {
			return cdr(lst_value);
		}
		if (is_let_special(lst_value)) {
			auto expr { build_let(lst_value) };
			ASSERT(is_good(expr), "let");
			return eval(expr, env);
		}
		if (is_set_special(lst_value)) {
			ASSERT(is_valid_set(lst_value), "set!");
			auto sym { dynamic_cast<Symbol *>(set_var(lst_value)) };
			auto val { eval(set_value(lst_value), env) };
			ASSERT(is_good(val), "set!");
			return env->update(sym, val);
		}
		auto lst { eval_list(lst_value, env) };
		return apply(car(lst), cdr(lst));
	}
	ASSERT(false, "eval");
}
