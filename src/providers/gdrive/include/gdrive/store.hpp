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

#ifndef __GDRIVE_STORE_HPP__
#define __GDRIVE_STORE_HPP__

#include "gdrive/util.hpp"
#include "common/all.hpp"

#include <map>
#include <fstream>

namespace GDRIVE {

enum StoreStatus {
    SS_EMPTY = 0,
    SS_FULL
};

class Store {
    public:
        virtual std::string get(std::string key) = 0;
        virtual void put(std::string key, std::string value) = 0;
        virtual bool dump() = 0;
        inline StoreStatus status() const { return _status; }
    protected:
        StoreStatus _status;
};

class FileStore : public Store {
    CLASS_MAKE_LOGGER
    public:
        FileStore(std::string filename);
        std::string get(std::string);
        void put(std::string key, std::string value);
        bool dump();
    private:
        std::map<std::string, std::string>  _content;
        std::string _filename;
};


}

#endif
