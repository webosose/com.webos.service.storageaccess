// The MIT License (MIT)
//
// Copyright (c) 2014-2024 Justin (Jianfeng) Lin
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

#ifndef __GDRIVE_ERROR_HPP__
#define __GDRIVE_ERROR_HPP__

#include <vector>
#include <map>
#include <exception>

#include "gdrive/gitem.hpp"

namespace GDRIVE {

class GoogleJsonResponseException : public std::exception {
    public:
        GoogleJsonResponseException(GError error)
            :_details(std::move(error))
        {
        }

        GError& details() {
            return _details;
        }

        virtual ~GoogleJsonResponseException() throw() {}

    private:
        GError _details;
};


class CurlException : public std::exception {
    public:
        CurlException(int code, std::string error)
            :_code(code), _error(std::move(error)) {}
        std::string error() { return _error; }
        int code() { return _code;}
        virtual ~CurlException() throw() {}
    private:
        std::string _error;
        int _code;
};


}

#endif
