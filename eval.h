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

		Element *build_env(Element *arg_values);
		Element *get_body();

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
		size_t init_size_;
		void reset() {
			while (active_frames.size() > init_size_) {
				active_frames.pop_back();
			}
		}
	public:
		Frame_Guard() : init_size_ { active_frames.size() } { }
		Frame_Guard(Frame *frame) : Frame_Guard() { set(frame); }
		void set(Frame *frame) { reset(); active_frames.push_back(frame); }
		~Frame_Guard() { reset(); }
};

Element *Procedure::build_env(Element *arg_values) {
	auto new_env { new Frame { env_ } };

	Element *cur { args_ };
	ASSERT(is_good(cur), "build_env");
	for (; is_pair(cur); cur = cdr(cur)) {
		auto sym { assert_sym(car(cur)) };
		ASSERT(sym, "build_env");
		auto value { car(arg_values) };
		ASSERT(is_good(value), "build_env");
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	ASSERT(is_good(cur), "build_env");
	if (cur) {
		auto sym { assert_sym(cur) };
		ASSERT(sym, "build_env");
		ASSERT(is_good(arg_values), "build_env");
		new_env->insert(sym->value(), arg_values);
	}
	return new_env;
}

Element *Procedure::get_body() {
	return new Pair { Symbol::get("begin"), body_ };
}

Element *Procedure::apply(Element *arg_values) {
	auto new_env { dynamic_cast<Frame *>(build_env(arg_values)) };
	ASSERT(new_env, "procedure apply");
	Frame_Guard fg { new_env };
	return eval(get_body(), new_env);
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
	auto result { new Pair {
		new Pair {
			Symbol::get("lambda"),
			new Pair { args, block }
		},
		vals
	}};
	return result;
}

bool is_set_special(Pair *lst) {
	return is_tagged_list(lst, "set!");
}

Element *set_var(Element *lst) { return cadr(lst); }
Element *set_value(Element *lst) { return caddr(lst); }

bool is_valid_set(Pair *lst) {
	return is_good(set_value(lst)) && is_null(cdddr(lst));
}

bool is_assert_special(Pair * lst) {
	return is_tagged_list(lst, "assert");
}

bool is_valid_assert(Pair *lst) {
	return is_good(cadr(lst)) && is_null(cddr(lst));
}

Element *eval(Element *exp, Frame *env) {
	Frame_Guard frame_guard;
	for (;;) {
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
					exp = if_consequence(lst_value);
					continue;
				} else {
					exp = if_alternative(lst_value);
					continue;
				}
			}
			if (is_cond_special(lst_value)) {
				exp = build_cond(cdr(lst_value));
				ASSERT(is_good(exp), "cond");
				continue;
			}
			if (is_begin_special(lst_value)) {
				auto cur { cdr(lst_value) };
				for (; cdr(cur); cur = cdr(cur)) {
					auto result { eval(car(cur), env) };
					ASSERT(is_good(result), "begin");
				}
				exp = car(cur);
				continue;
			}
			if (is_and_special(lst_value)) {
				auto cur { cdr(lst_value) };
				Element *result { one };
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
				exp = build_let(lst_value);
				ASSERT(is_good(exp), "let");
				continue;
			}
			if (is_set_special(lst_value)) {
				ASSERT(is_valid_set(lst_value), "set!");
				auto sym { dynamic_cast<Symbol *>(set_var(lst_value)) };
				auto val { eval(set_value(lst_value), env) };
				ASSERT(is_good(val), "set!");
				if (sym) {
					return env->update(sym, val);
				}
				auto pair { dynamic_cast<Pair *>(set_var(lst_value)) };
				if (pair) {
					if (is_tagged_list(pair, "car")) {
						auto got { eval(cadr(pair), env) };
						auto res { dynamic_cast<Pair *>(got) };
						ASSERT(res, "set! car");
						res->set_head(val);
						return val;
					}
					else if (is_tagged_list(pair, "cdr")) {
						auto got { eval(cadr(pair), env) };
						auto res { dynamic_cast<Pair *>(got) };
						ASSERT(res, "set! cdr");
						res->set_rest(val);
						return val;
					}
				}
				return err("set!", "unknown key", set_var(lst_value));
			}
			if (is_assert_special(lst_value)) {
				ASSERT(is_valid_assert(lst_value), "assert");
				auto val { eval(cadr(lst_value), env) };
				if (is_false(val)) {
					return err("assert", "failed", lst_value);
				}
				return Symbol::get("ok");
			}
			auto lst { eval_list(lst_value, env) };
			auto proc { dynamic_cast<Procedure *>(car(lst)) };
			if (proc) {
				auto new_env { dynamic_cast<Frame *>(proc->build_env(cdr(lst))) };
				ASSERT(new_env, "eval apply");
				frame_guard.set(new_env);
				env = new_env;
				exp = proc->get_body();
				continue;
			}
			return apply(car(lst), cdr(lst));
		}
		break;
	}
	ASSERT(false, "eval");
}
