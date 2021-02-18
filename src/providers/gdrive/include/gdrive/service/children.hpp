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

#ifndef __GDRIVE_CHILDRENSERVICE_HPP__
#define __GDRIVE_CHILDRENSERVICE_HPP__

#include "gdrive/config.hpp"
#include "gdrive/credential.hpp"
#include "gdrive/util.hpp"
#include "gdrive/gitem.hpp"
#include "gdrive/servicerequest.hpp"
#include "common/all.hpp"

#include <vector>

namespace GDRIVE {

class ChildrenService {
    CLASS_MAKE_LOGGER
    public:
        static ChildrenService& get_instance(Credential *cred) {
            _single_instance.set_cred(cred);
            return _single_instance;
        }

        ChildrenListRequest List(std::string folder_id);
        std::vector<GChildren> Listall(std::string folder_id);

        ChildrenGetRequest Get(std::string folder_id, std::string child_id);
        ChildrenInsertRequest Insert(std::string folder_id, GChildren* child);
        ChildrenDeleteRequest Delete(std::string folder_id, std::string child_id);
    private:
        ChildrenService();
        ChildrenService(const ChildrenService& other);
        ChildrenService& operator=(const ChildrenService& other);

        static ChildrenService _single_instance;

        Credential* _cred;
        inline void set_cred(Credential* cred) {
            _cred = cred;
        }
};

}
#endif
