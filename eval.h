/**
 * evaluate expressions
 * and call functions
 * also special forms are handled
 */

class Primitive : public Obj {
	public:
		virtual Obj *apply(Obj *args) = 0;
		std::ostream &write(std::ostream &out) override;

};

std::ostream &Primitive::write(std::ostream &out) {
	return out << "#primitive";
}

struct Procedure_Case {
	Procedure_Case(Obj *a, Obj *b): args { a }, body { b } { }
	Obj *args;
	Obj *body;
};

class Procedure : public Obj {
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

		void add_case(Obj *args, Obj *body) {
			cases_.emplace_back(args, new Pair { Symbol::get("begin"), body });
		}

		Procedure(Obj *args, Obj *body, Frame *env): env_ { env }
		{
			add_case(args, body);
	       	}

		Obj *build_env(Procedure_Case &c, Obj *arg_values);
		Obj *get_body(Procedure_Case &c);

		Obj *apply(Obj *arg_values);
		std::ostream &write(std::ostream &out) override;
};

std::ostream &Procedure::write(std::ostream &out) {
	if (cases_.size() == 1) {
		out << "(lambda " << cases_[0].args;
		if (is_pair(cdr(cases_[0].body))) {
			out << "\n  ";
			write_inner_complex_pair(out, dynamic_cast<Pair *>(cdr(cases_[0].body)), " ");
		}
		else {
			out << " . " << cdr(cases_[0].body);
		}
		out << ')';
	} else {
		out << "#lambda-case";

	}
	return out;
}

Symbol *assert_sym(Obj *elm) {
	auto sym { dynamic_cast<Symbol *>(elm) };
	if (! sym) { err("assert_sym", "no symbol", elm); }
	return sym;
}

Obj *eval(Obj *exp, Frame *env);

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

Obj *Procedure::build_env(Procedure_Case &c, Obj *arg_values) {
	auto new_env { new Frame { env_ } };

	Obj *cur { c.args };
	for (; is_pair(cur); cur = cdr(cur)) {
		auto sym { assert_sym(car(cur)) };
		ASSERT(sym, "build_env");
		auto value { car(arg_values) };
		new_env->insert(sym->value(), value);
		arg_values = cdr(arg_values);
	}
	if (cur) {
		auto sym { assert_sym(cur) };
		ASSERT(sym, "build_env");
		new_env->insert(sym->value(), arg_values);
	}
	return new_env;
}

Obj *Procedure::get_body(Procedure_Case &c) {
	return c.body;
}

bool matches(Obj *args, Obj *values) {
	if (! args) { return values == nullptr; }
	auto pa { dynamic_cast<Pair *>(args) };
	if (! pa) { return true; }
	auto pv { dynamic_cast<Pair *>(values) };
	if (! pv) { return false; }
	return matches(cdr(pa), cdr(pv));

}

bool case_matches(const Procedure_Case &c, Obj *arg_values) {
	return matches(c.args, arg_values);
}

Obj *Procedure::apply(Obj *arg_values) {
	for (auto &c : cases_) {
		if (case_matches(c, arg_values)) {
			auto new_env { dynamic_cast<Frame *>(build_env(c, arg_values)) };
			ASSERT(new_env, "procedure apply");
			{
				Frame_Guard fg { new_env };
				return eval(get_body(c), new_env);
			}
		}
	}

	err("procedure-apply", "no match", arg_values);
	return nullptr;
}

Obj *apply(Obj *op, Obj *operands) {
	auto prim { dynamic_cast<Primitive *>(op) };
	if (prim) { return prim->apply(operands); }
	auto proc { dynamic_cast<Procedure *>(op) };
	if (proc) { return proc->apply(operands); }
	err("apply", "unknown operation", op);
	return nullptr;
}

Obj *eval_list(Obj *exp, Frame *env) {
	if (! exp) { return exp; }
	auto head { eval(car(exp), env) };
	auto rest { dynamic_cast<Pair *>(cdr(exp)) };
	if (rest) {
		return new Pair { head, eval_list(rest, env) };
	} else {
		return new Pair { head, eval(cdr(exp), env) };
	}
}

Symbol *first_symbol(Obj *lst) {
	return dynamic_cast<Symbol *>(car(lst));
}

bool is_tagged_list(Obj *lst, const std::string &tag) {
	auto sym { first_symbol(lst) };
	return sym && sym->value() == tag;
}

inline bool is_define_special(Pair *lst) {
	return is_tagged_list(lst, "define");
}

inline bool is_define_syntax_special(Obj *lst) {
	return is_tagged_list(lst, "define-syntax");
}

inline Obj *define_key(Pair *lst) {
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
	return nullptr;
}

inline Obj *define_value(Pair *lst, Frame *env) {
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

inline Obj *lambda_args(Pair *lst) {
	return cadr(lst);
}

inline Obj *lambda_body(Pair *lst) {
	return cddr(lst);
}

inline bool is_case_lambda_special(Pair *lst) {
	return is_tagged_list(lst, "case-lambda") && is_pair(cdr(lst));
}

inline Obj *case_lambda_cases(Pair *lst) {
	return cdr(lst);
}

inline bool is_if_special(Pair *lst) {
	return is_tagged_list(lst, "if");
}

inline Obj *if_condition(Pair *lst) {
	return cadr(lst);
}

inline Obj *if_consequence(Pair *lst) {
	return caddr(lst);
}

inline Obj *if_alternative(Pair *lst) {
	return cadddr(lst);
}

inline bool is_cond_special(Pair *lst) {
	return is_tagged_list(lst, "cond");
}

Obj *build_cond(Obj *lst) {
	if (is_null(lst)) { return lst; }
	auto expr { car(lst) };
	auto cond { car(expr) };
	auto cons { cdr(expr) };
	auto sym { dynamic_cast<Symbol *>(cond) };
	if (sym && sym->value() == "else") {
		if (cdr(lst)) {
			err("cond", "else not in last case");
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

Obj *build_let_args(Obj *arg_vals, Obj *args) {
	if (is_null(arg_vals)) { return args; }
	args = build_let_args(cdr(arg_vals), args);
	return new Pair { car(car(arg_vals)), args };
}

Obj *build_let_vals(Obj *arg_vals, Obj *vals) {
	if (is_null(arg_vals)) { return vals; }
	vals = build_let_vals(cdr(arg_vals), vals);
	return new Pair { cadr(car(arg_vals)), vals };
}

Obj *build_let(Obj *lst) {
	lst = cdr(lst);
	auto name { dynamic_cast<Symbol *>(car(lst)) };
	if (name) { lst = cdr(lst); };
	auto arg_vals { car(lst) };
	auto block { cdr(lst) };
	auto args { build_let_args(arg_vals, nullptr) };
	auto vals { build_let_vals(arg_vals, nullptr) };
	auto lambda { new Pair {
		Symbol::get("lambda"),
		new Pair { args, block }
	}};
	if (name) {
		// (let name ((a 2) (b 3)) x y)
		// ((lambda (name) (name 2 3)) (lambda (a b) x y))
		// ((lambda (name) (set! name (lambda (a b) x y)) (name 2 3)) #f)
		// ((lambda (a b) x y) 2 3)

		auto inner_arg = new Pair { name, nullptr };
		auto inner_set = new Pair { 
			Symbol::get("set!"),
			new Pair {
				name,
				new Pair { lambda, nullptr }
			}
		};
		auto inner_call = new Pair { name, vals };
		auto inner = new Pair {
			Symbol::get("lambda"),
			new Pair {
				inner_arg,
				new Pair {
					inner_set,
					new Pair {
						inner_call,
						nullptr
					}
				}
			}
		};
		return new Pair { inner, new Pair { false_obj, nullptr } };
	} else {
		return new Pair { lambda, vals };
	}
}

bool is_set_special(Pair *lst) {
	return is_tagged_list(lst, "set!");
}

Obj *set_var(Obj *lst) { return cadr(lst); }
Obj *set_value(Obj *lst) { return caddr(lst); }

bool is_valid_set(Pair *lst) {
	return is_null(cdddr(lst));
}

bool is_assert_special(Pair * lst) {
	return is_tagged_list(lst, "assert");
}

bool is_valid_assert(Pair *lst) {
	return is_null(cddr(lst));
}

class Active_Guard {
		Obj **elm_;
	public:
		Active_Guard(Obj **elm) : elm_ { elm } {
			if (*elm_) { (*elm_)->make_active(); };
		}
		~Active_Guard() { if (*elm_) { (*elm_)->cease_active(); } }
		void swap(Obj *el) {
			if (el) { el->make_active(); }
			if (*elm_) { (*elm_)->cease_active(); }
			*elm_ = el;
		}
};

bool evals_to_self(Obj *obj) {
	if (dynamic_cast<Symbol *>(obj)) { return false; }
	if (dynamic_cast<Pair *>(obj)) { return false; }
	return true;
}

struct Syntax_Rule {
	Syntax_Rule(Obj *p, Obj *r): pattern { p }, replacement { r } { }
	Obj *pattern;
	Obj *replacement;
};

#include <set>

class Syntax : public Obj {
	public:
		const std::string name_;
		std::set<std::string> keywords_;
		std::vector<Syntax_Rule> rules_;
		bool is_repeating(Obj *cur) {
			auto nxt { dynamic_cast<Pair *>(cdr(cur)) };
			if (! nxt) { return false; }
			auto sym { dynamic_cast<Symbol *>(car(nxt)) };
			return sym && sym->value() == "...";
		}
		Frame *match_one(Obj *pattern, Obj *value, Frame *match, bool repeating) {
			auto pair { dynamic_cast<Pair *>(pattern) };
			if (pair) {
				auto vp { dynamic_cast<Pair *>(value) };
				if (! vp) { return nullptr; }
				return match_rest(pair, vp, match, repeating);
			}
			auto sym { dynamic_cast<Symbol *>(pattern) };
			if (sym) {
				if (keywords_.find(sym->value()) != keywords_.end()) {
					if (value == sym) {
						if (! match) {
							match = new Frame { nullptr };
						}
						return match;
					} else { return nullptr; }
				}
				if (! match) { match = new Frame { nullptr }; }
				if (repeating) {
					auto nw { new Pair {
						value, match->get(sym->value())
					}};
					match->insert(sym->value(), nw);
				} else {
					match->insert(sym->value(), value);
				}
				return match;
			}
			return nullptr;
		}
		Frame *match_rest(Obj *pattern, Obj *values, Frame *match, bool repeating) {
			for (;;) {
				if (! pattern) {
					if (values) { return nullptr; }
					if (! match) { match = new Frame { nullptr }; }
					return match;
				}
				if (! is_pair(pattern)) { return nullptr; }
				bool repeating { is_repeating(pattern) };
				if (! repeating) {
					if (! is_pair(values)) { return nullptr; }
					match = match_one(car(pattern), car(values), match, false);
					if (! match) { return nullptr; }
					values = cdr(values);
				} else {
					while (values) {
						if (! is_pair(values)) { return nullptr; }
						match = match_one(car(pattern), car(values), match, true);
						if (! match) { return nullptr; }
						values = cdr(values);
					}
				}
				pattern = cdr(pattern);
				if (repeating) {
				       	pattern = cdr(pattern);
					if (! pattern) { err("match-rest", "elements after ...", pattern); return nullptr; }
			       	}
			}

		}
		Frame *build_match(Syntax_Rule &rule, Obj *lst) {
			return match_rest(rule.pattern, lst, nullptr, false);
		}
		Obj *apply_match(Syntax_Rule &rule, Frame *match) {
			err("syntax", "match not implemented");
			return nullptr;
		}
	protected:
		void propagate_mark() override {
			for (auto &r : rules_) {
				mark(r.pattern);
				mark(r.replacement);
			}
		}
	public:
		Syntax(const std::string name) : name_ { name } { }
		const std::string &name() const { return name_; }
		void add_keyword(const std::string &kw) { keywords_.insert(kw); }
		void add_rule(Obj *pattern, Obj *replacement) {
			rules_.emplace_back(pattern, replacement);
		}
		std::ostream &write(std::ostream &out) override {
			return out << "#syntax";
		}
		Obj *apply(Obj *lst, Frame *env) {
			for (auto &r : rules_) {
				auto m { build_match(r, lst) };
				if (m) {
					return apply_match(r, m);
				}
			}
			err("syntax", "no match", lst);
			return nullptr;
		}
};

std::map<std::string, Syntax *> syntax_extensions;

Syntax *find_syntax_extension(Obj *lst) {
	auto sym { first_symbol(lst) };
	if (sym) {
		auto got { syntax_extensions.find(sym->value()) };
		if (got != syntax_extensions.end()) {
			return got->second;
		}
	}
	return nullptr;
}

#include <cassert>

void syntax_tests() {
	auto s { new Syntax { "test" } };
	s->add_keyword("foo");
	s->add_rule(parse_expression("(_ x)"), parse_expression("(* x x)"));
	std::cerr << s->rules_[0].pattern << '\n';
	std::cerr << s->rules_[0].replacement << '\n';
	auto m1 { s->match_one(parse_expression("foo"), parse_expression("foo"), nullptr, false) };
	assert(m1 != nullptr);
	auto m2 { s->match_one(parse_expression("x"), parse_expression("42"), nullptr, false) };
	assert(m2 != nullptr);
	assert(is_equal_num(m2->get("x"), Integer::create(42)));
	auto m3 { s->match_rest(nullptr, nullptr, nullptr, false) };
	assert(m3 != nullptr);
	auto m4 { s->match_rest(parse_expression("(a b)"), parse_expression("(2 3)"), nullptr, false) };
	assert(m4 != nullptr);
	assert(is_equal_num(m4->get("a"), Integer::create(2)));
	assert(is_equal_num(m4->get("b"), Integer::create(3)));
	auto m5 { s->build_match(s->rules_[0], parse_expression("(test 42)")) };
	assert (m5 != nullptr);
	std::cerr << m5->get("_") << '\n';
}

Obj *eval(Obj *exp, Frame *env) {
	Frame_Guard frame_guard;
	Active_Guard exp_guard { &exp };
	for (;;) {
		if (evals_to_self(exp)) { return exp; }
		auto sym_value { dynamic_cast<Symbol *>(exp) };
		if (sym_value) {
			return env->has(sym_value->value()) ? env->get(sym_value->value()) : exp;
		}
		auto lst_value { dynamic_cast<Pair *>(exp) };
		if (lst_value) {
			auto se { find_syntax_extension(lst_value) };
			if (se) {
				exp_guard.swap(se->apply(lst_value, env));
				continue;
			}
			if (is_define_special(lst_value)) {
				auto key { dynamic_cast<Symbol *>(define_key(lst_value)) };
				auto value { define_value(lst_value, env) };
				ASSERT(key, "define");
				env->insert(key->value(), value);
				return value;
			}
			if (is_define_syntax_special(lst_value)) {
				auto name { dynamic_cast<Symbol *>(cadr(lst_value)) };
				auto rules { dynamic_cast<Pair *>(caddr(lst_value)) };
				ASSERT(name && rules && ! cdddr(lst_value), "syntax-rules");
				ASSERT(is_tagged_list(rules, "syntax-rules"), "ysyntax-rules");
				auto se { new Syntax { name->value() } };
				auto keywords { cadr(rules) };
				for (; keywords; keywords = cdr(keywords)) {
					ASSERT(is_pair(keywords), "syntax-rules");
					auto sym { dynamic_cast<Symbol *>(car(keywords)) };
					ASSERT(sym, "syntax-rules");
					se->add_keyword(sym->value());
				}
				auto cur { cddr(rules) };
				for (; cur; cur = cdr(cur)) {
					ASSERT(is_pair(cur), "syntax-rules");
					auto rule { dynamic_cast<Pair *>(car(cur)) };
					ASSERT(rule, "syntax-rules");
					auto pattern { dynamic_cast<Pair *>(car(rule)) };
					auto replacement { dynamic_cast<Pair *>(cadr(rule)) };
					ASSERT(pattern && replacement && ! cddr(rule), "syntax-rules");
					se->add_rule(pattern, replacement);
				}
				syntax_extensions[se->name()] = se;
				return se;
			}
			if (is_lambda_special(lst_value)) {
				auto args { lambda_args(lst_value) };
				auto body { lambda_body(lst_value) };
				return new Procedure(args, body, env);
			}
			if (is_case_lambda_special(lst_value)) {
				auto cases { case_lambda_cases(lst_value) };
				auto proc { new Procedure(env) };
				for (; is_pair(cases) && is_pair(car(cases)); cases = cdr(cases)) {
					auto pair { dynamic_cast<Pair *>(car(cases)) };
					proc->add_case(car(pair), cdr(pair));
				}
				return proc;
			}
			if (is_if_special(lst_value)) {
				auto condition { eval(if_condition(lst_value), env) };
				ASSERT(is_null(cdddr(lst_value)) || is_null(cddddr(lst_value)), "if");
				if (is_true(condition)) {
					exp_guard.swap(if_consequence(lst_value));
					continue;
				} else {
					if (is_null(cdddr(lst_value))) {
						exp_guard.swap(false_obj);
					} else {
						exp_guard.swap(if_alternative(lst_value));
					}
					continue;
				}
			}
			if (is_cond_special(lst_value)) {
				exp_guard.swap(build_cond(cdr(lst_value)));
				continue;
			}
			if (is_begin_special(lst_value)) {
				auto cur { cdr(lst_value) };
				for (; cdr(cur); cur = cdr(cur)) {
					eval(car(cur), env);
				}
				exp_guard.swap(car(cur));
				continue;
			}
			if (is_and_special(lst_value)) {
				auto cur { cdr(lst_value) };
				Obj *result { true_obj };
				for (; is_true(cur) && cur; cur = cdr(cur)) {
					result = eval(car(cur), env);
					if (is_false(result)) { break; }
				}
				return result;
			}
			if (is_or_special(lst_value)) {
				auto cur { cdr(lst_value) };
				Obj *result { false_obj };
				for (; cur; cur = cdr(cur)) {
					result = eval(car(cur), env);
					if (is_true(result)) { break; }
				}
				return result;
			}
			if (is_quote_special(lst_value)) {
				return cadr(lst_value);
			}
			if (is_let_special(lst_value)) {
				exp_guard.swap(build_let(lst_value));
				continue;
			}
			if (is_set_special(lst_value)) {
				ASSERT(is_valid_set(lst_value), "set!");
				auto sym { dynamic_cast<Symbol *>(set_var(lst_value)) };
				auto val { eval(set_value(lst_value), env) };
				if (sym) {
					return env->update(sym, val);
				}
				err("set!", "unknown key", set_var(lst_value));
			}
			if (is_assert_special(lst_value)) {
				ASSERT(is_valid_assert(lst_value), "assert");
				auto val { eval(cadr(lst_value), env) };
				if (is_false(val)) {
					err("assert", "failed", lst_value);
				}
				return Symbol::get("ok");
			}
			auto lst { eval_list(lst_value, env) };
			auto proc { dynamic_cast<Procedure *>(car(lst)) };
			if (proc) {
				bool done { false };
				for (auto &c : proc->cases_) {
					if (case_matches(c, cdr(lst))) {
						auto new_env { dynamic_cast<Frame *>(proc->build_env(c, cdr(lst))) };
						ASSERT(new_env, "eval apply");
						env = new_env;
						exp_guard.swap(proc->get_body(c));
						done = true;
						break;
					}
				}

				if (done) {
					continue;
				} else {
					err("procedure-apply", "no match", cdr(lst));
				}
			}
			return apply(car(lst), cdr(lst));
		}
		break;
	}
	err("eval", "unknown", exp);
	return nullptr;
}
