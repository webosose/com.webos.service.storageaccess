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


#include "GDriveProvider.h"

GDriveProvider::GDriveProvider()
{
}

GDriveProvider::~GDriveProvider()
{
}

ReturnValue GDriveProvider::listFolderContents(string storageId, string path, int offset, int limit)
{
    return nullptr;
}

ReturnValue GDriveProvider::getProperties() {
    return nullptr;
}

ReturnValue GDriveProvider::copy(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue GDriveProvider::move(string srcStorageId, string srcPath, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue GDriveProvider::remove(string storageId, string path)
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

fstream GDriveProvider::getFileStream(string path, ios_base::openmode mode)
{
    fstream fs;
    return fs;
}
