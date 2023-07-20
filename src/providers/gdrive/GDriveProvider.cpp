/* @@@LICENSE
 *
 * Copyright (c) 2020-2023 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include <algorithm>
#include <cctype>
#include <SAFLog.h>
#include "GDriveProvider.h"
#include <future>
#include "gdrive/gdrive.hpp"
#include "SAFUtilityOperation.h"
#include "UpnpDiscover.h"

GDriveProvider::GDriveProvider() : mQuit(false)
{
    LOG_DEBUG_SAF(" GDriveProvider::GDriveProvider : Constructor Created");
    mDispatcherThread = std::thread(std::bind(&GDriveProvider::dispatchHandler, this));
    mDispatcherThread.detach();
    insertMimeTypes();
}

GDriveProvider::~GDriveProvider()
{
    mQuit = true;
    mCondVar.notify_one();
}

std::string GDriveProvider::generateDriveId()
{
    static int id = 0;
    return (std::string(GDRIVE_NAME) + "_" + std::to_string(++id));
}

void GDriveProvider::setErrorMessage(shared_ptr<ValuePairMap> valueMap, string errorText)
{
    valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
    valueMap->emplace("errorText", pair<string, DataType>(errorText, DataType::STRING));
    valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
}

bool GDriveProvider::validateExtraCommand(std::vector<std::string> extraParams, std::shared_ptr<RequestData> reqData)
{
    pbnjson::JValue payload = reqData->params["operation"]["payload"];
    for (auto paramName : extraParams)
    {
        if (!payload.hasKey(paramName) || payload[paramName].asString().empty())
            return false;
    }
    return true;
}

void GDriveProvider::extraMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    pbnjson::JValue operation = reqData->params["operation"];
    std::string type = operation["type"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    std::vector<std::string> validCtx;
    if ((type == "login") && validateExtraCommand({"clientId", "clientSecret"}, reqData))
    {
        attachCloud(reqData);
    }
    else if ((type == "authenticate") && validateExtraCommand({"secretToken"}, reqData))
    {
        authenticateCloud(reqData);
    }
    else if ((type == "token") && validateExtraCommand({"clientId", "clientSecret", "refreshToken"}, reqData))
    {
        std::string clientId = reqData->params["operation"]["payload"]["clientId"].asString();
        if (mClientIdDriveId.find(clientId) == mClientIdDriveId.end())
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::INVALID_PARAM);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PARAM));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        if(mDriveIdSessionMap[mClientIdDriveId[clientId]] != reqData->sessionId)
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        GDriveUserData& userDataObj = mDriveIdUserDataMap[mClientIdDriveId[clientId]];
        userDataObj.mAuthParam["client_id"] = reqData->params["operation"]["payload"]["clientId"].asString();
        userDataObj.mAuthParam["client_secret"] = reqData->params["operation"]["payload"]["clientSecret"].asString();
        userDataObj.mAuthParam["refresh_token"] = reqData->params["operation"]["payload"]["refreshToken"].asString();
        while (userDataObj.mCred.use_count() != 0)
            userDataObj.mCred.reset();
        userDataObj.mCred = std::shared_ptr<GDRIVE::Credential>(new GDRIVE::Credential(&userDataObj.mAuthParam));
        respObj.put("returnValue", true);
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PARAM));
        reqData->cb(respObj, reqData->subs);
    }
}

void GDriveProvider::attachCloud(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string authURL;
    std::string clientId = reqData->params["operation"]["payload"]["clientId"].asString();
    std::string clientSecret = reqData->params["operation"]["payload"]["clientSecret"].asString();
    if (mClientIdDriveId.find(clientId) == mClientIdDriveId.end())
    {
        mClientIdDriveId[clientId] = generateDriveId();
        mDriveIdSessionMap[mClientIdDriveId[clientId]] = reqData->sessionId;
        GDriveUserData userDataObj;
        userDataObj.mAuthParam["client_id"] = clientId;
        userDataObj.mAuthParam["client_secret"] = clientSecret;
        GDRIVE::OAuth oauth(clientId, clientSecret);
        authURL = oauth.get_authorize_url();
        mDriveIdUserDataMap[mClientIdDriveId[clientId]] = userDataObj;
        LOG_DEBUG_SAF("attachCloud :client_id = [ %s ]   client_secret = [ %s ]", clientId.c_str(), clientSecret.c_str());
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::ALREADY_AUTHENTICATED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::ALREADY_AUTHENTICATED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if (authURL.empty())
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_URL);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_URL));

    }
    else
    {
        pbnjson::JValue responsePayObjArr = pbnjson::Array();
        pbnjson::JValue responsePayObj = pbnjson::Object();
        pbnjson::JValue payloadObj = pbnjson::Object();
        payloadObj.put("response", authURL);
        responsePayObj.put("type", reqData->params["operation"]["type"].asString());
        responsePayObj.put("payload", payloadObj);
        responsePayObjArr.append(responsePayObj);
        respObj.put("returnValue", true);
        respObj.put("responsePayload", responsePayObjArr);
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::authenticateCloud(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData& userDataObj = mDriveIdUserDataMap[driveId];
    userDataObj.mAuthParam["access_token"] = reqData->params["operation"]["payload"]["secretToken"].asString();
    LOG_DEBUG_SAF("authenticateCloud :client_id = [ %s ]   client_secret = [ %s ]   secret_token = [ %s ]", 
        userDataObj.mAuthParam["client_id"].c_str(), userDataObj.mAuthParam["client_secret"].c_str(), userDataObj.mAuthParam["access_token"].c_str());
    while (userDataObj.mCred.use_count() != 0)
        userDataObj.mCred.reset();
    userDataObj.mCred = std::shared_ptr<GDRIVE::Credential>(new GDRIVE::Credential(&userDataObj.mAuthParam));
    LOG_DEBUG_SAF("authenticateCloudTest 1");
    GDRIVE::OAuth oauth(userDataObj.mAuthParam["client_id"], userDataObj.mAuthParam["client_secret"]);
    LOG_DEBUG_SAF("authenticateCloudTest 2");

    if (!userDataObj.mAuthParam["access_token"].empty()
        && oauth.build_credential(userDataObj.mAuthParam["access_token"], *(userDataObj.mCred.get())))
    {
        pbnjson::JValue responsePayObjArr = pbnjson::Array();
        pbnjson::JValue responsePayObj = pbnjson::Object();
        pbnjson::JValue payloadObj = pbnjson::Object();
        payloadObj.put("response", userDataObj.mAuthParam["refresh_token"]);
        responsePayObj.put("type", reqData->params["operation"]["type"].asString());
        responsePayObj.put("payload", payloadObj);
        responsePayObjArr.append(responsePayObj);
        respObj.put("returnValue", true);
        respObj.put("responsePayload", responsePayObjArr);
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_URL);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_URL));
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::list(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string path = reqData->params["path"].asString();
    int limit = reqData->params["limit"].asNumber<int>();
    int offset = reqData->params["offset"].asNumber<int>();
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];
    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
    std::string folderpathId = userDataObj.mGDriveOperObj.getFileId(path);
    if (!folderpathId.empty())
    {
        GDRIVE::Drive service(userDataObj.mCred.get());
        pbnjson::JValue contentsObj = pbnjson::Array();
        auto fileIdsMap = userDataObj.mGDriveOperObj.getFileMap(path);
        int start = (offset > (int)fileIdsMap.size())?(fileIdsMap.size() + 1):(offset - 1);
        start = (start < 0)?(fileIdsMap.size() + 1):(start);
        int end = ((limit + offset - 1) >  fileIdsMap.size())?(fileIdsMap.size()):(limit + offset - 1);
        end = (end < 0)?(fileIdsMap.size()):(end);
        int index = 0;
        for(auto entry : userDataObj.mGDriveOperObj.getFileMap( path))
        {
            if (index < start)
            {
                index++;
                continue;
            }
            if (index >= end)   break;
            index++;
            GDRIVE::FileGetRequest get = service.files().Get(entry.second);
            get.add_field("id,title,mimeType,fileSize");
            GDRIVE::GFile file = get.execute();
            GDRIVE::GAbout about = service.about().Get().execute();
            long totalspace = about.get_quotaBytesTotal();
            LOG_DEBUG_SAF("Id %s", file.get_id().c_str());
            LOG_DEBUG_SAF("Title %s", file.get_title().c_str());
            LOG_DEBUG_SAF("MimeType %s", file.get_mimeType().c_str());
            LOG_DEBUG_SAF("File Size %ld", file.get_fileSize());
            pbnjson::JValue contentObj = pbnjson::Object();
            contentObj.put("path", entry.first);
            //contentObj.put("id", file.get_id());
            contentObj.put("name", file.get_title());
            contentObj.put("mimeType", file.get_mimeType());
            contentObj.put("type", getFileType(file.get_mimeType()));
            if (-1 == file.get_fileSize())
                contentObj.put("size", 0);
            else
                contentObj.put("size", (int)(totalspace - file.get_fileSize()));
            contentsObj.append(contentObj);
        }
        respObj.put("returnValue", true);
        respObj.put("files", contentsObj);
        respObj.put("fullPath", path);
        respObj.put("totalCount", contentsObj.arraySize());
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::getProperties(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];
    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string path = "root";
    if (reqData->params.hasKey("path"))
    {
        path = reqData->params["path"].asString();
        path = userDataObj.mGDriveOperObj.getFileId(path);
    }
    GDRIVE::Drive service(userDataObj.mCred.get());
    GDRIVE::FileGetRequest get = service.files().Get(path);
    get.add_field("userPermission");
    GDRIVE::GFile file = get.execute();
    GDRIVE::GAbout about = service.about().Get().execute();
    string username = about.get_name();
    long totalspace = about.get_quotaBytesTotal();
    long freespace = about.get_quotaBytesUsed();
    LOG_DEBUG_SAF("==>Usrename:%s", username.c_str());
    LOG_DEBUG_SAF("==>TotalSpace:%ld", totalspace);
    LOG_DEBUG_SAF("==>FreeSpace:%ld", freespace);
    pbnjson::JValue attributesArr = pbnjson::Array();
    respObj.put("returnValue", true);
    respObj.put("storageType", "cloud");
    respObj.put("writable", true);
    respObj.put("deletable", true);
    if (path == "root")
    {
        respObj.put("totalSpace", (int)(totalspace / 1000000));
        respObj.put("freeSpace", (int)(freespace / 1000000));
    }
    respObj.put("attributes", attributesArr);
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::copy(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    if(!SAFUtilityOperation::getInstance().validateInterProviderOperation(reqData))
        return;

    pbnjson::JValue respObj = pbnjson::Object();
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    std::string srcStorageType = reqData->params["srcStorageType"].asString();
    std::string destStorageType = reqData->params["destStorageType"].asString();
    bool overwrite = reqData->params["overwrite"].asBool();
    std::string driveId = "";
    if(srcStorageType == "cloud")
        driveId = reqData->params["srcDriveId"].asString();
    else
        driveId = reqData->params["destDriveId"].asString();
    if ((srcStorageType == "cloud") && (destStorageType == "cloud")
        && (reqData->params["srcDriveId"].asString() != 
        reqData->params["destDriveId"].asString()))
    {
        respObj.put("errorCode", SAFErrors::SAF_ERROR_NOT_SUPPORTED);
        respObj.put("errorText", "Not supported yet");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];
    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string srcFileID = userDataObj.mGDriveOperObj.getFileId(srcPath);
    std::string destFileId = userDataObj.mGDriveOperObj.getFileId(destPath);
    GDRIVE::Drive service(userDataObj.mCred.get());
    if (destStorageType== "cloud")
    {
        if (srcStorageType == "cloud")
        {
            if (!srcFileID.empty())
            {
                GDRIVE::GParent obj;
                vector<GDRIVE::GParent> vec;
                GDRIVE::GFile *movedFile = new GDRIVE::GFile();
                if (!destFileId.empty()) {
                    obj.set_id(destFileId); //setting parent object to folder id
                } else {
                    obj.set_id("root"); //setting parent object to folder id
                }
                vec.push_back(obj);
                movedFile->set_parents(vec);
                service.files().Copy(srcFileID, movedFile).execute();
                respObj.put("returnValue", true);
                respObj.put("progress", 100);
                reqData->cb(respObj, reqData->subs);
                userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
                return;
            }
            else
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_SOURCE_PATH));
            }
        }
        else
        {
            std::string mimeType = getMimeType(srcPath.substr(srcPath.rfind(".")+1));
            if(mimeType.empty())
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_FILE);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_FILE));
                reqData->cb(respObj, reqData->subs);
                return;
            }

            ifstream fin(srcPath.c_str());
            if (!fin.good())
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::UNKNOWN_ERROR));
                reqData->cb(respObj, reqData->subs);
                return;
            }

            GDRIVE::FileContent fc(fin, mimeType);
            GDRIVE::GFile insertFile;
            GDRIVE::GParent obj;
            vector<GDRIVE::GParent> vec;
            if(!destFileId.empty()) {
                obj.set_id(destFileId);
            } else {
                obj.set_id("root");
            }
            vec.push_back(obj);
            insertFile.set_parents(vec);
            insertFile.set_title(srcPath.substr(srcPath.rfind("/")+1));
            insertFile.set_mimeType(mimeType);
            service.files().Insert(&insertFile, &fc).execute();
            fin.close();
            respObj.put("returnValue", true);
            respObj.put("progress", 100);
            reqData->cb(respObj, reqData->subs);
            userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
            return;
        }
    }
    else
    {
        if (srcFileID.empty())
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_SOURCE_PATH));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        else if (!validateInternalPath(destPath))
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::INVALID_DEST_PATH);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_DEST_PATH));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        destPath = destPath + srcPath.substr(srcPath.rfind("/"));
                LOG_DEBUG_SAF(" DEST is NOT Cloud %s", destPath.c_str());
        GDRIVE::GFile downloadFile = service.files().Get(srcFileID).execute();
        string url = downloadFile.get_downloadUrl();
        if (url == "") {
            url = downloadFile.get_exportLinks()["application/pdf"];
        }
        GDRIVE::CredentialHttpRequest request(userDataObj.mCred.get(), url, GDRIVE::RM_GET);
        GDRIVE::HttpResponse resp = request.request();
        ofstream fout(destPath.c_str(), std::ios::binary);
        if(!fout.good()) {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::UNKNOWN_ERROR));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        fout.write(resp.content().c_str(), resp.content().size());
        fout.close();
        respObj.put("returnValue", true);
        respObj.put("progress", 100);
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::move(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    if(!SAFUtilityOperation::getInstance().validateInterProviderOperation(reqData))
        return;

    pbnjson::JValue respObj = pbnjson::Object();
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    std::string srcStorageType = reqData->params["srcStorageType"].asString();
    std::string destStorageType = reqData->params["destStorageType"].asString();
    bool overwrite = reqData->params["overwrite"].asBool();
    if ((srcStorageType == "cloud") && (destStorageType == "cloud")
        && (reqData->params["srcDriveId"].asString() != 
        reqData->params["destDriveId"].asString()))
    {
        respObj.put("errorCode", SAFErrors::SAF_ERROR_NOT_SUPPORTED);
        respObj.put("errorText", "Not supported yet");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string driveId;
    if(srcStorageType == "cloud")
        driveId = reqData->params["srcDriveId"].asString();
    else
        driveId = reqData->params["destDriveId"].asString();

    if(reqData->params.hasKey("subscribe"))
        bool subscribe = reqData->params["subscribe"].asBool();

    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];

    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string srcFileID = userDataObj.mGDriveOperObj.getFileId(srcPath);
    std::string destFileId = userDataObj.mGDriveOperObj.getFileId(destPath);
    GDRIVE::Drive service(userDataObj.mCred.get());
    if (destStorageType== "cloud")
    {
        if (srcStorageType == "cloud")
         {
            if (!srcFileID.empty())
            {
                GDRIVE::GParent obj;
                vector<GDRIVE::GParent> vec;
                GDRIVE::GFile *movedFile = new GDRIVE::GFile();
                if (!destFileId.empty()) {
                    obj.set_id(destFileId); //setting parent object to folder id
                } else {
                    obj.set_id("root"); //setting parent object to folder id
                }
                vec.push_back(obj);
                movedFile->set_parents(vec);
                service.files().Copy(srcFileID, movedFile).execute();
                service.files().Delete(srcFileID).execute();
                respObj.put("returnValue", true);
                respObj.put("progress", 100);
                reqData->cb(respObj, reqData->subs);
                userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
                return;
            }
            else
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_SOURCE_PATH));
            }
        }
        else
        {
            std::string mimeType = getMimeType(srcPath.substr(srcPath.rfind(".")+1));
            if(mimeType.empty())
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_FILE);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_FILE));
                reqData->cb(respObj, reqData->subs);
                return;
            }
            ifstream fin(srcPath.c_str());
            if (!fin.good())
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::UNKNOWN_ERROR));
                reqData->cb(respObj, reqData->subs);
                return;
            }
            GDRIVE::FileContent fc(fin, mimeType);
            GDRIVE::GFile insertFile;
            GDRIVE::GParent obj;
            vector<GDRIVE::GParent> vec;
            if(!destFileId.empty()) {
                obj.set_id(destFileId);
            } else {
                obj.set_id("root");
            }
            vec.push_back(obj);
            insertFile.set_parents(vec);
            insertFile.set_title(srcPath.substr(srcPath.rfind("/")+1));
            insertFile.set_mimeType(mimeType);
            service.files().Insert(&insertFile, &fc).execute();
            fin.close();
            respObj.put("returnValue", true);
            respObj.put("progress", 100);
            auto intObj = InternalRemove(srcPath);
            (void)intObj;
            reqData->cb(respObj, reqData->subs);
            userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
        }
    }
    else
    {
        if (srcFileID.empty())
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_SOURCE_PATH));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        else if (!validateInternalPath(destPath))
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::INVALID_DEST_PATH);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_DEST_PATH));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        destPath = destPath + srcPath.substr(srcPath.rfind("/"));
        LOG_DEBUG_SAF(" DEST is NOT Cloud %s", destPath.c_str());
        GDRIVE::GFile downloadFile = service.files().Get(srcFileID).execute();
        string url = downloadFile.get_downloadUrl();
        if (url == "") {
            url = downloadFile.get_exportLinks()["application/pdf"];
        }
        GDRIVE::CredentialHttpRequest request(userDataObj.mCred.get(), url, GDRIVE::RM_GET);
        GDRIVE::HttpResponse resp = request.request();
        ofstream fout(destPath.c_str(), std::ios::binary);
        if(!fout.good()) {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::UNKNOWN_ERROR));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        fout.write(resp.content().c_str(), resp.content().size());
        fout.close();
        service.files().Delete(srcFileID).execute();
        respObj.put("returnValue", true);
        respObj.put("progress", 100);
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::remove(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("========> Remove API");
    std::string path = reqData->params["path"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];

    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string fileId = userDataObj.mGDriveOperObj.getFileId(path);
    LOG_DEBUG_SAF("========>FileID:%s", fileId.c_str());
    if (!fileId.empty())
    {
        GDRIVE::Drive service(userDataObj.mCred.get());
        service.files().Delete(fileId).execute();
        respObj.put("returnValue", true);
        respObj.put("status", "File Deleted");
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::rename(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(mDriveIdUserDataMap.find(driveId) == mDriveIdUserDataMap.end())
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(mDriveIdSessionMap[driveId] != reqData->sessionId)
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::STORAGE_TYPE_NOT_SUPPORTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    GDriveUserData &userDataObj = mDriveIdUserDataMap[driveId];

    if (userDataObj.mAuthParam["refresh_token"].empty())
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string fileId = userDataObj.mGDriveOperObj.getFileId(reqData->params["path"].asString());
    if (!fileId.empty())
    {
        GDRIVE::GFile patchFile;
        patchFile.set_title(reqData->params["newName"].asString());
        GDRIVE::Drive service(userDataObj.mCred.get());
        service.files().Patch(fileId, &patchFile).execute();
        respObj.put("returnValue", true);
        respObj.put("status", "File Renamed");
        reqData->cb(respObj, reqData->subs);
        userDataObj.mGDriveOperObj.loadFileIds(userDataObj.mCred);
        return;
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::listStoragesMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    pbnjson::JValue gdriveResArr = pbnjson::Array();
    for (auto entry : mDriveIdSessionMap)
    {
        pbnjson::JValue gdriveRes = pbnjson::Object();
        gdriveRes.put("driveName", GDRIVE_NAME);
        gdriveRes.put("driveId", entry.first);
        gdriveRes.put("path", "/");
        gdriveResArr.append(gdriveRes);
    }
    respObj.put("cloud", gdriveResArr);
    reqData->params.put("response", respObj);
    reqData->cb(reqData->params, std::move(reqData->subs));
}

void GDriveProvider::eject(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", false);
    respObj.put("errorCode", SAFErrors::SAFErrors::UNKNOWN_ERROR);
    respObj.put("errorText", "Not supported yet");
    reqData->params.put("response", respObj);
    reqData->cb(reqData->params, std::move(reqData->subs));
}

void GDriveProvider::insertMimeTypes() {
    mimetypesMap.insert({"xls","application/vnd.ms-excel"});
    mimetypesMap.insert({"xlsx","application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"});
    mimetypesMap.insert({"xml","text/xml"});
    mimetypesMap.insert({"ods","application/vnd.oasis.opendocument.spreadsheet"});
    mimetypesMap.insert({"csv","text/plain"});
    mimetypesMap.insert({"tmpl","text/plain"});
    mimetypesMap.insert({"pdf","application/pdf"});
    mimetypesMap.insert({"php","application/x-httpd-php"});
    mimetypesMap.insert({"jpg","image/jpeg"});
    mimetypesMap.insert({"png","image/png"});
    mimetypesMap.insert({"gif","image/gif"});
    mimetypesMap.insert({"bmp","image/bmp"});
    mimetypesMap.insert({"txt","text/plain"});
    mimetypesMap.insert({"doc","application/msword"});
    mimetypesMap.insert({"js","text/js"});
    mimetypesMap.insert({"swf","application/x-shockwave-flash"});
    mimetypesMap.insert({"mp3","audio/mpeg"});
    mimetypesMap.insert({"zip","application/zip"});
    mimetypesMap.insert({"rar","application/rar"});
    mimetypesMap.insert({"tar","application/tar"});
    mimetypesMap.insert({"arj","application/arj"});
    mimetypesMap.insert({"cab","application/cab"});
    mimetypesMap.insert({"html","text/html"});
    mimetypesMap.insert({"htm","text/html"});
    mimetypesMap.insert({"default","application/octet-stream"});
    mimetypesMap.insert({"folder","application/vnd.google-apps.folder"});
}

string GDriveProvider::getMimeType(std::string fileType) {
    std::transform(fileType.begin(), fileType.end(), fileType.begin(), ::tolower);
    return mimetypesMap[fileType];
    
}

string GDriveProvider::getFileType(std::string mimeType) {
    std::string fileType = "unknown";
    if(mimeType.rfind("folder") != std::string::npos)
        fileType = "directory";
    else if(!mimeType.empty())
        fileType = "regular";
    return fileType;
}

void GDriveProvider::copyFileinGDrive(GDRIVE::Drive service, string srcFileID, string destFolderId, string title) {
    GDRIVE::GParent obj;
    vector<GDRIVE::GParent> vec;
    GDRIVE::GFile copiedFile;
    if (!destFolderId.empty()) {
        obj.set_id(destFolderId);
    } else {
        obj.set_id("root");
    }
    vec.push_back(obj);
    copiedFile.set_parents(vec);
    copiedFile.set_title(title);
    service.files().Copy(srcFileID, &copiedFile).execute();
}

bool GDriveProvider::copyFilefromInternaltoGDrive(GDRIVE::Drive service, string srcPath, string destFolderId, string mimeType) {
    ifstream fin(srcPath.c_str());
    if (!fin.good()) {
        return false;
    }
    GDRIVE::FileContent fc(fin, mimeType);
    GDRIVE::GFile insertFile;
    GDRIVE::GParent obj;
    vector<GDRIVE::GParent> vec;
    if(!destFolderId.empty()) {
        obj.set_id(destFolderId);
    } else {
        obj.set_id("root");
    }
    vec.push_back(obj);
    insertFile.set_parents(vec);
    insertFile.set_title(srcPath.substr(srcPath.rfind("/")+1));
    insertFile.set_mimeType(mimeType);
    service.files().Insert(&insertFile, &fc).execute();
    fin.close();
    return true;
}

bool GDriveProvider::copyFilefromGDrivetoInternal(AuthParam authParam, GDRIVE::Drive service, string srcFileID, string destPath) {
    GDRIVE::GFile downloadFile = service.files().Get(srcFileID).execute();
    string url = downloadFile.get_downloadUrl();
    if (url == "") {
        url = downloadFile.get_exportLinks()["application/pdf"];
    }
    GDRIVE::Credential cred(&authParam);
    GDRIVE::CredentialHttpRequest request(&cred, url, GDRIVE::RM_GET);
    GDRIVE::HttpResponse resp = request.request();
    ofstream fout(destPath.c_str(), std::ios::binary);
    if(!fout.good()) {
        return false;
    }
    fout.write(resp.content().c_str(), resp.content().size());
    fout.close();
    return true;
}

void GDriveProvider::getFilesFromPath(vector<string> &filesVec, const string& path){
    size_t start;
    size_t end = 0;
    char delim = '/';
    filesVec.push_back("root");
    while ((start = path.find_first_not_of(delim, end)) != string::npos)
    {
        end = path.find(delim, start);
        filesVec.push_back(path.substr(start, end - start));
    }
}

string GDriveProvider::getFileID(GDRIVE::Drive service, const vector<string>& filesVec)
{
    vector<GDRIVE::GChildren> children;
    string fileID = filesVec.at(0);
    for (int i = 0; i < filesVec.size() - 1; i++) {
        children.clear();
        children = service.children().Listall(fileID);
        for (int j = 0; j < children.size(); j++) {
            GDRIVE::FileGetRequest get = service.files().Get(children[j].get_id());
            get.add_field("id,title,mimeType");
            GDRIVE::GFile file = get.execute();
            LOG_DEBUG_SAF("getFileID :: title : %s", file.get_title().c_str());
            if (file.get_title() == filesVec[i + 1]) {
                fileID = file.get_id();
                if (i == filesVec.size() - 2) {
                    return fileID;
                }
                break;
            }
        }
        children.clear();
    }
    return "";
}

void GDriveProvider::addRequest(std::shared_ptr<RequestData>& reqData)
{
    LOG_DEBUG_SAF("GDriveProvider :: Entering function %s", __FUNCTION__);
    {
        std::unique_lock < std::mutex > lock(mMutex);
        mQueue.push_back(std::move(reqData));
    }
    mCondVar.notify_one();

}

void GDriveProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    switch(reqData->methodType)
    {
        case MethodType::EXTRA_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EXTRA_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->extraMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::GET_PROP_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->list(reqData); });
            (void)fut;
        }
        break;
        case MethodType::GET_PROP_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::GET_PROP_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->getProperties(reqData); });
            (void)fut;
        }
        break;
        case MethodType::REMOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::REMOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->remove(reqData); });
            (void)fut;
        }
        break;
        case MethodType::MOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::MOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->move(reqData); });
            (void)fut;
        }
        break;
        case MethodType::COPY_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::COPY_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->copy(reqData); });
            (void)fut;
        }
        break;
        case MethodType::RENAME_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::RENAME_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->rename(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_STORAGES_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::LIST_STORAGES_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->listStoragesMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::EJECT_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EJECT_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->eject(reqData); });
            (void)fut;
        }
        break;
        default:
        break;
    }
}

void GDriveProvider::dispatchHandler()
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::unique_lock < std::mutex > lock(mMutex);
    do {
        mCondVar.wait(lock, [this] {
            return (mQueue.size() || mQuit);
        });
        LOG_DEBUG_SAF("Dispatch notif received : %d, mQuit: %d", mQueue.size(), mQuit);
        if (mQueue.size() && !mQuit)
        {
            auto request = mQueue.front();
            mQueue.erase(mQueue.begin());
            lock.unlock();
            handleRequests(request);
            lock.lock();
        }
    } while (!mQuit);
}

