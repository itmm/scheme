#include "obj.h"

Obj::Mark_Container Obj::a_marked_;
Obj::Mark_Container Obj::b_marked_;
Obj::Mark_Container *Obj::current_marked_ { &a_marked_ };
Obj::Mark_Container *Obj::other_marked_ { &b_marked_ };

std::vector<Obj *> Obj::active_elements;

std::ostream &operator<<(std::ostream &out, Obj *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}
