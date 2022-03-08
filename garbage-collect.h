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
		auto mark { cur->get_mark() };
		if (cur != One && cur != Zero && mark != current_mark) {
			++collected;
			auto tmp { remove_mark(cur->next_, mark) };
			delete cur;
			cur = tmp;
			if (prev) {
				bool prev_mark { prev->get_mark() };
				prev->next_ = add_mark(cur, prev_mark);
			} else { all_elements = cur; }
		} else {
			++kept;
			prev = cur;
			cur = remove_mark(cur->next_, mark);
		}
	}
	return { collected, kept };
}
