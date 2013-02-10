
#ifndef _REGEX_H_
#define _REGEX_H_

#include <regex.h>
#include <string>
#include <stdexcept>

class Regex {
public:
    Regex(const std::string &_source)
    {
        pat_buff.translate = 0;
        pat_buff.fastmap = 0;
        pat_buff.buffer = 0;
        pat_buff.allocated = REGS_UNALLOCATED;

        re_syntax_options = RE_SYNTAX_EGREP; 

        if(re_compile_pattern(_source.c_str(), _source.size(), &pat_buff)) {
            throw std::runtime_error("error compiling regex");
        }
    }

    bool search(const std::string &haystack, std::string &needle) {
        int rc;

        rc = re_search(&pat_buff, haystack.c_str(), haystack.size(), 0, haystack.size(), &regs);

        if(rc < -1) {
            throw std::runtime_error("regex failure");
        }

        if(rc == -1) {
            return false;
        }

        needle.assign(haystack.substr(regs.start[0], regs.end[0] - regs.start[0]));

        return true;
    }

    bool match(const std::string &haystack, size_t &needle) {
        int rc;

        rc = re_search(&pat_buff, haystack.c_str(), haystack.size(), 0, haystack.size(), &regs);

        if(rc < -1) {
            throw std::runtime_error("regex failure");
        }

        if(rc == -1) {
            return false;
        }

        needle = rc;

        return true;
    }

    ~Regex()
    {
        regfree(&pat_buff);
    }

private:
    struct re_pattern_buffer pat_buff;
    struct re_registers regs;
};

#endif
