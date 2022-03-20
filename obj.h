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

class Obj {
		Obj *next_;

		static Obj *all_elements;
		static bool current_mark;
		static std::vector<Obj *> active_elements;

		bool get_mark() {
			return static_cast<bool>(reinterpret_cast<size_t>(next_) & 0x1);
		}

		bool has_current_mark() {
			return get_mark() == current_mark;
		}

		void toggle_mark() {
			next_ = reinterpret_cast<Obj *>(
				reinterpret_cast<size_t>(next_) ^ 0x1
			);
		}

		static Obj *add_mark(Obj *ptr, bool mark) {
			return reinterpret_cast<Obj *>(
				reinterpret_cast<size_t>(ptr) | static_cast<size_t>(mark)
			);
		}

		static Obj *add_current_mark(Obj *ptr) {
			return add_mark(ptr, current_mark);
		}

		static Obj *remove_mark(Obj *marked, bool mark) {
			return reinterpret_cast<Obj *>(
				reinterpret_cast<size_t>(marked) ^ static_cast<size_t>(mark)
			);
		}

		void mark() {
			if (! has_current_mark()) {
				toggle_mark();
				propagate_mark();
			}
		}

	protected:
		virtual void propagate_mark() { }
			
		void mark(Obj *elm) { if (elm) { elm->mark(); } }

	public:
		Obj(): next_ { add_current_mark(all_elements) } {
			all_elements = this;
		}
		virtual ~Obj() { }
		virtual std::ostream &write(std::ostream &out) = 0;
		static std::pair<unsigned, unsigned> garbage_collect();

		void make_active() { active_elements.push_back(this); }
		void cease_active() {
			active_elements.erase(find(active_elements.begin(), active_elements.end(), this));
		}
};

using Element = Obj;

inline std::ostream &operator<<(std::ostream &out, Obj *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}
