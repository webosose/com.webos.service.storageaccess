// The MIT License (MIT)
//
// Copyright (c) 2014 Justin (Jianfeng) Lin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __GDRIVE_UTIL_HPP__
#define __GDRIVE_UTIL_HPP__
#include <string>
#include <map>
#include <set>
#include <vector>
#include <cctype>
#include <string.h>
#include <stdlib.h>
#include "common/all.hpp"

#define UNSAFE " $&+,/:;=@\"<>#%{}|\\^~[]`"

namespace GDRIVE {

class URLHelper {
    public:
        static bool check_unsafe(char c) {
            const char* p = UNSAFE;
            while (*p != '\0') {
                if (*p++ == c) return true;
            }
            return false;
        }
        static std::string encode(std::string url) {
            return encode(url.c_str());
        }

        static std::string encode(const char* url) {
            COMMON::VarString vs;
            const char* p = url;
            while (*p != '\0') {
                if (check_unsafe(*p)) {
                    vs.append('%');
                    char tmp[4] = "";
                    sprintf(tmp, "%02X", *p);
                    vs.append(tmp);
                } else {
                    vs.append(*p);
                }
                p ++;
            }
            return vs.toString();
        }

        static std::string encode(std::map<std::string, std::string> & body) {
            COMMON::VarString vs;
            for(std::map<std::string, std::string>::iterator iter = body.begin(); iter != body.end(); iter ++) {
                vs.append(iter->first).append('=').append(encode(iter->second)).append('&');
            }
            if (body.size() > 0) {
                return vs.drop().toString();
            } else {
                return "";
            }
        }
};


}

#endif
