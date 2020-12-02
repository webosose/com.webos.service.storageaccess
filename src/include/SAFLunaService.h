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

#ifndef SRC_LUNA_SAFLUNASERVICE_H_
#define SRC_LUNA_SAFLUNASERVICE_H_

#include <luna-service2/lunaservice.hpp>
#include <StatusHandler.h>
#include <SAFLunaUtils.h>
#include <SAFErrors.h>
#include <SAFLog.h>

class SAFLunaService: public LS::Handle, public StatusObserver
{
public:
    SAFLunaService();
    virtual ~SAFLunaService();
    bool listFolderContents(LSMessage &message);
    bool getProperties(LSMessage &message);
    bool listOfStorages(LSMessage &message);
    bool copy(LSMessage &message);
    bool move(LSMessage &message);
    bool eject(LSMessage &message);
    bool remove(LSMessage &message);
    bool format(LSMessage &message);

private :
    static LSHandle* lsHandle;

    void registerService();
//  static void responseCallback(Parameters* paramList, LS::Message& message);
//  bool addSubscription(LSHandle *sh, LSMessage *message, std::string key);
//  void setLSHandle(LSHandle* handle);

};
#endif /* SRC_LUNA_SAFLUNASERVICE_H_ */

