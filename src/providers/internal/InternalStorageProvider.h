// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef _INTERNAL_STORAGE_PROVIDER_H_
#define _INTERNAL_STORAGE_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include <iostream>

class InternalStorageProvider: public DocumentProvider
{
public:
    InternalStorageProvider();
    virtual ~InternalStorageProvider();
    /* bool listFolderContents(string path);
    string getProperties(string path);
    bool copy(string src, string dest);
    bool move(string src, string dest);
    bool remove(string path);
    bool eject(string deviceId) { return false; }
    bool format(string deviceId) { return false; }
    bool Authendication(string deviceId) { return true; }*/
private:
};

#endif /* _INTERNAL_STORAGE_PROVIDER_H_ */
