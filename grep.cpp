#include <iostream>
#include <fstream>
#include <exception>
#include <regex>
#include <string>
#include <cstring>
#include <Windows.h>

#define VERSION "0.0.1"

int optind = 1, // index of next argument to be processed 
    opterr = 1, // set to 0 to disable error messages
    optopt = 1; // character which caused error

char *nextchar = nullptr;
int first_arg = -1;

// Console Screen Buffer Info
HANDLE console;
WORD current_console_attr;
CONSOLE_SCREEN_BUFFER_INFO csbi;

const std::string options_long[] = {
	"version",
	"extended-regexp",
	"fixed-strings",
	"basic-regexp",
	"perl-regexp",
	"regexp",
	"file",
	"ignore-case",
	"invert-match",
	"word-regexp",
	"line-regexp"
};

const char *options = "VEFGPefivwx";

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

	while (std::getline(file, line)) {
		if(line.find(pattern) != std::string::npos) {
			std::cout << line << std::endl;
			match_found = true;
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
			match_found = true;
			std::cout << match.prefix();
			SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_GREEN);
			std::cout << match[0];
			SetConsoleTextAttribute(console, current_console_attr);
			std::cout << match.suffix() << std::endl;
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
			// TODO: parse full word options
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
		if(opt == 'V') { // version
			std::cout << VERSION << std::endl;
			return 0;
		}

		if(opt == 'G') { // basic-regexp
			flags |= std::regex_constants::basic;
		}

		if(opt == 'E') { // extended-regexp
			flags |= std::regex_constants::extended;
		}

		if(opt == 'F') { // fixed-strings
			useRegex = false;
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
