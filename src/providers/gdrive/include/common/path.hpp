#ifndef __common_PATH_HPP__
#define __common_PATH_HPP__

#include "common/varstring.hpp"
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <iostream>
#include <fstream>

namespace common {
    
class Path {
    public:
        static inline std::string sep() {
#ifdef Win32
            static std::string _sep = "\\";
#else
            static std::string _sep = "/";
#endif
            return _sep;
        }

#ifndef Win32
        static inline std::string devnull() {
            static std::string _devnull = "/dev/null";
            return _devnull;
        }
#endif

        static inline std::string curdir() {
            static std::string _curdir = ".";
            return _curdir;
        }

        static inline std::string parentdir() {
            static std::string _parentdir = "..";
            return _parentdir;
        }

        static inline std::string pathsep() {
#ifdef Win32
            static std::string _pathsep = ";";
#else
            static std::string _pathsep = ":";
#endif
            return _pathsep;
        }

        static std::pair<std::string, std::string> split(const std::string& path) {
            std::string::size_type pos = path.rfind(Path::sep());
            std::string dir = "", base = "";
            if (pos != std::string::npos) {
                dir = path.substr(0, pos);
                pos ++;
                base = path.substr(pos);
            } else {
                base = path;
            }
            return make_pair(dir, base);
        }

        static std::string join(const std::string& dir, const std::string & base) {
            std::string rst = "";
            if (VarString::starts_with(base, Path::sep())) {
                rst = base;
            } else {
                if (VarString::ends_with(dir, Path::sep())) {
                    rst = dir + base;
                } else {
                    rst = dir + Path::sep() + base;
                }
            }
            return rst;
        }

        static std::string basename(const std::string& path) {
            return Path::split(path).first;
        }

        static std::string dirname(const std::string& path) {
            return Path::split(path).second;
        }

        static std::pair<std::string, std::string> splitext(const std::string& path) {
            std::string::size_type pos = path.rfind(".");
            std::string name = "", ext = "";
            if (pos != std::string::npos) {
                name = path.substr(0, pos);
                ext = path.substr(pos);
            } else {
                name = path;
            }
            return make_pair(name, ext);
        }

        static bool exists(const std::string& path) {
            std::ifstream  fin(path.c_str());
            bool rst = fin.good();
            fin.close();
            return rst;
        }
};

}

#endif
