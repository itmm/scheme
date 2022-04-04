/**
 * store Scheme objects in memory
 */

class Frame : public Obj {
		Frame *next_;
		std::map<std::string, Obj *> elements_;
	protected:
		void propagate_mark() override {
			for (auto &v : elements_) {
				mark(v.second);
			}
			mark(next_);
		}
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Obj *value);
		bool has(const std::string &key) const;
		Obj *get(const std::string &key) const;
		Obj *update(Symbol *key, Obj *value);
		std::ostream &write(std::ostream &out) override {
			return out << "#frame";
		}
};

inline auto as_frame(Obj *obj) { return dynamic_cast<Frame *>(obj); }
inline bool is_frame(Obj *obj) { return as_frame(obj); }

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
