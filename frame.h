/**
 * store Scheme objects in memory
 */

#include <map>

class Frame : public Element {
		Frame *next_;
		std::map<std::string, Element *> elements_;
	protected:
		void propagate_mark() override {
			for (auto &v : elements_) {
				mark(v.second);
			}
			mark(next_);
		}
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Element *value);
		bool has(const std::string &key) const;
		Element *get(const std::string &key) const;
		std::ostream &write(std::ostream &out) const override {
			return out << "#frame";
		}
};

void Frame::insert(const std::string &key, Element *value) {
	elements_[key] = value;
}

bool Frame::has(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? true :
		next_ ? next_->has(key) : false;
}

Element *Frame::get(const std::string &key) const {
	auto it { elements_.find(key) };
	return it != elements_.end() ? it->second :
		next_ ? next_->get(key) : nullptr;
}