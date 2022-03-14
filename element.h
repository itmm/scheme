/**
 * define base element for all Scheme-related types
 * it can be written to an output stream
 * and it is kept in a list to be garbage collected
 * the lowest bit for the link-field is used as mark
 * for the garbage collection algorithm
 */

#include <iostream>

class Element {
		Element *next_;

		static Element *all_elements;
		static bool current_mark;

		bool get_mark() {
			return static_cast<bool>(reinterpret_cast<size_t>(next_) & 0x1);
		}

		bool has_current_mark() {
			return get_mark() == current_mark;
		}

		void toggle_mark() {
			next_ = reinterpret_cast<Element *>(
				reinterpret_cast<size_t>(next_) ^ 0x1
			);
		}

		static Element *add_mark(Element *ptr, bool mark) {
			return reinterpret_cast<Element *>(
				reinterpret_cast<size_t>(ptr) | static_cast<size_t>(mark)
			);
		}

		static Element *add_current_mark(Element *ptr) {
			return add_mark(ptr, current_mark);
		}

		static Element *remove_mark(Element *marked, bool mark) {
			return reinterpret_cast<Element *>(
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
			
		void mark(Element *elm) { if (elm) { elm->mark(); } }

	public:
		Element(): next_ { add_current_mark(all_elements) } {
			all_elements = this;
		}
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) = 0;
		static std::pair<unsigned, unsigned> garbage_collect();
};

inline std::ostream &operator<<(std::ostream &out, Element *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}
