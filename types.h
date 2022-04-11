/**
 * basic types
 * Integers can be as long as memory permits
 * nullptr is treated as the empty pair
 */

#pragma once

#include "dynamic.h"

#include <map>

class Symbol : public Obj {
		static std::map<std::string, Symbol *>symbols_;
		std::string value_;
		Symbol(const std::string &value): value_ { value } { }
	public:
		~Symbol();
		static Symbol *get(const std::string &value);
		const std::string &value() const { return value_; }
		std::ostream &write(std::ostream &out) { return out << value_; }
};

constexpr auto as_symbol = Dynamic::as<Symbol>;
constexpr auto is_symbol = Dynamic::is<Symbol>;

class False : public Obj {
	public:
		False() {}
		std::ostream &write(std::ostream &out) override { return out << "#f"; }
};

class True : public Obj {
	public:
		True() {}
		std::ostream &write(std::ostream &out) override { return out << "#t"; }
};

extern False *false_obj;
extern True *true_obj;

Obj *to_bool(bool cond);

inline bool is_false(Obj *value) { return value == false_obj; }

inline bool is_true(Obj *value) { return ! is_false(value); }

class Pair : public Obj {
		Obj *head_;
		Obj *rest_;
	protected:
		void propagate_mark() override { mark(head_); mark(rest_); }
	public:
		Pair(Obj *head, Obj *rest): head_ { head }, rest_ { rest } { }
		Obj *head() const { return head_; }
		Obj *rest() const { return rest_; }
		void set_head(Obj *head) { head_ = head; }
		void set_rest(Obj *rest) { rest_ = rest; }
		std::ostream &write(std::ostream &out) override;
};

constexpr auto as_pair = Dynamic::as<Pair>;
constexpr auto is_pair = Dynamic::is<Pair>;
inline bool is_null(Obj *element) { return ! element; }

Obj *car(Obj *obj);
Obj *cdr(Obj *obj);

inline Obj *cadr(Obj *obj) { return car(cdr(obj)); }
inline Obj *cddr(Obj *obj) { return cdr(cdr(obj)); }
inline Obj *caddr(Obj *obj) { return car(cddr(obj)); }
inline Obj *cdddr(Obj *obj) { return cdr(cddr(obj)); }
inline Obj *cadddr(Obj *obj) { return car(cdddr(obj)); }
inline Obj *cddddr(Obj *obj) { return cdr(cdddr(obj)); }

inline Obj *cons(Obj *a, Obj *b) { return new Pair { a, b }; }
constexpr Obj *build_list() { return nullptr; }
template<typename T1, typename... Rest> Obj *build_list(T1 first, Rest... rest) {
	return cons(first, build_list(rest...));
}

void write_inner_complex_pair(std::ostream &out, Pair *pair, std::string indent);

