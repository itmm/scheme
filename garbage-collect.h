/**
 * implementation of the garbage collection
 */

Element *Element::all_elements { nullptr };
bool Element::current_mark { true };

std::pair<unsigned, unsigned> Element::garbage_collect() {
	current_mark = ! current_mark;
	for (auto &f : active_frames) { f->mark(); }

	unsigned kept { 0 };
	unsigned collected { 0 };
	Element *prev { nullptr };
	Element *cur { all_elements };
	while (cur) {
		if (cur != One && cur != Zero && cur->mark_ != current_mark) {
			++collected;
			auto tmp { cur->next_ };
			delete cur;
			cur = tmp;
			if (prev) {
				prev->next_ = cur;	
			} else { all_elements = cur; }
		} else {
			++kept;
			prev = cur;
			cur = cur->next_;
		}
	}
	return { collected, kept };
}
