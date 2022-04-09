/**
 * simple Scheme interpreter with the following priorities
 *
 * 1. completness
 * 2. small source code size
 * 3. speed
 */

#include "parser.h"
#include "err.h"
#include "eval.h"
#include "frame.h"
#include "primitives.h"

std::ostream *prompt { nullptr };
std::ostream *result { nullptr };

void process_stream(std::istream &in, bool with_header, bool exit_on_exception) {
	active_frames.clear();
	active_frames.push_back(&initial_frame);

	if (prompt) { *prompt << "? "; }
	int ch = get(in);
	if (with_header && ch == '#') {
		while (ch != EOF && ch != '\n') { ch = get(in); }
	}
	for (;;) {
		try {
			auto exp { read_expression(in) };
			if (! in) { break; }
			exp = eval(exp, &initial_frame);
			if (result) { *result << exp << "\n"; }
		} catch (Error *err) {
			if (err_stream) { *err_stream << err << '\n'; }
			if (exit_on_exception) { return; }
		}
		if (prompt) { *prompt << "? "; }
	}
}

void process_stdin() {
	auto old_prompt { prompt };
	auto old_result { result };
	prompt = &std::cout;
	result = &std::cout;
	process_stream(std::cin, false, false);
	result = old_result;
	prompt = old_prompt;
}

void print_help() {
	std::cout << "Usage: scheme [ --help ] [ FILE ]...\n"
		"Interpret the Scheme FILEs.\n\n"
		"Use standard input, if no files are specified or if - is\n"
		"used as a file name.\n\n"
		"    --help   display this help and exit\n";
}

#include <fstream>

int main(int argc, const char *argv[]) {
	one = Integer::create(1);
	two = Integer::create(2);
	zero = Integer::create(0);
	setup_primitives();
	{
		std::istringstream s { 
			#include "scheme.scm.h"
		};
		process_stream(s, true, true);
	}
	syntax_tests();
	if (argc > 1) {
		for (int i { 1 }; i < argc; ++i) {
			if (argv[i] == std::string { "--help" }) {
				print_help();
				break;
			} else if (argv[i] == std::string { "-" }) {
				process_stdin();
			} else {
				std::ifstream in { argv[i] };
				auto old_result { result };
				result = &std::cout;
				process_stream(in, true, true);
				result = old_result;
			}
		}
	} else {
		process_stdin();
	}
}
