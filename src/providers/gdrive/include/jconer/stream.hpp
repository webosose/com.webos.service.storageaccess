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

#ifndef __JCONER_STREAM_HPP__
#define __JCONER_STREAM_HPP__

#include "jconer/token.hpp"
#include "common/logging.hpp"
#include "jconer/error.hpp"

#include <istream>
#include <fstream>


namespace JCONER {

class IStream {
    public:
        IStream(std::istream& _fin);
        virtual Token getNextToken();
        inline PError error() const { return _err; }
        virtual ~IStream();
    private:
        int _getLength();
        int _getRemainingLength();

        int _getNextChar();
        void _ungetChar();
        bool _stripWhitespace();
        int _readBuffer();
        int _getNChar(char* str, int n);

        std::istream& _fin;
        int _lineno;
        int _col;
        char* _buff;
        char* _end;
        char* _cur;
        int _cur_pos;
        int _content_length;
        PError _err;
};

}
#endif
