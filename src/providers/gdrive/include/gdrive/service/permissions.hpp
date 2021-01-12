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

#ifndef __GDRIVE_PERMISSIONSERVICE_HPP__
#define __GDRIVE_PERMISSIONSERVICE_HPP__

#include "gdrive/config.hpp"
#include "gdrive/credential.hpp"
#include "gdrive/util.hpp"
#include "gdrive/gitem.hpp"
#include "gdrive/servicerequest.hpp"
#include "common/all.hpp"

#include <vector>

namespace GDRIVE {

class PermissionService {
    CLASS_MAKE_LOGGER
    public:
        static PermissionService& get_instance(Credential *cred) {
            _single_instance.set_cred(cred);
            return _single_instance;
        }
        
        PermissionListRequest List(std::string file_id);
        PermissionGetRequest Get(std::string file_id, std::string permission_id);
        PermissionInsertRequest Insert(std::string file_id, GPermission* permission);
        PermissionDeleteRequest Delete(std::string file_id, std::string permission_id);
        PermissionPatchRequest Patch(std::string file_id, std::string permission_id, GPermission* permission);
        PermissionUpdateRequest Update(std::string file_id, std::string permission_id, GPermission* permission);
        PermissionGetIdForEmailRequest GetIdForEmail(std::string email);
    private:
        PermissionService();
        PermissionService(const PermissionService& other);
        PermissionService& operator=(const PermissionService& other);

        static PermissionService _single_instance;

        Credential* _cred;
        inline void set_cred(Credential* cred) {
            _cred = cred;
        }

};
}
#endif
