
#include "range_parse.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

static std::string getLine(std::ifstream &fs) {
    std::stringstream ss;
    char c;

    while (fs.get(c) && fs.good()) {
        if (c == '\n')
            return ss.str();
        ss << c;
    }
    return ss.str();
}

RangeParse::RangeParse(std::string filename)
    :_valid(true)
{
    parse(filename);
}

void
RangeParse::parse(std::string filename)
{
    std::ifstream fs;

    fs.open(filename, std::ifstream::in);
    if (!fs.good()) {
        _valid = false;
        return;
    }
    do {
        auto line = getLine(fs);
        if (line.size() == 0) {
            break;
        }
        std::istringstream ss(line);
        std::string  type;
        SourceLocation loc;

        ss >> type >> loc.name >> loc.file;
        if (type == "func") {
            loc.kind = kFunction;
        } else if (type == "start") {
            ss >> loc.lineno;
            loc.kind = kStart;
        } else if (type == "end") { 
            loc.kind = kEnd;
            ss >> loc.lineno;
        } else {
            _valid = false;
            return;
        }
        _locations.push_back(loc);
#if 0


        size_t loc = line.find('-');
        // Just a function definition
        if (loc == std::string::npos) {
            loc = line.find(':');
            if (loc == std::string::npos) {
                _valid = false;
                return;
            }
            auto file = line.substr(0, loc);
            auto func = line.substr(loc+1);

            _functions.push_back(std::tuple<std::string, std::string>(file, func));
        // range definition
        } else {
            auto from = line.substr(0, loc);
            auto to = line.substr(loc+1);

            auto parse = [this](std::string line) -> SourceLocation {
                SourceLocation sl;
                size_t loc = line.find(':');

                if (loc == std::string::npos) {
                    _valid = false;
                } else {
                    sl.file = line.substr(0, loc);
                    sl.lineno = std::stoi(line.substr(loc+1));
                }
                return sl;
            };

            auto slfrom = parse(from);
            if (!_valid)
                return;
            auto slto = parse(to);
            if (!_valid)
                return;

            _locations.push_back(std::tuple<SourceLocation, SourceLocation>(slfrom, slto));
        }
#endif

    } while (true);
}

#ifdef TESTING

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [file]\n";
        return 1;
    }
    RangeParse p(argv[1]);
    return 0;
}
#endif
