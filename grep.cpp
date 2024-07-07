#include <cctype>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <Windows.h>

#define VERSION "0.0.3"

int optind = 1, // index of next argument to be processed 
    opterr = 1, // set to 0 to disable error messages
    optopt = 1, // character which caused error
    previous_match = -1;

char *nextchar = nullptr;

int first_arg = -1;

bool files_with_matches = false,
     files_without_matches = false,
     ignore_case = false,
     invert_match = false,
     line_regex = false,
     multiple_patterns = false,
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
	auto flags = std::regex_constants::basic;
	int match,
	    matches_count = 0,
            line_number = 0;

	assert(patterns.size() > 0, "Expect patterns to have been found");

	while (std::getline(file, line)) {
		line_number++;

		pile = ignore_case ? to_lower(line.c_str()) : line;

		for(int i = 0; i < patterns.size(); i++) {
			if(use_regex) {
				if(ignore_case)
					flags |= std::regex_constants::icase;

				std::regex r_needle(patterns[i], flags);

				if(std::regex_search(line, r_match, r_needle)) {
					match_found = true;

					if(!invert_match && print_match_count) {
						matches_count++;
					}

					if(files_without_matches || files_with_matches || invert_match || print_match_count)
						break;

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

		switch(opt) {
			case 'V':
				std::cout << VERSION << std::endl;
				return 0;
			case 'G':
				flags = std::regex_constants::basic;
				break;
			case 'E':
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
				patterns.push_back(argv[optind+1]);
				break;
			case 'f':
				// TODO: implement -f option
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
			case 'o':
				only_matching = true;
				break;
			case 'h':
				use_default_print_file_name = false;
				print_file_name = true;
				break;
			case 'n':
				print_line_numbers = true;
				break;
		}
	}
	
	assert(argc >= 3, "expected minimum of two arguments. [usage]: grep <option(s)> <pattern> <file1> ...");
	
	if(patterns.size() > 1)
		multiple_patterns = true;

	// TODO: set print_file_name based on number of files when use_default_print_file_name is true
	try {
		for(int curr = optind+1; curr < argc; curr++) {
			if(multiple_patterns) {
				if(pattern_match(argv[curr])) {
					found = 0;
				}
			} else if(use_regex) {
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
