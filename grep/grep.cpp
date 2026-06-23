#include <regex>
#include <iostream>
#include <string>

#include "Logger.hpp"

// TODO, change stringstream to stringview

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
	enum PendingNum {
		None,
		After,
		Before,
		Both
	};

	int seen_pat = 0;
	PendingNum need_num = None;
	int num;

	for (int i = 1; i < argc; i++) { // each word
		if (need_num != None && !isnum(argv[i][0])) {
			LOG(LOG_ERROR, "Expected context length number: " << argv[i]); 
			return 1;
		}	
		if (need_num != None) {
			int j = 0;
			while (argv[i][++j] != '\0') { 
				char c = argv[i][j];
				if (!isnum(c)) {LOG(LOG_ERROR, "Invalid num: " << argv[i]); return 1;} 
			}
			if (j > 5) {LOG(LOG_ERROR, "Brooo ... size does matter: " << argv[i]); return 1;};
			num = cheap_atoi(argv[i], j);
			LOG(LOG_WARN, "Got N: " << num);
			switch (need_num) {
				case Both : o.aN = num; o.bN = num; break;
				case Before : o.bN = num; break;
				case After : o.aN = num; break;
				default : break;
			}
			need_num = None;
			continue;
		}
		if (argv[i][0] == '-' && !seen_pat) {
			int j = 1;
			while (argv[i][j] != '\0') { // nums come as next arg
				char c = argv[i][j];
				if (need_num != None) {LOG(LOG_ERROR, "Expected context length number"); return 1;}
				switch (c) {
					case 'i' : o.opts |= Args::NoCase; break;
					case 'F' : o.opts |= Args::Fixed; break;
					case 'v' : o.opts |= Args::Invert; break;
					case 'A' : o.opts |= Args::After; need_num = After; break;
					case 'B' : o.opts |= Args::Before; need_num = Before; break;
					case 'C' : o.opts |= Args::Both; need_num = Both; break;
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


void flush(std::vector<std::string>& ring, int idx, int& count, const Args& o) {
	int start = idx - count;
	start = (start < 0) ? start + o.bN : start;

	LOG(LOG_WARN, "flushing (count: " << count << "): " << ring[start]);
	int i = 0;
	while (i++ < count) {
		std::cout << ring[start] << "\n";
		ring[start].clear(); // TODO do I need this?
		start = (start + 1) % o.bN;
	}
	count = 0;
}

struct MatchRes {
	bool got_match = false;
	size_t last = 0;
};

MatchRes match(const std::string& line, std::stringstream& out, const std::regex& pat, const Args& o) {
	MatchRes r;
	int do_find = (o.opts & Args::Fixed);

	if (do_find) {
		size_t pos = line.find(o.pattern);
		const std::string pat(o.pattern);
		while (pos != std::string::npos) {
			out << line.substr(r.last, pos);
			out << "\033[31m";
			out << pat;
			out << "\033[0m";

			r.last = pos + pat.length();
			pos = line.find(pat, r.last);
		}
	}

	if (!do_find) {
		// highlight matches
		auto begin = 	std::sregex_iterator(line.begin(), line.end(), pat);
		auto end = 	std::sregex_iterator();


		for (auto it = begin; it != end; ++it) {
			auto match = *it;

			out << line.substr(r.last, match.position() - r.last);
			out << "\033[31m";
			out << match.str();
			out << "\033[0m";

			r.last = match.position() + match.length();
		}
	}
	if (r.last > 0) r.got_match = true;
	return r;
}

int do_ops(const Args& o) {
	int invert = (o.opts & Args::Invert);

	std::regex_constants::syntax_option_type flags = std::regex::ECMAScript;
	if (o.opts & Args::NoCase) flags |= std::regex::icase;
	std::regex pat(o.pattern, flags);

	std::vector<std::string> ring(o.bN); //calloc
	int idx=0, count=0;

	int exitc = 1;
	int dist = 0;
	int afters = 0;
	const bool out_after = o.opts & (Args::After | Args::Both);
	const bool out_before = o.opts & (Args::Before | Args::Both);

	LOG(LOG_WARN, "A: " << o.aN << ", " << "B: " << o.bN << ", F: " << 1 *(o.opts & Args::Fixed));
	for (std::string line; std::getline(std::cin, line);) {
		std::stringstream out;

		MatchRes r = match(line, out, pat, o); 
		if (out.tellp() > 0) out << line.substr(r.last);

		if (invert) r.got_match = r.got_match ? false : true;

		// no match, cache it (or print it)
		if (!r.got_match) {
			++dist;
			if (out_after && afters > 0) {
				if (out.tellp() > 0)
					std::cout << out.str() << "\n";
				else
					std::cout << line << "\n";
				--afters;
				continue;
			}
			if (out_before) {
				if (out.tellp() > 0)
					ring[idx] = std::move(out.str());
				else
					ring[idx] = std::move(line);
				LOG(LOG_WARN, "Cached: " << ring[idx]);
				idx = (idx + 1 ) % o.bN;
				count = std::min(count + 1, o.bN);
			}
			continue;
		}
		exitc = 0;

		// got match, print it
		if (out_after) afters = o.aN; //set/reset counter
		if (out_before) {
			if (dist > o.aN + o.bN)
				std::cout << "\033[36m" << "--" << "\033[0m" << "\n";
			flush(ring, idx, count, o);
		}
		dist = 0;
		if (out.tellp() > 0)
			std::cout << out.str() << "\n";
		else
			std::cout << line << "\n";

	}
	return exitc;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		LOG(LOG_WARN, "Usage: input | grep [-iFvABC] pattern");
		return 1;
	}

	Args o = {};
	if (0 != parse_args(argc, argv, o))
		return 1;

	int res = 0;
	try {
		res = do_ops(o);
	} catch (std::exception& e) {
		return 1;
	}

	return res;
}
