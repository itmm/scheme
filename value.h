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
