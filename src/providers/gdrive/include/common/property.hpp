#ifndef __COMMON_PROPERTY_HPP__
#define __COMMON_PROPERTY_HPP__

#include "common/misc.hpp"
#include "common/varstring.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace common {

class Property {
    public:
        Property(std::string filename)
            :_in(* new std::ifstream(filename.c_str())), _own(true)
        {
        }
        Property(std::istream& in)
            :_in(in), _own(false)
        {
            _property.clear();
            _parse();
        }
        ~Property() {
            if (_own) {
                ((std::ifstream*)(&_in))->close();
                delete &_in;
            }
        }

        std::string get(std::string key, std::string d = "") {
            if (_property.find(key) != _property.end())
                return _property[key];
            return d;
        }

        int get_int(std::string key, int d = 0) {
            std::string value = get(key);
            if (value == "") {
                return d;
            } else {
                return atoi(value.c_str());
            }
        }

        double get_real(std::string key, double d = 0.0) {
            std::string value = get(key);
            if (value == "") {
                return d;
            } else {
                return atof(value.c_str());
            }
        }

        bool get_bool(std::string key, bool d = false) {
            std::string value = get(key);
            if (value == "") {
                return d;
            } else {
                std::string lower = VarString::tolower(value);
                if (lower == "false" || lower == "0") return false;
                if (lower == "true" || lower == "1") return true;
                return d;
            }
        }

    private:
        std::istream& _in;
        bool _own;
        string_map _property;

        void _parse(){
            while(true) {
                std::string line;
                getline(_in, line);
                if (_in.good()) {
                    if (line == "" || line[0] == '#') {
                        continue;
                    } else {
                        std::vector<std::string> parts = VarString::split(line, "=");
                        if (parts.size() == 1) {
                            continue;
                        } else {
                            std::string key = VarString::strip(parts[0]);
                            std::string value = VarString::strip(parts[1]);
                            _property[key] = value;
                            std::cout << key << " " << value << std::endl;
                        }
                    }
                } else {
                    break;
                }
            }
        }
};

}

#endif
