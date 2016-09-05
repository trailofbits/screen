#ifndef __RANGE_PARSE_H
#define __RANGE_PARSE_H

#include <string>
#include <tuple>
#include <vector>

class RangeParse {
public:
    enum Kind {
        kFunction,
        kStart,
        kEnd
    };

    struct SourceLocation {
        std::string file;
        std::string name;
        unsigned long lineno;
        Kind kind;
        bool added = false;
    };

    RangeParse(std::string filename);

    bool valid() const { return _valid; }

    std::vector<SourceLocation> &locations() {
        return _locations;
    }

private:
    void parse(std::string filename);
    std::vector<SourceLocation> _locations;
    bool _valid;
};


#endif // __RANGE_PARSE_H
