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
#include <DocumentProviderManager.h>
#include <SAFLunaUtils.h>
#include <SAFErrors.h>
#include <SAFLog.h>
#include <stdbool.h>
#include <pbnjson.h>
#include "ClientWatch.h"

#define REQUEST_BUILDER(OBJ, TYPE, PARAMS, CB) \
	OBJ->storageType = getStorageDeviceType(PARAMS); \
	OBJ->params = PARAMS; \
	OBJ->cb = std::bind(&CB, this, std::placeholders::_1, std::placeholders::_2); \
	OBJ->methodType = TYPE; \
	std::function<void(void)> fun = std::bind(&SAFLunaService::getSubsDropped, this); \
	OBJ->subs = std::shared_ptr<LSUtils::ClientWatch>(new LSUtils::ClientWatch(this->get(), request.get(), fun));

class SAFLunaService: public LS::Handle, public StatusObserver
{
public:
    SAFLunaService();
    virtual ~SAFLunaService();
    bool handleExtraCommand(LSMessage &message);
	void onHandleExtraCommandReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs);
    bool list(LSMessage &message);
    void onListReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool getProperties(LSMessage &message);
    void onGetPropertiesReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool listStorageProviders(LSMessage &message);
    void onListOfStoragesReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool copy(LSMessage &message);
    void onCopyReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool move(LSMessage &message);
    void onMoveReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool remove(LSMessage &message);
    void onRemoveReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool eject(LSMessage &message);
    void onEjectReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    bool rename(LSMessage &message);
    void onRenameReply(pbnjson::JValue, std::shared_ptr<LSUtils::ClientWatch>);
    void getSubsDropped(void);
    static LSHandle* lsHandle;
private :
    std::shared_ptr<DocumentProviderManager> mDocumentProviderManager;
    AuthParam mAuthParam;
    void registerService();
	StorageType getStorageDeviceType(pbnjson::JValue jsonObj);
};
#endif /* SRC_LUNA_SAFLUNASERVICE_H_ */

