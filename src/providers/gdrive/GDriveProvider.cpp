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

ReturnValue GDriveProvider::attachCloud(AuthParam authParam)
{
    shared_ptr<ContentList> authentify = make_shared<ContentList>();
    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    std::string authURL = "";

    //Actual Functionality Needs to added here

    valueMap->emplace("authURL", pair<string, DataType>("http://google.com/Parmi", DataType::STRING));
    LOG_DEBUG_SAF("GDriveProvider::attachCloud : Last Function Called");
    return make_shared<ResultPair>(valueMap, authentify);
}

ReturnValue GDriveProvider::authenticateCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue GDriveProvider::listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue GDriveProvider::getProperties(AuthParam authParam) {
    return nullptr;
}

ReturnValue GDriveProvider::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue GDriveProvider::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
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

