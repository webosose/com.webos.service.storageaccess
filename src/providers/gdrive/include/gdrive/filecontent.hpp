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

#ifndef __GDRIVE_FILECONTENT_HPP__
#define __GDRIVE_FILECONTENT_HPP__

#include "gdrive/util.hpp"
#include "gdrive/config.hpp"
#include "common/all.hpp"

#include <fstream>

namespace GDRIVE {

class FileContent {
    CLASS_MAKE_LOGGER
    public:
        FileContent(std::ifstream& fin, std::string mimetype)
            :_fin(fin), _mimetype(mimetype)
        {
            _length = -1;
            _resumable_cur_pos = _resumable_start_pos = _resumable_length = 0;
#ifdef GDRIVE_DEBUG
            CLASS_INIT_LOGGER("FileContent", COMMON::L_DEBUG)
#endif
        }

        FileContent(const FileContent& other)
            :_fin(other._fin), _mimetype(other._mimetype)
        {
            _length = other._length;
            _resumable_length = other._resumable_length;
            _resumable_start_pos = other._resumable_start_pos;
            _resumable_cur_pos = other._resumable_cur_pos;
#ifdef GDRIVE_DEBUG
            CLASS_INIT_LOGGER("FileContent", COMMON::L_DEBUG)
#endif
        }
 
        int get_length();
        inline std::string mimetype() const { return _mimetype; }

        std::string get_content();
        static size_t read(void* ptr, size_t size, size_t nmemb, void* userp);
        static size_t resumable_read(void* ptr, size_t size, size_t nmemb, void* userp);

        void set_resumable_start_pos(int pos);
        void set_resumable_length(int length);
    protected:
        int _getRemainingLength();
        std::ifstream& _fin;
        std::string _mimetype;
        int _length;
 
        int _resumable_start_pos;
        int _resumable_length;
        int _resumable_cur_pos;
};

}

#endif
