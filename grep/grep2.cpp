#include <regex>
#include <iostream>
#include <string>

#include "Logger.hpp"

struct Args
{
	uint8_t opts = 0;
	static constexpr uint8_t NoCase = 1 << 0; //i
	static constexpr uint8_t Fixed = 1 << 1; //F
	static constexpr uint8_t Invert = 1 << 2; //v

	static constexpr uint8_t After = 1 << 3; //A
	static constexpr uint8_t Before = 1 << 4; //B
	static constexpr uint8_t Both = 1 << 5; //C

	int aN = 0;
	int bN = 0;
	char* pattern = NULL;
};

bool isnum(unsigned char c) {
	return (c <= '9' && c >= '0');
}

int cheap_atoi(char *s, int len) {
	int num = 0;
	int i = -1;

	while (++i < len) {
		num *= 10;
		num += s[i] - '0';
	}
	return num;
}

int parse_args(int argc, char **argv, Args& o) {
	int seen_pat = 0;
	int need_num = 0;

	for (int i = 1; i < argc; i++) { // each word
		if (need_num && !isnum(argv[i][0])) {LOG(LOG_ERROR, "Expected number: " << argv[i]); return 1;}
		if (need_num) {
			int j = 0;
			while (argv[i][++j] != '\0') { 
				char c = argv[i][j];
				if (!isnum(c)) {LOG(LOG_ERROR, "Invalid num: " << argv[i]); return 1;} 
			}
			if (j > 5) {LOG(LOG_ERROR, "Brooo ... size does matter: " << argv[i]); return 1;};
			o.N = cheap_atoi(argv[i], j);
			LOG(LOG_WARN, "Got N: " << o.N);
			need_num = 0;
			continue;
		}
		if (argv[i][0] == '-' && !seen_pat) {
			int j = 1;
			while (argv[i][j] != '\0') { // nums come as next arg
				char c = argv[i][j];
				switch (c) {
					case 'i' : o.opts |= Args::NoCase; break;
					case 'F' : o.opts |= Args::Fixed; break;
					case 'v' : o.opts |= Args::Invert; break;
					case 'A' : o.opts |= Args::After; need_num = 1; break;
					case 'B' : o.opts |= Args::Before; need_num = 1; break;
					case 'C' : o.opts |= Args::Both; need_num = 1; break;
					default: LOG(LOG_ERROR, "Invalid option: " << std::string() + c); return 1;
				}
				j++;
			}
		} else if (argv[i][0] == '-' && seen_pat) { //check position
			LOG(LOG_ERROR, "Invalid arg: " << std::string(argv[i]));
			return 1;
		} else if (!seen_pat) { // empty pattern ok
			seen_pat = 1;
			o.pattern = argv[i];
			LOG(LOG_WARN, "Got pattern: " << std::string(o.pattern));
		}
	}
	if (0 == seen_pat) {LOG(LOG_ERROR, "No pattern given"); return 1;}
	return 0;
}


void flush(std::vector<std::string>& ring, int idx, int count, const Args& o) {
	int start = idx - count;
	start = (start < 0) ? start + o.N : start;

	LOG(LOG_WARN, "flushing: " << ring[start]);
	int i = 0;
	while (i++ < count) {
		std::cout << ring[start] << "\n";
		start = (start + 1) % o.N;
	}
}

int do_ops(const Args& o) {
	int do_find = 0;
	int invert = 0;
	std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;

	if (o.opts & Args::NoCase) flags |= std::regex::icase;
	if (o.opts & Args::Invert) invert = 1;
	if (o.opts & Args::Fixed) do_find = 1;

	std::regex pat(o.pattern, flags);

	std::vector<std::string> ring(o.N); //calloc
	int idx=0, count=0;

	int afters = 0;
	bool out_after = (o.opts & Args::After || o.opts & Args::Both) ? true : false;
	bool out_before = (o.opts & Args::Before || o.opts & Args::Both) ? true : false;
	if (out_after) afters = o.N;

	LOG(LOG_WARN, "A: " << out_after << ", " << "B: " << out_before);
	for (std::string line; std::getline(std::cin, line);) {

		std::stringstream out;
		size_t last = 0;
		bool got_match = false;

		if (do_find)
			got_match = (std::string::npos != line.find(o.pattern));

		if (!do_find) {
			// highlight matches
			auto begin = 	std::sregex_iterator(line.begin(), line.end(), pat);
			auto end = 	std::sregex_iterator();


			for (auto it = begin; it != end; ++it) {
				auto match = *it;

				out << line.substr(last, match.position() - last);
				out << "\033[31m";
				out << match.str();
				out << "\033[0m";

				last = match.position() + match.length();
			}
			if (last > 0) got_match = true;
		}

		if (!got_match && invert) got_match = true;

		// no match, cache it (or print it)
		if (!got_match || invert) {
			if (out_after && afters > 0) {
				std::cout << line << "\n";
				--afters;
				continue;
			}

			ring[idx] = std::move(line);
			LOG(LOG_WARN, "Cached: " << ring[idx]);
			idx = (idx + 1 ) % o.N;
			count = std::min(count + 1, o.N);
			continue;
		}
		assert(got_match);
		// got match, print it
		out << line.substr(last);
		if (out_after) afters = o.N; //reset counter

		if (out_before) flush(ring, idx, count, o);
		std::cout << out.str() << "\n";

	}
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		LOG(LOG_WARN, "Usage: input | grep [-iFvABC] pattern");
		return 1;
	}

	Args o = {};
	if (0 != parse_args(argc, argv, o))
		return 1;

	try {
		do_ops(o);
	} catch (std::exception& e) {
		return 1;
	}

	return 0;
}
