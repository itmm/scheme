/**
 * store Scheme objects in memory
 */

#pragma once

#include "types.h"
#include <map>

class Frame : public Obj {
		Frame *next_;
		std::map<std::string, Obj *> elements_;
	protected:
		void propagate_mark() override;
	public:
		Frame(Frame *next): next_ { next } { }
		void insert(const std::string &key, Obj *value);
		bool has(const std::string &key) const;
		Obj *get(const std::string &key) const;
		Obj *update(Symbol *key, Obj *value);
		std::ostream &write(std::ostream &out);
};

constexpr auto as_frame = Dynamic::as<Frame>;
constexpr auto is_frame = Dynamic::is<Frame>;

