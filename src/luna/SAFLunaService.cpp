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

#include<string>
#include<SAFLunaService.h>

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
    return true;
}

bool SAFLunaService::authenticateCloud(LSMessage &message)
{
    LS::Message request(&message);
    return true;
}

bool SAFLunaService::listFolderContents(LSMessage &message)
{
    LS::Message request(&message);

    LOG_TRACE("Entering function %s", __FUNCTION__);

    pbnjson::JValue requestObj;
    std::string payload;
    int parseError = 0;

    LOG_DEBUG_SAF("listFolderContents : Before Schema check");
    const std::string schema = STRICT_SCHEMA(PROPS_1(PROP(folderPath, string))REQUIRED_1(folderPath));

    if (!LSUtils::parsePayload(request.getPayload(), requestObj, schema, &parseError))
    {
        LOG_DEBUG_SAF("Parmi : listFolderContents : Invalid Json Format Error");
        const std::string errorStr = SAFErrors::getSAFErrorString(SAFErrors::INVALID_JSON_FORMAT);
        LSUtils::respondWithError(request, errorStr, SAFErrors::INVALID_JSON_FORMAT);
        return true;
    }

    std::string folderPathString = requestObj["folderPath"].asString();
    LOG_DEBUG_SAF("listFolderContents : Folder Path : %s", folderPathString.c_str());

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

