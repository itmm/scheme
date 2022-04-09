#pragma once

#include "obj.h"

namespace Dynamic {
	template<typename C> C *as(Obj *obj) { return dynamic_cast<C *>(obj); }
	template<typename C> bool is(Obj *obj) { return as<C>(obj); }
}
