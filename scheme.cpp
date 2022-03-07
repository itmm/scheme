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

std::ostream *prompt { nullptr };
std::ostream *result { nullptr };

void process_stream(std::istream &in) {
	active_frames.clear();
	active_frames.push_back(&initial_frame);

	if (prompt) { *prompt << "? "; }
	ch = in.get();
	if (ch == '#') {
		while (ch != EOF && ch != '\n') { ch = in.get(); }
	}
	for (;;) {
		auto exp { read_expression(in) };
		if (! in) { break; }
		exp = eval(exp, &initial_frame);
		if (result) { *result << exp << "\n"; }
		if (prompt) { *prompt << "? "; }
	}
}

void process_stdin() {
	auto old_prompt { prompt };
	auto old_result { result };
	prompt = &std::cout;
	result = &std::cout;
	process_stream(std::cin);
	result = old_result;
	prompt = old_prompt;
}

#include <fstream>

int main(int argc, const char *argv[]) {
	setup_primitives();
	{
		std::istringstream s { 
			#include "scheme.scm.h"
		};
		process_stream(s);
	}

	if (argc > 1) {
		for (int i { 1 }; i < argc; ++i) {
			if (argv[i] == std::string { "-" }) {
				process_stdin();
			} else {
				std::ifstream in { argv[i] };
				auto old_result { result };
				result = &std::cout;
				process_stream(in);
				result = old_result;
			}
		}
	} else {
		process_stdin();
	}
}
