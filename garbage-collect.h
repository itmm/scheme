/**
 * implementation of the garbage collection
 */

std::pair<unsigned, unsigned> Obj::garbage_collect() {
	std::swap(Obj::current_marked_, Obj::other_marked_);
	
	for (auto &s : syntax_extensions) { (*s.second).mark(); }
	for (auto &f : active_frames) { f->mark(); }
	for (auto &e : active_elements) { e->mark(); }
	one->mark(); zero->mark(); two->mark();
	false_obj->mark(); true_obj->mark();

	unsigned kept = current_marked_->size();
	unsigned collected = other_marked_->size();
	std::vector<Obj *> to_delete;
	for (auto obj : *other_marked_) {
		to_delete.push_back(obj);
	}
	other_marked_->clear();
	for (auto obj : to_delete) { delete obj; }

	return { collected, kept };
}
