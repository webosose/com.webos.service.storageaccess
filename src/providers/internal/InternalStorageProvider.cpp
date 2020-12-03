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

ReturnValue InternalStorageProvider::listFolderContents(string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::getProperties() {
    return nullptr;
}

ReturnValue InternalStorageProvider::copy(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::move(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::remove(string storageId, string path)
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

fstream InternalStorageProvider::getFileStream(string path, ios_base::openmode mode)
{
    fstream fs;
    return fs;
}
