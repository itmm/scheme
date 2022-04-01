/**
 * define base element for all Scheme-related types
 * it can be written to an output stream
 * and it is kept in a list to be garbage collected
 * the lowest bit for the link-field is used as mark
 * for the garbage collection algorithm
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <set>

class Obj {
		using Mark_Container = std::set<Obj *>;
		static Mark_Container a_marked_;
		static Mark_Container b_marked_;
		static Mark_Container *current_marked_;
		static Mark_Container *other_marked_;

		static std::vector<Obj *> active_elements;

		bool has_current_mark() {
			return current_marked_->find(this) != current_marked_->end();
		}

		void mark() {
			if (! has_current_mark()) {
				other_marked_->erase(this);
				current_marked_->insert(this);
				propagate_mark();
			}
		}

	protected:
		virtual void propagate_mark() { }
			
		void mark(Obj *elm) { if (elm) { elm->mark(); } }

	public:
		Obj() { current_marked_->insert(this); }
		virtual ~Obj() {
			current_marked_->erase(this);
			other_marked_->erase(this);
		}
		virtual std::ostream &write(std::ostream &out) = 0;
		static std::pair<unsigned, unsigned> garbage_collect();

		void make_active() { active_elements.push_back(this); }
		void cease_active() {
			active_elements.erase(find(active_elements.begin(), active_elements.end(), this));
		}
};

Obj::Mark_Container Obj::a_marked_;
Obj::Mark_Container Obj::b_marked_;
Obj::Mark_Container *Obj::current_marked_ { &a_marked_ };
Obj::Mark_Container *Obj::other_marked_ { &b_marked_ };

std::vector<Obj *> Obj::active_elements;

inline std::ostream &operator<<(std::ostream &out, Obj *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}
