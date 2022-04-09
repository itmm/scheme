/**
 * numeric types
 */

#pragma once

#include "dynamic.h"

class Numeric : public Obj { };

constexpr auto is_numeric = Dynamic::is<Numeric>;

class Exact_Numeric : public Numeric { };

constexpr auto is_exact = Dynamic::is<Exact_Numeric>;

class Inexact_Numeric : public Numeric { };

constexpr auto is_inexact = Dynamic::is<Inexact_Numeric>;

struct Complex_Numeric {
	virtual ~Complex_Numeric() { }
};

constexpr auto is_complex = Dynamic::is<Complex_Numeric>;

