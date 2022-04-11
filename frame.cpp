#include "frame.h"

void Frame::propagate_mark() {
	for (auto &[key, obj] : elements_) {
		mark(obj);
	}
	mark(next_);
}

std::ostream &Frame::write(std::ostream &out) {
	return out << "#frame";
}

void Frame::insert(const std::string &key, Obj *value) {
	elements_[key] = value;
}

bool Frame::has(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? true :
		next_ ? next_->has(key) : false;
}

Obj *Frame::get(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? it->second :
		next_ ? next_->get(key) : nullptr;
}

Obj *Frame::update(Symbol *key, Obj *value) {
	ASSERT(key, "update");
	auto it { elements_.find(key->value()) };
	if (it != elements_.end()) {
		it->second = value;
		return value;
	} else if (next_) {
		return next_->update(key, value);
	} else {
		err("update", "not found", key);
		return nullptr;
	}
}
