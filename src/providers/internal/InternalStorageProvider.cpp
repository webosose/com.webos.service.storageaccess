/* @@@LICENSE
 *
 * Copyright (c) 2020 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include "InternalStorageProvider.h"

using namespace std;

InternalStorageProvider::InternalStorageProvider()
{
}

InternalStorageProvider::~InternalStorageProvider()
{
}

ReturnValue InternalStorageProvider::attachCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::authenticateCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::getProperties(AuthParam authParam) {
    return nullptr;
}

ReturnValue InternalStorageProvider::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::remove(AuthParam authParam, string storageId, string path)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::eject(string storageId)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::format(string storageId, string fileSystem, string volumeLabel)
{
    return nullptr;
}

