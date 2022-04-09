#pragma once

#include "value.h"
#include "dynamic.h"

class String : public Value_Element<std::string> {
	public:
		String(const std::string &value) : Value_Element(value) { }
		std::ostream &write(std::ostream &out) override {
			return out << '"' << value() << '"';
		}
};

constexpr auto as_string = Dynamic::as<String>;
constexpr auto is_string = Dynamic::is<String>;
