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

#ifndef __JCONER_ERROR_HPP__
#define __JCONER_ERROR_HPP__

#include "jconer/util.hpp"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define BUFSIZE 1024

namespace JCONER {
enum ErrorType {
    ET_SUC = 0,
    ET_IO_FILE_NOT_FOUND,
    ET_IO_STREAM_NOT_FOUND,
    ET_IO_READ,
    ET_PARSE_EOF,
    ET_PARSE_INVALID_CHAR,
    ET_PARSE_INVALID_UNICODE,
    ET_PARSE_INVALID_TOKEN,
    ET_PARSE_RANGE,
    ET_PARSE_UNEXPECTED_TOKEN,
};

struct PError {
    PError()
        :lineno(-1), col(-1),
         type(ET_SUC)
    {}
    PError(int lineno_, int col_, std::string text_, ErrorType type_)
        :lineno(lineno_),
         col(col_),
         text(text_),
         type(type_)
    {}
    PError(const PError& other)
        :lineno(other.lineno),
         col(other.col),
         text(other.text),
         type(other.type)
    {}

    void setErrorDetail(int lineno_, int col_, ErrorType type_, const char* fmt, ...) {
        lineno = lineno_;
        col = col_;
        type = type_;
        
        va_list va;
        va_start(va, fmt);
        char fullstr[BUFSIZE];
        vsnprintf(fullstr, BUFSIZE, fmt, va);
        va_end(va);
        text = std::string(fullstr);
    }

    void setErrorText(const char* fmt, ...) {
        va_list va;
        va_start(va, fmt);
        char fullstr[BUFSIZE];
        vsnprintf(fullstr, BUFSIZE, fmt, va);
        va_end(va);
        text = std::string(fullstr);
    }

    inline void clear() {
        lineno = -1;
        col = -1;
        text = "";
        type = ET_SUC;
    }

    int lineno;
    int col;
    std::string text;
    ErrorType type;
};

}
#endif
