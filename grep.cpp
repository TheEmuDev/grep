#include <iostream>
#include <fstream>
#include <exception>
#include <regex>
#include <cctype>
#include <string>
#include <cstring>
#include <Windows.h>

#define VERSION "0.0.3"

int optind = 1, // index of next argument to be processed 
    opterr = 1, // set to 0 to disable error messages
    optopt = 1; // character which caused error

char *nextchar = nullptr;
int first_arg = -1;

bool files_with_matches = false,
     files_without_matches = false,
     ignore_case = false,
     invert_match = false,
     line_regex = false,
     only_matching = false,
     print_file_name = false,
     print_line_numbers = false,
     print_match_count = false,
     use_default_print_file_name = true,
     use_regex = true,
     word_regex = false;

// Console Screen Buffer Info
HANDLE console;
WORD current_console_attr;
CONSOLE_SCREEN_BUFFER_INFO csbi;

const int options_length = 17;
const char *options = "VEFGLHefivwxclohn";
const std::string options_long[options_length] = {
	"version",			// DONE
	"extended-regexp",		// DONE
	"fixed-strings",		// DONE
	"basic-regexp",			// DONE
	"files-without-matches",	// DONE
	"with-filename",		// DONE
	"regexp",
	"file",
	"ignore-case",			// DONE
	"invert-match",			// DONE
	"word-regexp",			
	"line-regexp",			// DONE
	"count",			// DONE
	"files-with-matches",		// DONE
	"only-matching",
	"no-filename",			// DONE
	"line-number"			// DONE
};

void assert(const bool expression, const char* msg)
{
	if(!expression)
	{
		std::cerr <<"assertion failed: " << msg << std::endl;
		abort();
	}
}

std::string to_lower(const char* in) {
	std::string out = in;

	for(int i = 0; i < out.length(); i++) {
		if(std::isupper(out[i])) {
			out[i] = std::tolower(out[i]);
		}
	}

	return out;
}

bool pattern_match(const char* pattern, const char* filepath)
{
	bool match_found = false;
	std::fstream file(filepath);
	std::string line, needle, pile;
	int match,
	    matches_count = 0,
            line_number = 0;

	while (std::getline(file, line)) {
		line_number++;
		
		needle = ignore_case ? to_lower(pattern) : pattern;
		pile = ignore_case ? to_lower(line.c_str()) : line;

		match = line_regex ? std::strcmp(needle.c_str(), pile.c_str()) : pile.find(needle);

		if((!line_regex && match != std::string::npos) || (line_regex && match == 0)) {
			match_found = true;

			if(!invert_match && print_match_count) {
				matches_count++;
			}

			if(files_without_matches || files_with_matches) {
				break;
			}

			if(invert_match || print_match_count)
				continue;

			if(print_file_name) {
				std::cout << filepath << ":";
			}

			if(print_line_numbers) {
				std::cout << line_number << ":";
			}

			if(line_regex) {
				std::cout << line << std::endl;
			} else {
				if(!only_matching) 
					std::cout << line.substr(0, match);

				SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
				std::cout << line.substr(match, std::strlen(pattern));
				SetConsoleTextAttribute(console, current_console_attr);

				if(!only_matching)
					std::cout << line.substr(match + std::strlen(pattern));

				std::cout << std::endl;
			}
		}
		else if(invert_match && print_match_count) {
			matches_count++;
			continue;
		}
		else if(invert_match && !files_without_matches) {
			std::cout << line << std::endl;
		}
	}

	if(print_match_count) {
		std::cout << filepath << ": " << matches_count << std::endl;
	}

	if((files_without_matches && !match_found) || (files_with_matches && match_found)) {
		std::cout << filepath << std::endl;
	}
	
	file.close();
	return match_found;
}

bool pattern_match(const std::regex pattern, const char* filepath)
{
	bool match_found = false;
	std::fstream file(filepath);
	std::string line;
	std::smatch match;
	int matches_count = 0,
	    line_number = 0;

	while(std::getline(file, line)) {
		line_number++;
		if(std::regex_search(line, match, pattern)) {
			match_found = true;

			if(!invert_match && print_match_count) {
				matches_count++;
			}

			if(files_without_matches || files_with_matches)
				break;

			if(invert_match || print_match_count)
				continue;

			if(print_file_name) {
				std::cout << filepath << ":";
			}

			if(print_line_numbers) {
				std::cout << line_number << ":";
			}

			if(!only_matching)
				std::cout << match.prefix();

			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << match[0];
			SetConsoleTextAttribute(console, current_console_attr);

			if(!only_matching)
				std::cout << match.suffix();

			std::cout << std::endl;
		}
		else if(invert_match && print_match_count) {
			matches_count++;
			continue;
		}
		else if(invert_match && !files_without_matches) {
			std::cout << line << std::endl;
		}
	}

	if(print_match_count) {
		std::cout << filepath << ": " << matches_count << std::endl;
	}

	if((files_without_matches && !match_found) || (files_with_matches && match_found)) {
		std::cout << filepath << std::endl;
	}

	file.close();
	return match_found;
}

int getopt(int argc, char *argv[], const char *optarg) {
	if(optind >= argc)
		return -1;

	int optlen = std::strlen(optarg);

	if(!nextchar) {
		nextchar = argv[optind];
	}

	while(optind < argc) {
		if(strncmp(argv[optind], "--", 2) == 0) {
			nextchar = std::strstr(argv[optind], "--");
			nextchar += 2;
			optind++;
			return -2;
		}

		else if(strncmp(argv[optind], "-", 1) == 0) {
			nextchar++;

			if(!nextchar) {
				optind++;
				nextchar = argv[optind];
				continue;
			}

			for(int i = 0; i < optlen; ++i) {
				if(nextchar[0] == optarg[i]) {
					return optarg[i];
				}
			}

			// TODO: nextchar is unrecognized, swap to end of argv
		}
		
		else if(first_arg == -1) {
			first_arg = optind;
		}

		optind++;
	}

	optind = first_arg;

	return -1;
}

int main(int argc, char* argv[]) 
{
	int found = 1;
	int opt;
	auto flags = std::regex_constants::basic;

	// retrieve and save current console attributes
	console = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(console, &csbi)) {
		current_console_attr = csbi.wAttributes;
	}

	while((opt = getopt(argc, argv, options)) != -1) {
		if(opt == -2) {
			if(nextchar[0] == '\0')
				continue;
			
			if(std::strcmp(nextchar, "help") == 0) {
				std::cout << "[usage]: grep [OPTION...] PATTERNS [FILE...]" << std::endl;
				std::cout << "grep [OPTION...] -e PATTERNS ... [FILE...]" << std::endl;
				std::cout << "grep [OPTION...] -f PATTERN_FILE ... [FILE...]" << std::endl;
				return 0;
			}

			for(int i = 0; i < options_length; i++) {
				if(options_long[i].compare(nextchar) == 0)  {
					opt = options[i]; // TODO: handle --regex=PATTERNS
					break;
				}
			}
		}

		if(opt == 'V') { // version
			std::cout << VERSION << std::endl;
			return 0;
		}

		if(opt == 'G') { // basic-regexp
			flags = std::regex_constants::basic;
		}

		if(opt == 'E') { // extended-regexp
			flags = std::regex_constants::extended;
		}

		if(opt == 'F') { // fixed-strings
			use_regex = false;
		}
		
		if(opt == 'L') {
			files_without_matches = true;
		}
		
		if(opt == 'H') {
			use_default_print_file_name = false;
			print_file_name = true;
		}
		
		if(opt == 'l') {
			files_with_matches = true;
		}

		if(opt == 'i') {
			ignore_case = true;
			flags |= std::regex_constants::icase;
		}

		if(opt == 'v') {
			invert_match = true;
		}

		if(opt == 'x') {
			line_regex = true;
		}

		if(opt == 'c') {
			print_match_count = true;
		}

		if(opt == 'o') {
			only_matching = true;
		}

		if(opt == 'h') {
			use_default_print_file_name = false;
			print_file_name = false;
		}

		if(opt == 'n') {
			print_line_numbers = true;
		}

const char *options = "VEFGLHefivwxclohn";
	}
	
	assert(argc >= 3, "expected minimum of two arguments. [usage]: grep <option(s)> <pattern> <file1> ...");

	// TODO: set print_file_name based on number of files when use_default_print_file_name is true
	try {
		for(int curr = optind+1; curr < argc; curr++) {
			if(use_regex) {
				std::string pattern_text = argv[optind];

				if(line_regex) {
					pattern_text = "^(" + pattern_text + ")$";
				}

				std::regex regex_pattern(pattern_text, flags);

				if (pattern_match(regex_pattern, argv[curr])) {
					found = 0;
				}
			} else {
				if (pattern_match(argv[optind], argv[curr])) {
					found = 0;
				}
			}
		}
		return found;
	} catch (std::exception e) {
		return 2;
	}
}
