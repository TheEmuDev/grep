#include <iostream>
#include <fstream>
#include <exception>
#include <regex>
#include <string>
#include <cstring>
#include <Windows.h>

#define VERSION "0.0.2"

int optind = 1, // index of next argument to be processed 
    opterr = 1, // set to 0 to disable error messages
    optopt = 1; // character which caused error

char *nextchar = nullptr;
int first_arg = -1;

bool invertMatch = false;

// Console Screen Buffer Info
HANDLE console;
WORD current_console_attr;
CONSOLE_SCREEN_BUFFER_INFO csbi;

const int options_length = 10;
const char *options = "VEFGefivwx";
const std::string options_long[options_length] = {
	"version",
	"extended-regexp",
	"fixed-strings",
	"basic-regexp",
	"regexp",
	"file",
	"ignore-case",
	"invert-match",
	"word-regexp",
	"line-regexp",
};

void assert(const bool expression, const char* msg)
{
	if(!expression)
	{
		std::cerr <<"assertion failed: " << msg << std::endl;
		abort();
	}	
}

bool pattern_match(const char* pattern, const char* filepath)
{
	bool match_found = false;
	std::fstream file(filepath);
	std::string line;
	int match;

	while (std::getline(file, line)) {
		match = line.find(pattern);
		if(match != std::string::npos) {
			if(invertMatch)
				continue;

			match_found = true;
			std::cout << line.substr(0, match);
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << pattern;
			SetConsoleTextAttribute(console, current_console_attr);
			std::cout << line.substr(match + std::strlen(pattern)) << std::endl;
		}
		else if(invertMatch) {
			match_found = true;
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << line << std::endl;
			SetConsoleTextAttribute(console, current_console_attr);
		}
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

	while(std::getline(file, line)) {
		if(std::regex_search(line, match, pattern)) {
			if(invertMatch)
				continue;

			match_found = true;
			std::cout << match.prefix();
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << match[0];
			SetConsoleTextAttribute(console, current_console_attr);
			std::cout << match.suffix() << std::endl;
		}
		else if(invertMatch) {
			match_found = true;
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << line << std::endl;
			SetConsoleTextAttribute(console, current_console_attr);
		}
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
	bool useRegex = true;
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
			useRegex = false;
		}

		if(opt == 'i') {
			flags |= std::regex_constants::icase;
		}

		if(opt == 'v') {
			invertMatch = true;
		}
	}
	
	assert(argc >= 3, "expected minimum of two arguments. [usage]: grep <pattern> <file1> ...");

	try {
		for(int curr = 2; curr < argc; curr++) {
			if(useRegex) {
				std::regex regex_pattern(argv[optind], flags);
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
