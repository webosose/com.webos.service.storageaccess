/* @@@LICENSE
 *
 * Copyright (c) 2020-2021 LG Electronics, Inc.
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
#include "InternalOperationHandler.h"

GDriveProvider::GDriveProvider() : mQuit(false),
    mCred(nullptr), mUser("usetest360@gmail.com")
{
    LOG_DEBUG_SAF(" GDriveProvider::GDriveProvider : Constructor Created");
    mDispatcherThread = std::thread(std::bind(&GDriveProvider::dispatchHandler, this));
    mDispatcherThread.detach();
    insertMimeTypes();
}

GDriveProvider::~GDriveProvider()
{
    mQuit = true;
    if (mDispatcherThread.joinable())
    {
        mDispatcherThread.join();
    }
}

void GDriveProvider::setErrorMessage(shared_ptr<ValuePairMap> valueMap, string errorText)
{
    valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
    valueMap->emplace("errorText", pair<string, DataType>(errorText, DataType::STRING));
    valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
}

void GDriveProvider::attachCloud(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string authURL;
    if (mAuthParam["refresh_token"].empty())
    {
        mAuthParam["client_id"] = reqData->params["commandArgs"]["clientId"].asString();
        mAuthParam["client_secret"] = reqData->params["commandArgs"]["clientSecret"].asString();
        OAuth oauth(mAuthParam["client_id"], mAuthParam["client_secret"]);
        authURL = oauth.get_authorize_url();
        LOG_DEBUG_SAF("attachCloud :client_id = [ %s ]   client_secret = [ %s ]", mAuthParam["client_id"].c_str(), mAuthParam["client_secret"].c_str());
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
        respObj.put("returnValue", true);
        respObj.put("authURL", authURL);
        LOG_DEBUG_SAF("========> authorize_url:%s", authURL.c_str());
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::authenticateCloud(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    mAuthParam["access_token"] = reqData->params["commandArgs"]["secretToken"].asString();
    LOG_DEBUG_SAF("authenticateCloud :client_id = [ %s ]   client_secret = [ %s ]   secret_token = [ %s ]", mAuthParam["client_id"].c_str(),
        mAuthParam["client_secret"].c_str(), mAuthParam["access_token"].c_str());
    while (mCred.use_count() != 0)
        mCred.reset();
    mCred = std::shared_ptr<Credential>(new Credential(&mAuthParam));
    LOG_DEBUG_SAF("authenticateCloudTest 1");
    OAuth oauth(mAuthParam["client_id"], mAuthParam["client_secret"]);
    LOG_DEBUG_SAF("authenticateCloudTest 2");

    if (!mAuthParam["access_token"].empty()
        && oauth.build_credential(mAuthParam["access_token"], *(mCred.get())))
    {
        respObj.put("returnValue", true);
        respObj.put("refreshToken", mAuthParam["refresh_token"]);
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_URL);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_URL));
    }
    reqData->cb(respObj, reqData->subs);
    mGDriveOperObj.loadFileIds(mCred);
}

void GDriveProvider::list(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string path = reqData->params["path"].asString();
    int limit = reqData->params["limit"].asNumber<int>();
    int offset = reqData->params["offset"].asNumber<int>();
    if (reqData->params.hasKey("refresh") && reqData->params["refresh"].asBool())
        mGDriveOperObj.loadFileIds(mCred);
    pbnjson::JValue respObj = pbnjson::Object();
    if (mAuthParam["refresh_token"].empty())
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string folderpathId = mGDriveOperObj.getFileId(path);
    if (!folderpathId.empty())
    {
        Drive service(mCred.get());
        pbnjson::JValue contentsObj = pbnjson::Array();
        auto fileIdsMap = mGDriveOperObj.getFileMap(path);
        int start = (offset > (int)fileIdsMap.size())?(fileIdsMap.size() + 1):(offset - 1);
        start = (start < 0)?(fileIdsMap.size() + 1):(start);
        int end = ((limit + offset - 1) >  fileIdsMap.size())?(fileIdsMap.size()):(limit + offset - 1);
        end = (end < 0)?(fileIdsMap.size()):(end);
        int index = 0;
        for(auto entry : mGDriveOperObj.getFileMap( path))
        {
            if (index < start)
            {
                index++;
                continue;
            }
            if (index >= end)   break;
            index++;
            FileGetRequest get = service.files().Get(entry.second);
            get.add_field("id,title,mimeType,fileSize");
            GFile file = get.execute();
            GAbout about = service.about().Get().execute();
            long totalspace = about.get_quotaBytesTotal();
            LOG_DEBUG_SAF("Id %s", file.get_id().c_str());
            LOG_DEBUG_SAF("Title %s", file.get_title().c_str());
            LOG_DEBUG_SAF("MimeType %s", file.get_mimeType().c_str());
            LOG_DEBUG_SAF("File Size %ld", file.get_fileSize());
            pbnjson::JValue contentObj = pbnjson::Object();
            contentObj.put("itemPath", entry.first);
            //contentObj.put("id", file.get_id());
            contentObj.put("itemName", file.get_title());
            contentObj.put("itemType", file.get_mimeType());
            if (-1 == file.get_fileSize())
                contentObj.put("size", 0);
            else
                contentObj.put("size", (totalspace - file.get_fileSize()));
            contentsObj.append(contentObj);
        }
        respObj.put("returnValue", true);
        respObj.put("contents", contentsObj);
        respObj.put("fullPath", path);
        respObj.put("totalCount", contentsObj.arraySize());
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::getProperties(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    if (mAuthParam["refresh_token"].empty())
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    Drive service(mCred.get());
    FileGetRequest get = service.files().Get("root");
    get.add_field("userPermission");
    GFile file = get.execute();
    mUser = file.get_userPermission().get_emailAddress();
    GAbout about = service.about().Get().execute();
    string username = about.get_name();
    long totalspace = about.get_quotaBytesTotal();
    long freespace = about.get_quotaBytesUsed();
    LOG_DEBUG_SAF("==>Usrename:%s", username.c_str());
    LOG_DEBUG_SAF("==>TotalSpace:%ld", totalspace);
    LOG_DEBUG_SAF("==>FreeSpace:%ld", freespace);
    respObj.put("returnValue", true);
    respObj.put("storageType", "CLOUD");
    respObj.put("writable", true);
    respObj.put("deletable", false);
    //respObj.put("userName", username);
    //respObj.put("user", mUser);
    respObj.put("totalSpace", totalspace / 1000000);
    respObj.put("freeSpace", freespace / 1000000);
    reqData->cb(respObj, reqData->subs);
}

void GDriveProvider::copy(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    std::string srcStorageType = reqData->params["srcStorageType"].asString();
    std::string destStorageType = reqData->params["destStorageType"].asString();
    bool overwrite = reqData->params["overwrite"].asBool();
    if(reqData->params.hasKey("subscribe"))
        bool subscribe = reqData->params["subscribe"].asBool();
    auto obj = InternalCreateDir(destPath);
    if (mAuthParam["refresh_token"].empty() || (obj.getStatus() < 0))
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string srcFileID = mGDriveOperObj.getFileId(srcPath);
    std::string destFileId = mGDriveOperObj.getFileId(destPath);
    Drive service(mCred.get());
    if (destStorageType== "CLOUD")
    {
        if (srcStorageType == "CLOUD")
        {
            if (!srcFileID.empty())
            {
                GParent obj;
                vector<GParent> vec;
                GFile *movedFile = new GFile();
                if (!destFileId.empty()) {
                    obj.set_id(destFileId); //setting parent object to folder id
                } else {
                    obj.set_id("root"); //setting parent object to folder id
                }
                vec.push_back(obj);
                movedFile->set_parents(vec);
                service.files().Copy(srcFileID, movedFile).execute();
                respObj.put("returnValue", true);
            }
            else
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_SOURCE_PATH);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_SOURCE_PATH));
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
                respObj.put("errorCode", SAFErrors::CloudErrors::UNKNOWN_ERROR);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::UNKNOWN_ERROR));
                reqData->cb(respObj, reqData->subs);
                return;
            }

            FileContent fc(fin, mimeType);
            GFile insertFile;
            GParent obj;
            vector<GParent> vec;
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
            respObj.put("status", "File Copied");
        }
    }
    else
    {
        destPath = destPath + srcPath.substr(srcPath.rfind("/"));
                LOG_DEBUG_SAF(" DEST is NOT Cloud %s", destPath.c_str());
        GFile downloadFile = service.files().Get(srcFileID).execute();
        string url = downloadFile.get_downloadUrl();
        if (url == "") {
            url = downloadFile.get_exportLinks()["application/pdf"];
        }
        CredentialHttpRequest request(mCred.get(), url, RM_GET);
        HttpResponse resp = request.request();
        ofstream fout(destPath.c_str(), std::ios::binary);
        if(!fout.good()) {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::CloudErrors::UNKNOWN_ERROR);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::UNKNOWN_ERROR));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        fout.write(resp.content().c_str(), resp.content().size());
        fout.close();
        respObj.put("returnValue", true);
        respObj.put("status", "File Copied");
    }
    reqData->cb(respObj, reqData->subs);
    mGDriveOperObj.loadFileIds(mCred);
}

void GDriveProvider::move(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    std::string srcStorageType = reqData->params["srcStorageType"].asString();
    std::string destStorageType = reqData->params["destStorageType"].asString();
    bool overwrite = reqData->params["overwrite"].asBool();
    if(reqData->params.hasKey("subscribe"))
        bool subscribe = reqData->params["subscribe"].asBool();

    auto obj = InternalCreateDir(destPath);
    if (mAuthParam["refresh_token"].empty() || (obj.getStatus() < 0))
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string srcFileID = mGDriveOperObj.getFileId(srcPath);
    std::string destFileId = mGDriveOperObj.getFileId(destPath);
    Drive service(mCred.get());
    if (destStorageType== "CLOUD")
    {
        if (srcStorageType == "CLOUD")
         {
            if (!srcFileID.empty())
            {
                GParent obj;
                vector<GParent> vec;
                GFile *movedFile = new GFile();
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
                respObj.put("status", "File Moved");
            }
            else
            {
                respObj.put("returnValue", false);
                respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_SOURCE_PATH);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_SOURCE_PATH));
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
                respObj.put("errorCode", SAFErrors::CloudErrors::UNKNOWN_ERROR);
                respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::UNKNOWN_ERROR));
                reqData->cb(respObj, reqData->subs);
                return;
            }
            FileContent fc(fin, mimeType);
            GFile insertFile;
            GParent obj;
            vector<GParent> vec;
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
            respObj.put("status", "File Moved");
            auto intObj = InternalRemove(srcPath);
            (void)intObj;
        }
    }
    else
    {
        destPath = destPath + srcPath.substr(srcPath.rfind("/"));
        LOG_DEBUG_SAF(" DEST is NOT Cloud %s", destPath.c_str());
        GFile downloadFile = service.files().Get(srcFileID).execute();
        string url = downloadFile.get_downloadUrl();
        if (url == "") {
            url = downloadFile.get_exportLinks()["application/pdf"];
        }
        CredentialHttpRequest request(mCred.get(), url, RM_GET);
        HttpResponse resp = request.request();
        ofstream fout(destPath.c_str(), std::ios::binary);
        if(!fout.good()) {
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::CloudErrors::UNKNOWN_ERROR);
            respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::UNKNOWN_ERROR));
            reqData->cb(respObj, reqData->subs);
            return;
        }
        fout.write(resp.content().c_str(), resp.content().size());
        fout.close();
        service.files().Delete(srcFileID).execute();
        respObj.put("returnValue", true);
        respObj.put("status", "File Moved");
    }
    reqData->cb(respObj, reqData->subs);
    mGDriveOperObj.loadFileIds(mCred);
}

void GDriveProvider::remove(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("========> Remove API");
    std::string path = reqData->params["path"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if (mAuthParam["refresh_token"].empty())
    {
        LOG_DEBUG_SAF("===> Authentication Not Done");
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::AUTHENTICATION_NOT_DONE));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string fileId = mGDriveOperObj.getFileId(path);
    LOG_DEBUG_SAF("========>FileID:%s", fileId.c_str());
    if (!fileId.empty())
    {
        Drive service(mCred.get());
        service.files().Delete(fileId).execute();
        respObj.put("returnValue", true);
        respObj.put("status", "File Deleted");
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
    mGDriveOperObj.loadFileIds(mCred);
}

void GDriveProvider::rename(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string fileId = mGDriveOperObj.getFileId(reqData->params["path"].asString());
    if (!fileId.empty())
    {
        GFile patchFile;
        patchFile.set_title(reqData->params["newName"].asString());
        Drive service(mCred.get());
        service.files().Patch(fileId, &patchFile).execute();
        respObj.put("returnValue", true);
        respObj.put("staus", "File Renamed");
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::CloudErrors::INVALID_PATH);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::CloudErrors::INVALID_PATH));
    }
    reqData->cb(respObj, reqData->subs);
    mGDriveOperObj.loadFileIds(mCred);
}

void GDriveProvider::listStoragesMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    pbnjson::JValue gdriveResArr = pbnjson::Array();
    pbnjson::JValue gdriveRes = pbnjson::Object();
    gdriveRes.put("storgaeType", "GDRIVE");
    gdriveRes.put("storgaeId", mUser);
    gdriveResArr.append(gdriveRes);
    respObj.put("CLOUD", gdriveResArr);
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

void GDriveProvider::copyFileinGDrive(Drive service, string srcFileID, string destFolderId, string title) {
    GParent obj;
    vector<GParent> vec;
    GFile copiedFile;
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

bool GDriveProvider::copyFilefromInternaltoGDrive(Drive service, string srcPath, string destFolderId, string mimeType) {
    ifstream fin(srcPath.c_str());
    if (!fin.good()) {
        return false;
    }
    FileContent fc(fin, mimeType);
    GFile insertFile;
    GParent obj;
    vector<GParent> vec;
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

bool GDriveProvider::copyFilefromGDrivetoInternal(AuthParam authParam, Drive service, string srcFileID, string destPath) {
    GFile downloadFile = service.files().Get(srcFileID).execute();
    string url = downloadFile.get_downloadUrl();
    if (url == "") {
        url = downloadFile.get_exportLinks()["application/pdf"];
    }
    Credential cred(&authParam);
    CredentialHttpRequest request(&cred, url, RM_GET);
    HttpResponse resp = request.request();
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

string GDriveProvider::getFileID(Drive service, const vector<string>& filesVec)
{
    vector<GChildren> children;
    string fileID = filesVec.at(0);
    for (int i = 0; i < filesVec.size() - 1; i++) {
        children.clear();
        children = service.children().Listall(fileID);
        for (int j = 0; j < children.size(); j++) {
            FileGetRequest get = service.files().Get(children[j].get_id());
            get.add_field("id,title,mimeType");
            GFile file = get.execute();
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
    mQueue.push_back(std::move(reqData));
    mCondVar.notify_one();
}

void GDriveProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    switch(reqData->methodType)
    {
        case MethodType::ATTACH_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::ATTACH_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->attachCloud(reqData); });
            (void)fut;
        }
        break;
        case MethodType::AUTHENTICATE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::AUTHENTICATE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->authenticateCloud(reqData); });
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
            lock.unlock();
            handleRequests(std::move(mQueue.front()));
            mQueue.erase(mQueue.begin());
            lock.lock();
        }
    } while (!mQuit);
}

