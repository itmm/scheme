#pragma once

#include "obj.h"

template<typename VALUE_TYPE, typename BASE = Obj>
class Value_Element : public BASE {
		VALUE_TYPE value_;
	public:
		Value_Element(const VALUE_TYPE &value): value_ { value } { }
		const VALUE_TYPE &value() const { return value_; }
		std::ostream &write(std::ostream &out) override {
			return out << value_;
		}
};

class String : public Value_Element<std::string> {
	public:
		String(const std::string &value) : Value_Element(value) { }
		std::ostream &write(std::ostream &out) override {
			return out << '"' << value() << '"';
		}
};

namespace Dynamic {
	template<typename C> C *as(Obj *obj) { return dynamic_cast<C *>(obj); }
	template<typename C> bool is(Obj *obj) { return as<C>(obj); }
}

constexpr auto as_string = Dynamic::as<String>;
constexpr auto is_string = Dynamic::is<String>;
