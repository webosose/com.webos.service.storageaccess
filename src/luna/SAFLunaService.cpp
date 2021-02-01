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
#include "USBJsonParser.h"

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
    LS_CREATE_CATEGORY_BEGIN(SAFLunaService, rootAPI)
        LS_CATEGORY_METHOD(testMethod)
        LS_CATEGORY_METHOD(listStorageProviders)
        LS_CREATE_CATEGORY_END

        try {
            this->registerCategory("/", LS_CATEGORY_TABLE_NAME(rootAPI), nullptr, nullptr);
            this->setCategoryData("/", this);
        } catch (LS::Error &lunaError) {
        }

    LS_CREATE_CATEGORY_BEGIN(SAFLunaService, device)
        LS_CATEGORY_METHOD(list)
        LS_CATEGORY_METHOD(getProperties)
        LS_CATEGORY_METHOD(copy)
        LS_CATEGORY_METHOD(move)
        LS_CATEGORY_METHOD(remove)
        LS_CATEGORY_METHOD(eject)
        LS_CATEGORY_METHOD(format)
        LS_CATEGORY_METHOD(rename)
        LS_CREATE_CATEGORY_END

        try {
            this->registerCategory("/device", LS_CATEGORY_TABLE_NAME(device), nullptr, nullptr);
            this->setCategoryData("/device", this);
        } catch (LS::Error &lunaError) {
        }

    LS_CREATE_CATEGORY_BEGIN(SAFLunaService, cloud)
        LS_CATEGORY_METHOD(attachCloud)
        LS_CATEGORY_METHOD(authenticateCloud)
        LS_CREATE_CATEGORY_END

        try {
            this->registerCategory("/cloud", LS_CATEGORY_TABLE_NAME(cloud), nullptr, nullptr);
            this->setCategoryData("/cloud", this);
        } catch (LS::Error &lunaError) {
        }

    StatusHandler::GetInstance()->Register(this);
}

void SAFLunaService::getSubsDropped(void)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    return;
}

bool SAFLunaService::testMethod(LSMessage &message)
{
    LS::Message request(&message);
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    pbnjson::JValue requestObj;
    std::string value;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(storageType, string), PROP(value, string))REQUIRED_2(storageType, value));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("%s: Invalid Json Format Error", __FUNCTION__);
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::TEST_METHOD, requestObj, SAFLunaService::onTestMethodReply)
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onTestMethodReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue requestObj;
    if (LSUtils::parsePayload(request.getPayload(), requestObj, std::string(SCHEMA_ANY), NULL))
    {
        LOG_DEBUG_SAF("%s: %d", __FUNCTION__, getStorageDeviceType(requestObj["storageType"].asString()));
    }
    LSUtils::postToClient(subs->getMessage(), rootObj);
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

    ReturnValue retValue;

    LOG_DEBUG_SAF("attachCloud : Before Schema check");
    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(clientID, string), PROP(clientSecret, string))REQUIRED_2(clientID, clientSecret));

    LOG_DEBUG_SAF("attachCloud : Schema = %s", schema.c_str());

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("attachCloud : Invalid Json Format Error");
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    //StorageType storage_type = getStorageDeviceType(requestObj);
    clientID = requestObj["clientID"].asString();
    clientSecret = requestObj["clientSecret"].asString();
    if (!clientID.empty() && !clientSecret.empty())
    {
        mAuthParam["client_id"] = clientID;
        mAuthParam["client_secret"] = clientSecret;
        retValue = mDocumentProviderManager->attachCloud(StorageType::GDRIVE, mAuthParam);
    }
    else
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

    pbnjson::JValue responseObj = pbnjson::Object();
    shared_ptr<ValuePairMap> valuPairmapPtr = retValue->first;
    shared_ptr<ContentList> contentListPtr = retValue->second;

    for (auto itr = valuPairmapPtr->begin(); itr != valuPairmapPtr->end(); ++itr)
    {
        std::string firstObj = itr->first;
        if("authURL" == firstObj )
        {
            ValuePair secondObj = itr->second;
            string secondObj_str = secondObj.first;
            auto secondObj_DataType = secondObj.second;
            LOG_DEBUG_SAF("attachCloud : Map -> firstObj = [ %s ]", firstObj.c_str());
            LOG_DEBUG_SAF("attachCloud : Map -> secondObj__str = %s", secondObj_str.c_str());
            LOG_DEBUG_SAF("attachCloud : Map -> secondObj_DataType = %d", secondObj_DataType);
            responseObj.put("authURL", secondObj_str);
            responseObj.put("returnValue", true);
        }
        else if("errorCode" == firstObj)
        {
            responseObj.put("returnValue", false);
            responseObj.put("errorText", "Invalid URL");
            responseObj.put("errorCode", -1);
            LSUtils::postToClient(request, responseObj);
            return true;
        }
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
    //const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(storageType, integer),PROP(accessToken, string))REQUIRED_2(storageType,secretToken));

    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(secretToken, string))REQUIRED_1(secretToken));
    LOG_DEBUG_SAF("authenticateCloud : Schema = %s", schema.c_str());

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError)) {
        LOG_DEBUG_SAF("authenticateCloud : Invalid Json Format Error");
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    //StorageType storage_type = getStorageDeviceType(requestObj);
    std::string secretToken = requestObj["secretToken"].asString();

    if (secretToken.empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

    mAuthParam["secret_token"] = secretToken;
    ReturnValue retValue;

    if (!mAuthParam["client_id"].empty() && !mAuthParam["client_secret"].empty()) {
        retValue = mDocumentProviderManager->authenticateCloud(StorageType::GDRIVE, mAuthParam);
    }
    else
    {
        LSUtils::respondWithError(request,"attachCloud first", SAFErrors::SERVICE_NOT_READY);
        return true;
    }

    pbnjson::JValue responseObj = pbnjson::Object();
    shared_ptr<ValuePairMap> valuPairmapPtr = retValue->first;

    for (auto itr = valuPairmapPtr->begin(); itr != valuPairmapPtr->end(); ++itr)
    {
        std::string firstObj = itr->first;
        if("refresh_token" == firstObj )
        {
            ValuePair secondObj = itr->second;
            string secondObj_str = secondObj.first;
            responseObj.put("refreshToken", secondObj_str);
            responseObj.put("returnValue", true);
            break;
        }
        else if("errorCode" == firstObj)
        {
            responseObj.put("returnValue", false);
            responseObj.put("errorText", "Invalid URL");
            responseObj.put("errorCode", -1);
            LSUtils::postToClient(request, responseObj);
            return true;
        }
    }
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

bool SAFLunaService::list(LSMessage &message)
{
    LS::Message request(&message);

    LOG_TRACE("Entering function %s", __FUNCTION__)

    pbnjson::JValue requestObj;
    std::string payload;
    int parseError = 0;

    LOG_DEBUG_SAF("listFolderContents : Before Schema check");
    const std::string schema = STRICT_SCHEMA(PROPS_6(PROP(storageType, string),PROP(storageId, string),PROP(path, string),PROP(limit, integer),PROP(offset, integer),PROP(refreshToken, string))REQUIRED_5(storageType,storageId,path,offset,limit));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    int offset = -1;
    int limit = -1;
    string storageType = requestObj["storageType"].asString();
    string folderPathString = requestObj["path"].asString();
    string storageIdStr = requestObj["storageId"].asString();

    LOG_DEBUG_SAF("listFolderContents : Folder Path : %s", folderPathString.c_str());

    requestObj["offset"].asNumber<int>(offset);
    requestObj["limit"].asNumber<int>(limit);

    if ((storageType.empty()) || (folderPathString.empty()) || (storageIdStr.empty()) || (offset == -1) || (limit == -1))
     {
         const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
         LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
         return true;
     }

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::LIST_METHOD, requestObj, SAFLunaService::onListReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("status", "In Progress");
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

void SAFLunaService::onListReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::getProperties(LSMessage &message)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_3(PROP(storageType, string), PROP(storageId, string), PROP(refreshToken, string))REQUIRED_2(storageType,storageId));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageType = requestObj["storageType"].asString();
    std::string storageId = requestObj["storageId"].asString();

    if ((storageType.empty()) || (storageId.empty()))
     {
         const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
         LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
         return true;
     }

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::GET_PROP_METHOD, requestObj, SAFLunaService::onGetPropertiesReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("status", "In Progress");
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

void SAFLunaService::onGetPropertiesReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::listStorageProviders(LSMessage &message)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    std::string value;
    int parseError = 0;

    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(subscribe, boolean)));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, requestObj, SAFLunaService::onListOfStoragesReply)
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onListOfStoragesReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    USBPbnJsonParser parser;
    pbnjson::JValue responseObj = parser.ParseListOfStorages(rootObj);
    LSUtils::postToClient(subs->getMessage(), responseObj);
}

bool SAFLunaService::copy(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_10(PROP(srcStorageType, string), PROP(srcStorageId, string), PROP(destStorageType, string), PROP(destStorageId, string), PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string), PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
                               REQUIRED_6(srcStorageType, srcStorageId, destStorageType, destStorageId, srcPath, destPath));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string srcType = requestObj["srcStorageType"].asString();
    std::string srcId = requestObj["srcStorageId"].asString();
    std::string desType = requestObj["destStorageType"].asString();
    std::string desId = requestObj["destStorageId"].asString();
    std::string srcPath = requestObj["srcPath"].asString();
    std::string destPath = requestObj["destPath"].asString();
    if ((srcType.empty()) || (srcId.empty()) || (desType.empty()) || (desId.empty()) || (srcPath.empty()) || (destPath.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

    requestObj.put("storageType",srcType);
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::COPY_METHOD, requestObj, SAFLunaService::onCopyReply)
    mDocumentProviderManager->addRequest(reqData);

    LOG_DEBUG_SAF("copy : Src Folder Path:%s And Dst Folder Path=%s", srcPath.c_str(), destPath.c_str());
    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("status", "In Progress");
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

void SAFLunaService::onCopyReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::move(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_10(PROP(srcStorageType, string), PROP(srcStorageId, string), PROP(destStorageType, string), PROP(destStorageId, string), PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string), PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
                               REQUIRED_6(srcStorageType, srcStorageId, destStorageType, destStorageId, srcPath, destPath));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string srcType = requestObj["srcStorageType"].asString();
    std::string srcId = requestObj["srcStorageId"].asString();
    std::string desType = requestObj["destStorageType"].asString();
    std::string desId = requestObj["destStorageId"].asString();
    std::string srcPath = requestObj["srcPath"].asString();
    std::string destPath = requestObj["destPath"].asString();

    if ((srcType.empty()) || (srcId.empty()) || (desType.empty()) || (desId.empty()) || (srcPath.empty()) || (destPath.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

    requestObj.put("storageType",srcType);
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::MOVE_METHOD, requestObj, SAFLunaService::onMoveReply)
    mDocumentProviderManager->addRequest(reqData);

    LOG_DEBUG_SAF("Move : Src Folder Path:%s And Dst Folder Path=%s", srcPath.c_str(), destPath.c_str());
    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("status", "In Progress");
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

void SAFLunaService::onMoveReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::remove(LSMessage &message)
{
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string), PROP(storageId, string), PROP(path, string), PROP(refreshToken, string))REQUIRED_3(storageType,storageId,path));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageTypeString = requestObj["storageType"].asString();
    std::string folderpathString = requestObj["path"].asString();
    std::string storageIdString = requestObj["storageId"].asString();

   if ((storageTypeString.empty()) || (storageIdString.empty()) || (folderpathString.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    LOG_DEBUG_SAF("========> storageType:%s",storageTypeString.c_str());
    LOG_DEBUG_SAF("========> folderPath:%s",folderpathString.c_str());

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::REMOVE_METHOD, requestObj, SAFLunaService::onRemoveReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("status", "In Progress");
    LSUtils::generatePayload(responseObj, payload);
    request.respond(payload.c_str());
    return true;
}

void SAFLunaService::onRemoveReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::eject(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;

    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(storageId, string))REQUIRED_1(storageId));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string storageId = requestObj["storageId"].asString();
    if (storageId.empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    requestObj.put("storageType","USB"); //for USB storageType

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::EJECT_METHOD, requestObj, SAFLunaService::onEjectReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    responseObj.put("eject", "In Progress");
    LSUtils::postToClient(request, responseObj);

    return true;
}

void SAFLunaService::onEjectReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::format(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_3(PROP(storageId, string), PROP(fileSystem, string), PROP(volumeLabel, string))REQUIRED_1(storageId));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string storageId = requestObj["storageId"].asString();
    if (storageId.empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    requestObj.put("storageType","USB"); //for USB storageType

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::FORMAT_METHOD, requestObj, SAFLunaService::onFormatReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    responseObj.put("format", "In Progress");
    LSUtils::postToClient(request, responseObj);

    return true;
}

void SAFLunaService::onFormatReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

bool SAFLunaService::rename(LSMessage &message)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string), PROP(storageId, string), PROP(path, string), PROP(newName, string))REQUIRED_4(storageType,storageId,path,newName));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string storageType = requestObj["storageType"].asString();
    std::string storageId = requestObj["storageId"].asString();
    std::string path = requestObj["path"].asString();
    std::string newName = requestObj["newName"].asString();

    if (storageId.empty() || storageId.empty() || storageId.empty() || storageId.empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::RENAME_METHOD, requestObj, SAFLunaService::onRenameReply)
    mDocumentProviderManager->addRequest(reqData);

    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", true);
    LSUtils::postToClient(request, responseObj);

    return true;
}

void SAFLunaService::onRenameReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL)
        {
            respObj = rootObj;
        }
        else
        {
            respObj.put("returnValue", false);
        }
    }
    LSUtils::postToClient(subs->getMessage(), respObj);
}

#if 0
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
#endif

StorageType SAFLunaService::getStorageDeviceType(pbnjson::JValue jsonObj)
{
    StorageType storageType = StorageType::INVALID;
    std::string type;
    if (!jsonObj.isNull() && jsonObj.hasKey("storageType")
        && jsonObj["storageType"].isString())
        type = jsonObj["storageType"].asString();
    else if (!jsonObj.isNull() && jsonObj.hasKey("srcStorageType")
        && jsonObj["srcStorageType"].isString())
        type = jsonObj["srcStorageType"].asString();

    if(type == "INTERNAL")
        storageType = StorageType::INTERNAL;
    else if(type == "USB")
        storageType = StorageType::USB;
    else if(type == "CLOUD")
        storageType = StorageType::GDRIVE;

    LOG_DEBUG_SAF("getStorageDeviceType : Invalid storageType");
    return storageType;
}

