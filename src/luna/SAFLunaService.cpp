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
        LS_CATEGORY_METHOD(listStorageProviders)
    LS_CREATE_CATEGORY_END
    try
    {
        this->registerCategory("/", LS_CATEGORY_TABLE_NAME(rootAPI), nullptr, nullptr);
        this->setCategoryData("/", this);
    }
    catch (LS::Error &lunaError)
    {
        LOG_DEBUG_SAF("Error in registerCategory (/)");
    }

    LS_CREATE_CATEGORY_BEGIN(SAFLunaService, device)
        LS_CATEGORY_METHOD(handleExtraCommand)
        LS_CATEGORY_METHOD(list)
        LS_CATEGORY_METHOD(getProperties)
        LS_CATEGORY_METHOD(copy)
        LS_CATEGORY_METHOD(move)
        LS_CATEGORY_METHOD(remove)
        LS_CATEGORY_METHOD(eject)
        LS_CATEGORY_METHOD(rename)
    LS_CREATE_CATEGORY_END

    try
    {
        this->registerCategory("/device", LS_CATEGORY_TABLE_NAME(device), nullptr, nullptr);
        this->setCategoryData("/device", this);
    }
    catch (LS::Error &lunaError)
    {
        LOG_DEBUG_SAF("Error in registerCategory (/device)");
    }
    StatusHandler::GetInstance()->Register(this);
}

void SAFLunaService::getSubsDropped(void)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    return;
}

bool SAFLunaService::handleExtraCommand(LSMessage &message)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string), PROP(storageId, string), PROP(command, string), OBJECT(commandArgs, OBJSCHEMA_3(PROP(clientId, string), PROP(clientSecret, string), PROP(secretToken, string))))REQUIRED_4(storageType, storageId, command, commandArgs));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("%s, Invalid Json Format Error", __FUNCTION__);
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    pbnjson::JValue args = requestObj["commandArgs"];
    std::string clientId = "";
    std::string clientSecret = "";
    std::string secretToken = "";
    std::string command = requestObj["command"].asString();
    LS::SubscriptionPoint* subs = new LS::SubscriptionPoint;
    subs->setServiceHandle(this);
    subs->subscribe(request);
    if("attachCloud" == command)
    {
        if(args.hasKey("clientId"))
            clientId = args["clientId"].asString();
        if(args.hasKey("clientSecret"))
            clientSecret = args["clientSecret"].asString();
        if (clientId.empty() || clientSecret.empty())
        {
            const std::string errorStr =
                SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_PARAM);
            LSUtils::respondWithError(request, errorStr, SAFErrors::CloudErrors::INVALID_PARAM);
            return true;
        }
        mAuthParam["client_id"] = clientId;
        mAuthParam["client_secret"] = clientSecret;
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::ATTACH_METHOD, requestObj, SAFLunaService::onHandleExtraCommandReply)
        mDocumentProviderManager->addRequest(reqData);
    }
    else if("authenticateCloud" == command)
    {
        if(args.hasKey("secretToken"))
            secretToken = args["secretToken"].asString();
        if (secretToken.empty())
        {
            const std::string errorStr =
                SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_PARAM);
            LSUtils::respondWithError(request, errorStr, SAFErrors::CloudErrors::INVALID_PARAM);
            return true;
        }
        if (mAuthParam["client_id"].empty() || mAuthParam["client_secret"].empty())
        {
            const std::string errorStr =
                SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::ATTACH_NOT_DONE);
            LSUtils::respondWithError(request, errorStr, SAFErrors::CloudErrors::ATTACH_NOT_DONE);
            return true;
        }
        mAuthParam["secret_token"] = secretToken;
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::AUTHENTICATE_METHOD, requestObj, SAFLunaService::onHandleExtraCommandReply)
        mDocumentProviderManager->addRequest(reqData);
    }
    else
    {
        const std::string errorStr =
            SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::UNKNOWN_ERROR);
        LSUtils::respondWithError(request, errorStr, SAFErrors::CloudErrors::UNKNOWN_ERROR);
    }
    return true;
}

void SAFLunaService::onHandleExtraCommandReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LSUtils::postToClient(subs->getMessage(), rootObj);
}

bool SAFLunaService::list(LSMessage &message)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    std::string payload;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_7(PROP(storageType, string),PROP(storageId, string),PROP(path, string),PROP(limit, integer),PROP(offset, integer),PROP(refreshToken, string),PROP_WITH_VAL_1(refresh, boolean, true))REQUIRED_5(storageType,storageId,path,offset,limit));
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
    REQUEST_BUILDER(reqData, MethodType::LIST_METHOD, requestObj, SAFLunaService::onListReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onListReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL || type == StorageType::USB || type == StorageType::GDRIVE)
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
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
    REQUEST_BUILDER(reqData, MethodType::GET_PROP_METHOD, requestObj, SAFLunaService::onGetPropertiesReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onGetPropertiesReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if ((type == StorageType::INTERNAL) || (type == StorageType::GDRIVE))
        {
            respObj = rootObj;
        }
        else if(type == StorageType::USB)
        {
            USBPbnJsonParser parser;
            respObj = parser.ParseGetProperties(rootObj, reqObj["storageId"].asString());
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(subscribe, boolean)));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    shared_ptr<vector<StorageType>> storageProviders = DocumentProviderFactory::getSupportedDocumentProviders();
    for (auto itr = storageProviders->begin(); itr != storageProviders->end(); ++itr) {
        if(*itr == StorageType::USB)
        {
            requestObj.put("storageType","USB");
        }
    }
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, requestObj, SAFLunaService::onListOfStoragesReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onListOfStoragesReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue respObj = pbnjson::Object();
    StorageType type = getStorageDeviceType(rootObj);
    if(type == StorageType::USB)
    {
        shared_ptr<vector<StorageType>> storageProviders = DocumentProviderFactory::getSupportedDocumentProviders();
        for (auto itr = storageProviders->begin(); itr != storageProviders->end(); ++itr)
        {
            if(*itr == StorageType::USB)
            {
                USBPbnJsonParser parser;
                respObj = parser.ParseListOfStorages(rootObj["response"]);
            }
        }
        pbnjson::JValue providerRespArray = pbnjson::Array();
        providerRespArray.append(respObj);
        rootObj.put("storageType","INTERNAL");
        rootObj.put("listStorageResponse",providerRespArray);
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, rootObj, SAFLunaService::onListOfStoragesReply);
        mDocumentProviderManager->addRequest(reqData);
    }
    else if(type == StorageType::INTERNAL)
    {
        rootObj.put("storageType","CLOUD");
        pbnjson::JValue providerRespArray = rootObj["listStorageResponse"];
        providerRespArray.append(rootObj["response"]);
        rootObj.put("listStorageResponse",providerRespArray);
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, rootObj, SAFLunaService::onListOfStoragesReply);
        mDocumentProviderManager->addRequest(reqData);
    }
    else if (type == StorageType::GDRIVE)
    {
        pbnjson::JValue providerRespArray = rootObj["listStorageResponse"];
        providerRespArray.append(rootObj["response"]);
        pbnjson::JValue responseObj = pbnjson::Object();
        responseObj.put("returnValue", true);
        responseObj.put("response", providerRespArray);
        LSUtils::postToClient(request, responseObj);
    }
    else
    {
        respObj.put("returnValue", false);
        LSUtils::postToClient(subs->getMessage(), respObj);
    }
}

bool SAFLunaService::copy(LSMessage &message)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_10(PROP(srcStorageType, string),
        PROP(srcStorageId, string), PROP(destStorageType, string), PROP(destStorageId, string),
        PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string),
        PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
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
    REQUEST_BUILDER(reqData, MethodType::COPY_METHOD, requestObj, SAFLunaService::onCopyReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onCopyReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL || type == StorageType::USB || type == StorageType::GDRIVE)
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    std::string payload;
    const std::string schema = STRICT_SCHEMA(PROPS_10(PROP(srcStorageType, string),
        PROP(srcStorageId, string), PROP(destStorageType, string), PROP(destStorageId, string),
        PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string),
        PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
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
    REQUEST_BUILDER(reqData, MethodType::MOVE_METHOD, requestObj, SAFLunaService::onMoveReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onMoveReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL || type == StorageType::USB || type == StorageType::GDRIVE)
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
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
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::REMOVE_METHOD, requestObj, SAFLunaService::onRemoveReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onRemoveReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL || type == StorageType::USB || type == StorageType::GDRIVE)
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(storageNumber, integer))REQUIRED_1(storageNumber));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    int storageNumber = 0;
    requestObj["storageNumber"].asNumber<int>(storageNumber);
    if (storageNumber <= 0)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    requestObj.put("storageType","USB"); //for USB storageType
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::EJECT_METHOD, requestObj, SAFLunaService::onEjectReply)
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onEjectReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        reqObj.put("storageType","USB"); //for USB storageType
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::USB)
        {
            USBPbnJsonParser parser;
            respObj = parser.ParseEject(rootObj);
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
    LOG_DEBUG_SAF("%s", __FUNCTION__);
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
    REQUEST_BUILDER(reqData, MethodType::RENAME_METHOD, requestObj, SAFLunaService::onRenameReply);
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onRenameReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    LS::Message request(subs->getMessage());
    pbnjson::JValue reqObj;
    pbnjson::JValue respObj;
    if (LSUtils::parsePayload(request.getPayload(), reqObj, std::string(SCHEMA_ANY), NULL))
    {
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::INTERNAL || type == StorageType::USB || type == StorageType::GDRIVE)
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
    else
        LOG_DEBUG_SAF("getStorageDeviceType : Invalid storageType");
    return storageType;
}

