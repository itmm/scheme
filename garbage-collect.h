/**
 * implementation of the garbage collection
 */

Obj *Obj::all_elements { nullptr };
bool Obj::current_mark { true };
std::vector<Obj *> Obj::active_elements;

std::pair<unsigned, unsigned> Obj::garbage_collect() {
	current_mark = ! current_mark;
	for (auto &f : active_frames) { f->mark(); }
	for (auto &e : active_elements) { e->mark(); }

	unsigned kept { 0 };
	unsigned collected { 0 };
	Obj *prev { nullptr };
	Obj *cur { all_elements };
	while (cur) {
		auto mark { cur->get_mark() };
		if (cur != one && cur != zero && cur != two && cur != false_obj && mark != current_mark) {
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
