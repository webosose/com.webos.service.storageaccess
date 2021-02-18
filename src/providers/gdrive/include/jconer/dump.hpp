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

#ifndef __JCONER_DUMP_HPP__
#define __JCONER_DUMP_HPP__

#include "jconer/token.hpp"
#include "jconer/value.hpp"
#include "jconer/stream.hpp"
#include "jconer/parser.hpp"

#include <iostream>

#define DUMP_SORT_KEY 0x01
#define DUMP_PRETTY_PRINT 0x02
#define DUMP_ENSURE_ASCII 0x04
#define DUMP_COMPACT_PRINT 0x08

#define DUMP_INDENT 4

namespace JCONER {

void dump(JValue* value, std::ostream& out, int flag = DUMP_ENSURE_ASCII);
void dumpFile(JValue* value, std::string filename, int flag = DUMP_ENSURE_ASCII);
std::string dumps(JValue* value, int flag = DUMP_ENSURE_ASCII);
void dumps(JValue* value, char** pbuffer, int flag = DUMP_ENSURE_ASCII);

}

#endif
