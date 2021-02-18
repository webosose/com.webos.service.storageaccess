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
    const std::string schema = STRICT_SCHEMA(PROPS_3(PROP(storageType, string), PROP(driveId, string),
        OBJECT(operation, OBJSCHEMA_2(PROP(type, string), OBJECT(payload,
        OBJSCHEMA_4(PROP(clientId, string), PROP(clientSecret, string), PROP(secretToken, string), PROP(refreshToken, string))))))
        REQUIRED_2(storageType,  operation));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("%s, Invalid Json Format Error", __FUNCTION__);
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    requestObj.put("storageType", requestObj["storageType"].asString());
    std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
    REQUEST_BUILDER(reqData, MethodType::EXTRA_METHOD, requestObj, SAFLunaService::onHandleExtraCommandReply)
    mDocumentProviderManager->addRequest(reqData);
    return true;
}

void SAFLunaService::onHandleExtraCommandReply(pbnjson::JValue rootObj, std::shared_ptr<LSUtils::ClientWatch> subs)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    // Fill Reply Object from Root Object and send
    pbnjson::JValue responseObj = pbnjson::Object();
    responseObj.put("returnValue", rootObj["returnValue"]);
    pbnjson::JValue responsePayload = pbnjson::Array();
    if(rootObj.hasKey("response"))
        responsePayload.append(rootObj["response"]);
    responseObj.put("responsePayload", responsePayload);
    LSUtils::postToClient(subs->getMessage(), rootObj);
}

bool SAFLunaService::list(LSMessage &message)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    LS::Message request(&message);
    pbnjson::JValue requestObj;
    std::string payload;
    int parseError = 0;
    const std::string schema = STRICT_SCHEMA(PROPS_7(PROP(storageType, string),PROP(driveId, string),PROP(path, string),PROP(limit, integer),PROP(offset, integer),PROP(refreshToken, string),PROP_WITH_VAL_1(refresh, boolean, true))REQUIRED_5(storageType,driveId,path,offset,limit));
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
    string storageIdStr = requestObj["driveId"].asString();
    LOG_DEBUG_SAF("listFolderContents : Folder Path : %s", folderPathString.c_str());
    requestObj["offset"].asNumber<int>(offset);
    requestObj["limit"].asNumber<int>(limit);
    if ((storageType.empty()) || (folderPathString.empty()) || (storageIdStr.empty())
        || (offset < 1) || (offset > 100) || (limit == -1))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(storageType) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string),
        PROP(driveId, string), PROP(path, string), PROP(refreshToken, string))REQUIRED_2(storageType,driveId));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageType = requestObj["storageType"].asString();
    std::string driveId = requestObj["driveId"].asString();
    if ((storageType.empty()) || (driveId.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(storageType) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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
            if(reqObj.hasKey("path"))
            {
                respObj = rootObj;
            }
            else
            {
                USBPbnJsonParser parser;
                respObj = parser.ParseGetProperties(rootObj, reqObj["driveId"].asString());
            }
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
    for (auto itr = storageProviders->begin(); itr != storageProviders->end(); ++itr)
    {
        if(*itr == StorageType::USB)
        {
            requestObj.put("storageType","usb");
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
        if(rootObj.hasKey("response"))
            respObj = rootObj["response"];
        pbnjson::JValue providerRespArray = pbnjson::Array();
        providerRespArray.append(respObj);
        rootObj.put("storageType","internal");
        rootObj.put("listStorageResponse",providerRespArray);
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, rootObj, SAFLunaService::onListOfStoragesReply);
        mDocumentProviderManager->addRequest(reqData);
    }
    else if(type == StorageType::INTERNAL)
    {
        rootObj.put("storageType","cloud");
        pbnjson::JValue providerRespArray;
        if(rootObj.hasKey("listStorageResponse"))
            providerRespArray = rootObj["listStorageResponse"];
        if(rootObj.hasKey("response"))
            providerRespArray.append(rootObj["response"]);
        rootObj.put("listStorageResponse",providerRespArray);
        std::shared_ptr<RequestData> reqData = std::make_shared<RequestData>();
        REQUEST_BUILDER(reqData, MethodType::LIST_STORAGES_METHOD, rootObj, SAFLunaService::onListOfStoragesReply);
        mDocumentProviderManager->addRequest(reqData);
    }
    else if (type == StorageType::GDRIVE)
    {
        pbnjson::JValue providerRespArray;
        if(rootObj.hasKey("listStorageResponse"))
            providerRespArray = rootObj["listStorageResponse"];
        if(rootObj.hasKey("response"))
            providerRespArray.append(rootObj["response"]);
        pbnjson::JValue responseObj = pbnjson::Object();
        responseObj.put("returnValue", true);
        responseObj.put("storageProviders", providerRespArray);
        LSUtils::postToClient(subs->getMessage(), responseObj);
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
        PROP(srcDriveId, string), PROP(destStorageType, string), PROP(destDriveId, string),
        PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string),
        PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
        REQUIRED_6(srcStorageType, srcDriveId, destStorageType, destDriveId, srcPath, destPath));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string srcType = requestObj["srcStorageType"].asString();
    std::string srcId = requestObj["srcDriveId"].asString();
    std::string desType = requestObj["destStorageType"].asString();
    std::string desId = requestObj["destDriveId"].asString();
    std::string srcPath = requestObj["srcPath"].asString();
    std::string destPath = requestObj["destPath"].asString();
    if ((srcType.empty()) || (srcId.empty()) || (desType.empty()) || (desId.empty()) || (srcPath.empty()) || (destPath.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(srcType) == StorageType::INVALID || getStorageDeviceType(desType) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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
        PROP(srcDriveId, string), PROP(destStorageType, string), PROP(destDriveId, string),
        PROP(srcPath, string), PROP(destPath, string), PROP(srcRefreshToken, string),
        PROP(destRefreshToken, string), PROP(overwrite, boolean), PROP(subscribe, boolean))
        REQUIRED_6(srcStorageType, srcDriveId, destStorageType, destDriveId, srcPath, destPath));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string srcType = requestObj["srcStorageType"].asString();
    std::string srcId = requestObj["srcDriveId"].asString();
    std::string desType = requestObj["destStorageType"].asString();
    std::string desId = requestObj["destDriveId"].asString();
    std::string srcPath = requestObj["srcPath"].asString();
    std::string destPath = requestObj["destPath"].asString();
    if ((srcType.empty()) || (srcId.empty()) || (desType.empty()) || (desId.empty()) || (srcPath.empty()) || (destPath.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(srcType) == StorageType::INVALID || getStorageDeviceType(desType) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string), PROP(driveId, string), PROP(path, string), PROP(refreshToken, string))REQUIRED_3(storageType,driveId,path));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageTypeString = requestObj["storageType"].asString();
    std::string folderpathString = requestObj["path"].asString();
    std::string storageIdString = requestObj["driveId"].asString();

    if ((storageTypeString.empty()) || (storageIdString.empty()) || (folderpathString.empty()))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(storageTypeString) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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
    const std::string schema = STRICT_SCHEMA(PROPS_2(PROP(storageType, string),
        PROP(driveId, string))REQUIRED_2(storageType, driveId));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    if (requestObj["storageType"].asString().empty() || requestObj["driveId"].asString().empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }

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
        reqObj.put("storageType","usb"); //for USB storageType
        StorageType type = getStorageDeviceType(reqObj);
        if (type == StorageType::USB)
        {
            USBPbnJsonParser parser;
            respObj = parser.ParseEject(rootObj);
        }
        else if (type == StorageType::GDRIVE)
        {
            respObj.put("returnValue", true);
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
    const std::string schema = STRICT_SCHEMA(PROPS_4(PROP(storageType, string), PROP(driveId, string), PROP(path, string), PROP(newName, string))REQUIRED_4(storageType,driveId,path,newName));
    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }
    std::string storageType = requestObj["storageType"].asString();
    std::string driveId = requestObj["driveId"].asString();
    std::string path = requestObj["path"].asString();
    std::string newName = requestObj["newName"].asString();
    if (storageType.empty() || driveId.empty() || path.empty() || newName.empty())
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_PARAM);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_PARAM);
        return true;
    }
    else if(getStorageDeviceType(storageType) == StorageType::INVALID)
    {
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        LSUtils::respondWithError(request, errorStr, SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
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

    if(type == "internal")
        storageType = StorageType::INTERNAL;
    else if(type == "usb")
        storageType = StorageType::USB;
    else if(type == "cloud")
        storageType = StorageType::GDRIVE;
    else
        LOG_DEBUG_SAF("getStorageDeviceType : Invalid storageType");
    return storageType;
}

StorageType SAFLunaService::getStorageDeviceType(std::string type)
{
    StorageType storageType = StorageType::INVALID;
    if(type == "internal")
        storageType = StorageType::INTERNAL;
    else if(type == "usb")
        storageType = StorageType::USB;
    else if(type == "cloud")
        storageType = StorageType::GDRIVE;
    else
        LOG_DEBUG_SAF("getStorageDeviceType : Invalid storageType");
    return storageType;
}
