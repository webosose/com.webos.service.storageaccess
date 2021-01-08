// Copyright (c) 2020-2021 LG Electronics, Inc.
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

#include <string>
#include <SAFLunaService.h>

const std::string service_name = "com.webos.service.storageaccess";

LSHandle* SAFLunaService::lsHandle = nullptr;

SAFLunaService::SAFLunaService()
        : LS::Handle(LS::registerService(service_name.c_str()))
{
    SAFLunaService::lsHandle = this->get();
    registerService();
}

SAFLunaService::~SAFLunaService()
{
    StatusHandler::GetInstance()->Unregister(this);
}

void SAFLunaService::registerService()
{
    LOG_DEBUG_SAF("Register Service\n");
    LS_CREATE_CATEGORY_BEGIN(SAFLunaService, rootAPI)
    LS_CATEGORY_METHOD(listFolderContents)
    LS_CATEGORY_METHOD(attachCloud)
    LS_CATEGORY_METHOD(authenticateCloud)
    LS_CATEGORY_METHOD(getProperties)
    LS_CATEGORY_METHOD(listOfStorages)
    LS_CATEGORY_METHOD(copy)
    LS_CATEGORY_METHOD(move)
    LS_CATEGORY_METHOD(remove)
    LS_CATEGORY_METHOD(eject)
    LS_CATEGORY_METHOD(format)
    LS_CREATE_CATEGORY_END

    try {
        this->registerCategory("/", LS_CATEGORY_TABLE_NAME(rootAPI), nullptr, nullptr);
        this->setCategoryData("/", this);
    } catch (LS::Error &lunaError) {
    }
    StatusHandler::GetInstance()->Register(this);
}

bool SAFLunaService::attachCloud(LSMessage &message)
{
    LS::Message request(&message);

    LOG_TRACE("Entering function %s", __FUNCTION__);
    pbnjson::JValue requestObj;
    std::string payload;
    std::string clientID;
    std::string clientSecret;
    int parseError = 0;
    int storageType = 0;

    ReturnValue retValue;

    LOG_DEBUG_SAF("attachCloud : Before Schema check");
    const std::string schema = STRICT_SCHEMA(PROPS_3(PROP(storageType, integer), PROP(clientID, string), PROP(clientSecret, string))REQUIRED_3(storageType, clientID, clientSecret));

    LOG_DEBUG_SAF("attachCloud : Schema = %s", schema.c_str());

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("attachCloud : Invalid Json Format Error");
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    StorageType storage_type = getStorageDeviceType(requestObj);
    clientID = requestObj["clientID"].asString();
    clientSecret = requestObj["clientSecret"].asString();
    if (!clientID.empty() && !clientSecret.empty())
    {
        mAuthParam["client_id"] = clientID;
        mAuthParam["client_secret"] = clientSecret;
        retValue = mDocumentProviderManager->attachCloud(storage_type, mAuthParam);
    }

    pbnjson::JValue responseObj = pbnjson::Object();
    shared_ptr<ValuePairMap> valuPairmapPtr = retValue->first;
    shared_ptr<ContentList> contentListPtr = retValue->second;

    for (auto itr = valuPairmapPtr->begin(); itr != valuPairmapPtr->end(); ++itr)
    {
        std::string firstObj = itr->first;
        ValuePair secondObj = itr->second;
        string secondObj_str = secondObj.first;
        auto secondObj_DataType = secondObj.second;
        LOG_DEBUG_SAF("attachCloud : Map -> firstObj = [ %s ]", firstObj.c_str());
        LOG_DEBUG_SAF("attachCloud : Map -> secondObj__str = %s", secondObj_str.c_str());
        LOG_DEBUG_SAF("attachCloud : Map -> secondObj_DataType = %d", secondObj_DataType);
        responseObj.put("AuthURL", secondObj_str);
    }

    for (auto itr = contentListPtr->begin(); itr != contentListPtr->end(); ++itr)
    {
        shared_ptr<Storage> storage = dynamic_pointer_cast<Storage>(*itr);
        if(storage != nullptr)
        {
            LOG_DEBUG_SAF("attachCloud : Storage->Id = %s", storage->id.c_str());
            LOG_DEBUG_SAF("attachCloud : Storage->name = %s", storage->name.c_str());
        }
    }

    responseObj.put("returnValue", true);
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());

    LOG_DEBUG_SAF("attachCloud : clientID = %s && clientSecret = %s", clientID.c_str(), clientSecret.c_str());
    return true;
}

bool SAFLunaService::authenticateCloud(LSMessage &message)
{
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(storageType, integer),PROP(secretToken, string))REQUIRED_2(storageType,secretToken));
    LOG_DEBUG_SAF("authenticateCloud : Schema = %s", schema.c_str());

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError)) {
        LOG_DEBUG_SAF("authenticateCloud : Invalid Json Format Error");
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    StorageType storage_type = getStorageDeviceType(requestObj);
    mAuthParam["secret_token"] = requestObj["secretToken"].asString();

    if (!mAuthParam["client_id"].empty() && !mAuthParam["client_secret"].empty()) {
        ReturnValue retValue = mDocumentProviderManager->authenticateCloud(storage_type, mAuthParam);
    }

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);

    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::listFolderContents(LSMessage &message)
{
    LS::Message request(&message);

    LOG_TRACE("Entering function %s", __FUNCTION__)

    pbnjson::JValue requestObj;
    std::string payload;
    int parseError = 0;

    LOG_DEBUG_SAF("listFolderContents : Before Schema check");
    const std::string schema = STRICT_SCHEMA(PROPS_5(PROP(storageType, integer),PROP(folderPath, string),PROP(limit, integer),PROP(offset, integer),PROP(refreshToken, string))REQUIRED_2(storageType,folderPath));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    int offset = -1;
    int limit = -1;
    StorageType storageType = getStorageDeviceType(requestObj);
    string folderPathString = requestObj["folderPath"].asString();
    string storageIdStr = requestObj["storageId"].asString();

    LOG_DEBUG_SAF("listFolderContents : Folder Path : %s", folderPathString.c_str());

    requestObj["offset"].asNumber<int>(offset);
    requestObj["limit"].asNumber<int>(limit);
    mAuthParam["refresh_token"] = requestObj["refreshToken"].asString();

    ReturnValue retValue = mDocumentProviderManager->listFolderContents(mAuthParam, storageType, storageIdStr, folderPathString, offset, limit);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::getProperties(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(storageType, string))REQUIRED_1(storageType));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    StorageType storageType = getStorageDeviceType(requestObj);

    mDocumentProviderManager->getProperties(mAuthParam,storageType);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);

    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::listOfStorages(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(folderPath, string))REQUIRED_1(folderPath));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string folderPathString = requestObj["folderPath"].asString();
    LOG_DEBUG_SAF("getProperties : Folder Path : %s", folderPathString.c_str());

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);

    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::copy(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(folderSrcPath, string), PROP(folderDstPath, string))REQUIRED_2(folderSrcPath, folderDstPath));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string folderSrcPathString = requestObj["folderSrcPath"].asString();
    std::string folderDstPathString = requestObj["folderDstcPath"].asString();

    LOG_DEBUG_SAF("copy : Src Folder Path:%s And Dst Folder Path=%s", folderSrcPathString.c_str(), folderDstPathString);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);

    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::move(LSMessage &message)
{
    LS::Message request(&message);
     std::string payload;

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);

    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());

    return true;
}

bool SAFLunaService::remove(LSMessage &message)
{
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(storageType, string),PROP(folderPath, string))REQUIRED_2(storageType,folderPath));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageTypeString = requestObj["storageType"].asString();
    std::string folderpathString = requestObj["folderPath"].asString();
    std::string storageIdString = "GDrive"; //requestObj["storageId"].asString();
    LOG_DEBUG_SAF("========> storageType:%s",storageTypeString.c_str());
    LOG_DEBUG_SAF("========> folderPath:%s",folderpathString.c_str());

    StorageType storageType = getStorageDeviceType(requestObj);
    mDocumentProviderManager->remove(mAuthParam,storageType,storageIdString,folderpathString);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::eject(LSMessage &message)
{
    LS::Message request(&message);

    return true;
}

bool SAFLunaService::format(LSMessage &message)
{
    LS::Message request(&message);

    return true;
}

StorageType SAFLunaService::getStorageDeviceType(pbnjson::JValue jsonObj)
{
    int storageTypeInt = -1;
    StorageType storageType = StorageType::INVALID;
    if (!jsonObj.isNull() && jsonObj["storageType"].isNumber() && jsonObj["storageType"].asNumber<int>(storageTypeInt) == CONV_OK) {
        switch (storageTypeInt) {
        case 0:
            storageType = StorageType::INTERNAL;
            break;
        case 1:
            storageType = StorageType::USB;
            break;
        case 2:
            storageType = StorageType::GDRIVE;
            break;
        default:
            storageType = StorageType::INVALID;
            LOG_DEBUG_SAF("getStorageDeviceType : Invalid storageType");
        }
    }
    return storageType;
}

