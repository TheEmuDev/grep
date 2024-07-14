#include <cctype>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <Windows.h>

#define VERSION "0.0.4"

int optind = 1, // index of next argument to be processed 
    previous_match = -1;

char *nextchar = nullptr;

int first_arg = -1;

bool basic_regex = true,
     files_with_matches = false,
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

std::vector<std::string> patterns;

// Console Screen Buffer Info
HANDLE console;
WORD current_console_attr;
CONSOLE_SCREEN_BUFFER_INFO csbi;

const int options_length = 18;
const char *options = "VEFGLHefivwxclmohn";
const std::string options_long[options_length] = {
	"version",			// DONE
	"extended-regexp",		// DONE
	"fixed-strings",		// DONE
	"basic-regexp",			// DONE
	"files-without-matches",	// DONE
	"with-filename",		// DONE
	"regexp",			// DONE
	"file",				// DONE
	"ignore-case",			// DONE
	"invert-match",			// DONE
	"word-regexp",			
	"line-regexp",			// DONE
	"count",			// DONE
	"files-with-matches",		// DONE
	"max-count",
	"only-matching",		// DONE
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

void print_patterns() {
	std::cout << "patterns found: ";

	for(auto it = patterns.begin(); it != patterns.end(); it++) {
		std::cout << *it << " ";
	}
	
	std::cout << std::endl;
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

bool pattern_match(const char* filepath)
{
	bool match_found = false;
	std::fstream file(filepath);
	std::string line, needle, pile;
	std::smatch r_match;
	auto flags = basic_regex ? std::regex_constants::basic : std::regex_constants::extended;
	int match,
	    matches_count = 0,
            line_number = 0;

	assert(patterns.size() > 0, "Expect patterns to have been found");

	while (std::getline(file, line)) {
		line_number++;

		pile = ignore_case ? to_lower(line.c_str()) : line;

		for(int i = 0; i < patterns.size(); i++) {
			if(use_regex) {
				if(ignore_case) {
					flags |= std::regex_constants::icase;
				}

				std::regex r_needle(patterns[i], flags);

				if(std::regex_search(line, r_match, r_needle)) {
					match_found = true;

					if(!invert_match && print_match_count) {
						matches_count++;
					}

					if(files_without_matches || files_with_matches || invert_match || print_match_count) {
						break;
					}

					if(print_file_name) {
						std::cout << filepath << ":";
					}

					if(print_line_numbers) {
						std::cout << line_number << ":";
					}

					if(!only_matching)
						std::cout << r_match.prefix();

					SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
					std::cout << r_match[0];
					SetConsoleTextAttribute(console, current_console_attr);

					if(!only_matching)
						std::cout << r_match.suffix();

					std::cout << std::endl;
					
					break;
				}
			} else {
				needle = ignore_case ? to_lower(patterns[i].c_str()) : patterns[i];

				match = line_regex ? std::strcmp(needle.c_str(), pile.c_str()) : pile.find(needle);

				if((!line_regex && match != std::string::npos) || (line_regex && match == 0)) {
					match_found = true;

					if(!invert_match && print_match_count) {
						matches_count++;
					}
				
					if(files_without_matches || files_with_matches || invert_match || print_match_count) {
						break;
					}

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
						std::cout << line.substr(match, patterns[i].length());
						SetConsoleTextAttribute(console, current_console_attr);

						if(!only_matching)
							std::cout << line.substr(match + patterns[i].length());

						std::cout << std::endl;
					}

					break;
				}
			}
		}

		if((files_with_matches || files_without_matches) && match_found) {
			break;
		}

		if(!match_found && invert_match && print_match_count) {
			matches_count++;
			continue;
		}

		else if(invert_match && !files_without_matches) {
			std::cout << line << std::endl;
		}
	}

	if(print_match_count) {
		std::cout << filepath << ":" << matches_count << std::endl;
	}

	if((files_without_matches && !match_found) || (files_with_matches && match_found)) {
		std::cout << filepath << std::endl;
	}
	
	file.close();
	return match_found;
}

void get_patterns_from_input(const char *in) {
	bool found_multiple_patterns = false;
	int offset = 0, previous_offset = 0;
	std::string input = in;

	while((offset = input.find('\n', previous_offset)) != std::string::npos) {
		found_multiple_patterns = true;
		patterns.push_back(input.substr(previous_offset, offset));

		previous_offset = offset+1 < input.length() ? offset+1 : input.length()-1;
	}

	patterns.push_back(found_multiple_patterns ? input.substr(previous_offset) : input);
}

void get_patterns_from_file(const char *filepath) {
	if(std::strcmp(filepath, "-") == 0) {
		// TODO: Implement reading from stdin
	} else {
		std::fstream file(filepath);
		std::string line;

		while(std::getline(file, line)) {
			patterns.push_back(line);
		}

		file.close();
	}
}

std::string generate_regex_from_file() {
	assert(patterns.size() > 0, "Expect patterns to have been read from file");
	std::string pattern = "";

	for(int i = 0; i < patterns.size(); i++) {
		if(i == 0) {
			pattern = line_regex ? "^(" + patterns[i] + ")$" : patterns[i];
		} else {
			pattern = line_regex ? pattern + "|" + "^(" + patterns[i] + ")$" : pattern + "|" + patterns[i];
		}
	}

	return pattern;
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

			return -2; // -2 means long param, nextchar set to long param
		}

		else if(strncmp(argv[optind], "-", 1) == 0) {
			if(nextchar < argv[optind])
				nextchar = argv[optind];

			nextchar++; 

			if(!nextchar || nextchar[0] == '\0') {
				++optind;
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
		nextchar = argv[optind];
	}

	optind = first_arg;

	return -1;
}

int main(int argc, char* argv[]) 
{
	auto flags = std::regex_constants::basic;
	bool any_options_with_params = false,
	     long_option_with_param = false;
	int found = 1,
	    opt,
	    long_opt_with_param_counter = 0,
	    opt_with_param_counter = 0,
	    opt_count = 0,
	    file_offset = 0;

	std::string option, param;

	// retrieve and save current console attributes
	console = GetStdHandle(STD_OUTPUT_HANDLE);
	if(GetConsoleScreenBufferInfo(console, &csbi)) {
		current_console_attr = csbi.wAttributes;
	}

	while((opt = getopt(argc, argv, options)) != -1) {
		opt_count++;
		if(opt == -2) {
			if(nextchar[0] == '\0')
				continue;
			
			if(std::strcmp(nextchar, "help") == 0) {
				std::cout << "[usage]:\ngrep [OPTION...] PATTERNS [FILE...]" << std::endl;
				std::cout << "grep [OPTION...] -e PATTERNS ... [FILE...]" << std::endl;
				std::cout << "grep [OPTION...] -f PATTERN_FILE ... [FILE...]" << std::endl;
				return 0;
			}
			
			long_option_with_param = std::strcspn(nextchar, "=") < std::strlen(nextchar);

			option = nextchar;
			int param_offset = option.find("=");

			for(int i = 0; i < options_length; i++) {
				if(long_option_with_param) {
	 				if(options_long[i].compare(option.substr(0, param_offset)) == 0) {
						long_opt_with_param_counter++;
						opt = options[i];
						param = option.substr(param_offset+1);
						break;
					}
				} else if(options_long[i].compare(nextchar) == 0)  {
					if(options[i] == 'e' || options[i] == 'f' || options[i] == 'm') {
						opt_with_param_counter++;
					}
					opt = options[i];
					break;
				}
			}
		}

		switch(opt) {
			case 'V':
				std::cout << VERSION << std::endl;
				return 0;
			case 'G':
				basic_regex = true;
				flags = std::regex_constants::basic;
				break;
			case 'E':
				basic_regex = false;
				flags = std::regex_constants::extended;
				break;
			case 'F':
				use_regex = false;
				break;
			case 'L':
				files_without_matches = true;
				break;
			case 'H':
				use_default_print_file_name = false;
				print_file_name = true;
				break;
			case 'e':
				patterns.push_back(long_option_with_param ? param : argv[optind+1]);
				opt_with_param_counter = long_option_with_param ? opt_with_param_counter : opt_with_param_counter + 1;
				any_options_with_params = true;
				long_option_with_param = false;
				break;
			case 'f':
				get_patterns_from_file(long_option_with_param ? param.c_str() : argv[optind+1]);
				opt_with_param_counter = long_option_with_param ? opt_with_param_counter : opt_with_param_counter + 1;
				any_options_with_params = true;
				long_option_with_param = false;
				break;
			case 'i':
				ignore_case = true;
				flags |= std::regex_constants::icase;
				break;
			case 'v':
				invert_match = true;
				break;
			case 'w':
				// TODO: implement -w option
				break;
			case 'x':
				line_regex = true;
				break;
			case 'c':
				print_match_count = true;
				break;
			case 'l':
				files_with_matches = true;
				break;
			case 'm':// TODO: implement -m option
				break;
			case 'o':
				only_matching = true;
				break;
			case 'h':
				use_default_print_file_name = false;
				print_file_name = false;
				break;
			case 'n':
				print_line_numbers = true;
				break;
		}
	}
	
	assert(argc >= 3, "expected minimum of two arguments. [usage]: grep <option(s)> <pattern1> <file1> ...");

	if(!any_options_with_params) {
		get_patterns_from_input(argv[optind]);
	}
	
	print_patterns();
	// optind will point to first param that doesn't start with '-'
	
	if(opt_with_param_counter > 0) {
		// .\grep.exe -e "stuff" -e "things" .\grep-test.txt
		file_offset += 2 * opt_with_param_counter;
	}
	
	if(long_opt_with_param_counter > 0) {
		// .\grep.exe --regexp=stuff .\grep-test.txt
		file_offset += long_opt_with_param_counter;
	}

	file_offset = (opt_count > opt_with_param_counter + long_opt_with_param_counter) ? file_offset + 2 : file_offset + 1;
	
	assert(file_offset < argc, "Expect that file offset does not go out of bounds");

	// Display file name by default if searching for more than one file
	if(argc - file_offset > 1 && use_default_print_file_name) {
		print_file_name = true;
	}

	try {
		for(int curr = file_offset; curr < argc; curr++) {
			if(pattern_match(argv[curr])) {
				found = 0;
			}
		}
		return found;
	} catch (std::exception e) {
		return 2;
	}
}
