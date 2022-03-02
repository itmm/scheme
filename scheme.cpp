/**
 * simple Scheme interpreter with the following priorities
 *
 * 1. completness
 * 2. small source code size
 * 3. speed
 */

#include "element.h"
#include "err.h"
#include "types.h"
#include "parser.h"
#include "frame.h"
#include "eval.h"
#include "primitives.h"
#include "garbage-collect.h"

void process_stream(std::istream &in, std::ostream *out, bool prompt) {
	active_frames.clear();
	active_frames.push_back(&initial_frame);

	if (prompt && out) { *out << "? "; }
	ch = in.get();
	if (ch == '#') {
		while (ch != EOF && ch != '\n') { ch = in.get(); }
	}
	for (;;) {
		auto exp { read_expression(in) };
		if (! exp) { break; }
		exp = eval(exp, &initial_frame);
		if (out) { *out << exp << "\n"; }
		if (prompt && out) { *out << "? "; }
	}
}

#include <fstream>

int main(int argc, const char *argv[]) {
	setup_primitives();
	{
		std::istringstream s { 
			#include "scheme.scm.h"
		};
		process_stream(s, nullptr, false);
	}

	if (argc > 1) {
		for (int i { 1 }; i < argc; ++i) {
			if (argv[i] == std::string { "-" }) {
				process_stream(std::cin, &std::cout, true);
			} else {
				std::ifstream in { argv[i] };
				process_stream(in, &std::cout, false);
			}
		}
	} else {
		process_stream(std::cin, &std::cout, true);
	}
}
