/**
 * define base element for all Scheme-related types
 * it can be written to an output stream
 * and it is kept in a list to be garbage collected
 */

#include <iostream>

class Element {
		Element *next_;
		bool mark_;

		static Element *all_elements;
		static bool current_mark;

		void mark() {
			if (mark_ != current_mark) {
				mark_ = current_mark;
				propagate_mark();
			}
		}

	protected:
		virtual void propagate_mark() { }
			
		void mark(Element *elm) { if (elm) { elm->mark(); } }

	public:
		Element(): next_ { all_elements }, mark_ { current_mark } {
			all_elements = this;
		}
		virtual ~Element() { }
		virtual std::ostream &write(std::ostream &out) const = 0;
		static std::pair<unsigned, unsigned> garbage_collect();
};

inline std::ostream &operator<<(std::ostream &out, Element *elm) {
	if (elm) {
		return elm->write(out);
	} else {
		return out << "()";
	}
}
