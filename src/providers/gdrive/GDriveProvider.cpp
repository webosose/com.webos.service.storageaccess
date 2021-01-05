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

#include <SAFLog.h>
#include "GDriveProvider.h"

GDriveProvider::GDriveProvider()
{
    LOG_DEBUG_SAF(" GDriveProvider::GDriveProvider : Constructor Created");
}

GDriveProvider::~GDriveProvider()
{
}

void GDriveProvider::setErrorMessage(shared_ptr<ValuePairMap> valueMap, string errorText)
{
    valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
    valueMap->emplace("errorText", pair<string, DataType>(errorText, DataType::STRING));
    valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
}
ReturnValue GDriveProvider::attachCloud(AuthParam authParam)
{
    shared_ptr<ContentList> authentify = make_shared<ContentList>();
    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    std::string authURL = "";

    Credential cred(&authParam);
    if (authParam["refresh_token"] == "")
    {
        OAuth oauth(authParam["clientID"], authParam["clientSecret"]);
        authURL = oauth.get_authorize_url();
        LOG_DEBUG_SAF("========> authorize_url:%s", authURL.c_str());
    }
    if (authURL.empty())
    {
        valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
        valueMap->emplace("errorText", pair<string, DataType>("Invalid URL", DataType::STRING));
        valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
    }
    else
    {
        valueMap->emplace("authURL", pair<string, DataType>(authURL, DataType::STRING));
        //valueMap->emplace("returnValue", pair<string, DataType>("True", DataType::BOOLEAN));
    }
    return make_shared<ResultPair>(valueMap, authentify);
}

ReturnValue GDriveProvider::authenticateCloud(AuthParam authParam)
{
    shared_ptr<ContentList> authentify = make_shared<ContentList>();
    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    Credential cred(&authParam);
    OAuth oauth(authParam["client_id"], authParam["client_secret"]);
    
    if (oauth.build_credential(authParam["secret_token"], cred))
    {
        valueMap->emplace("returnValue", pair<string, DataType>("true", DataType::BOOLEAN));
        valueMap->emplace("refresh_token", pair<string, DataType>(authParam["refresh_token"], DataType::STRING));
    }
    else
    {
        LOG_DEBUG_SAF("===> AuthenticateCloud Failed");
        valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
        valueMap->emplace("errorText", pair<string, DataType>("Invalid URL", DataType::STRING));
        valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
    }
    return make_shared<ResultPair>(valueMap, authentify);
}

ReturnValue GDriveProvider::listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue GDriveProvider::getProperties(AuthParam authParam)
{
    return nullptr;
}

ReturnValue GDriveProvider::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam,
        StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    shared_ptr<ContentList> contentList = make_shared<ContentList>();
    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    if (srcAuthParam["refresh_token"] == "") {
        setErrorMessage(valueMap, "Authentication Not Done");
        return make_shared<ResultPair>(valueMap, contentList);
    }
    Credential cred(&srcAuthParam);
    Drive service(&cred);

    vector<string> srcFilesVec, destFilesVec;
    getFilesFromPath(srcFilesVec, srcPath);
    getFilesFromPath(destFilesVec, destPath);

    string srcFileID = (srcFilesVec.size() > 0) ? getFileID(service, srcFilesVec) : "";
    string destFolderId = (destFilesVec.size() > 0) ? getFileID(service, destFilesVec) : "";

    if (srcStorageType == destStorageType) {
        if (srcFileID != "") {
            destFilesVec.push_back(srcFilesVec.at(srcFilesVec.size() - 1));
            string fileID = (destFilesVec.size() > 0) ? getFileID(service, destFilesVec) : "";
            if (overwrite) {
                if (fileID != "") service.files().Delete(fileID).execute();
            } else {
                if (fileID != "") {
                    setErrorMessage(valueMap, "File Already exist");
                    return make_shared<ResultPair>(valueMap, contentList);
                }
            }
            string title = srcFilesVec.at(srcFilesVec.size() - 1);
            copyFileinGDrive(service, srcFileID, destFolderId, title);
        } else {
            setErrorMessage(valueMap, "Invalid Source Path");
        }
    } else {
    }
    return make_shared<ResultPair>(valueMap, contentList);
}

ReturnValue GDriveProvider::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam,
        StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue GDriveProvider::remove(AuthParam authParam, string storageId, string path)
{
    return nullptr;
}

ReturnValue GDriveProvider::eject(string storageId)
{
    return nullptr;
}

ReturnValue GDriveProvider::format(string storageId, string fileSystem, string volumeLabel)
{
    return nullptr;
}

void GDriveProvider::copyFileinGDrive(Drive service, string srcFileID, string destFolderId, string title)
{
    GParent obj;
    vector<GParent> vec;
    GFile copiedFile;
    if (!(destFolderId == "")) {
        obj.set_id(destFolderId);
    } else {
        obj.set_id("root");
    }
    vec.push_back(obj);
    copiedFile.set_parents(vec);
    copiedFile.set_title(title);
    service.files().Copy(srcFileID, &copiedFile).execute();
}

void GDriveProvider::getFilesFromPath(vector<string> &filesVec, const string& path)
{
    size_t start;
    size_t end = 0;
    char delim = '/';
    filesVec.push_back("root");
    while ((start = path.find_first_not_of(delim, end)) != string::npos) {
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
