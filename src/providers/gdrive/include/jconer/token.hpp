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

#ifndef __JCONER_TOKEN_HPP__
#define __JCONER_TOKEN_HPP__

#include <string>
#include "common/all.hpp"
using namespace COMMON;

namespace JCONER {

enum TokenType {
    TT_ARRAY_OPEN_BRACE = 0,
    TT_ARRAY_CLOSE_BRACE,
    TT_OBJECT_OPEN_BRACE,
    TT_OBJECT_CLOSE_BRACE,

    TT_COMMA,
    TT_COLON,

    TT_STRING,
    TT_INTEGER,
    TT_REAL,

    TT_TRUE,
    TT_FALSE,
    TT_NULL,

    TT_END, // end of file

    TT_INVALID
};

class Token {
    public:
        Token(TokenType type)
            :_type(type), _lineno(-1), _col(-1)
        {
        }
        Token(TokenType type, int lineno, int col, std::string text)
            : _type(type), _lineno(lineno), _col(col), _text(text)
        {
            LOG(DEBUG) << "Generate a new token[" << toString() << "] at [" << _lineno << "|" << _col << "]" << std::endl;
        }
        Token(TokenType type, int lineno, int col, char c)
            : _type(type), _lineno(lineno), _col(col), _text(1, c)
        {
            LOG(DEBUG) << "Generate a new token[" << toString() << "] at [" << _lineno << "|" << _col << "]" << std::endl;
        }

        Token(const Token& t)
            : _type(t._type), _lineno(t._lineno), _col(t._col), _text(t._text)
        {
        }

        inline TokenType type() const { return _type; }
        inline int lineno() const { return _lineno; }
        inline int col() const { return _col;}
        inline std::string text() const { return _text; }
        static TokenType getTokenOne(char c);
        std::string toString();
    private:
        TokenType _type;
        int _lineno;
        int _col;
        std::string _text;
};


}
#endif
