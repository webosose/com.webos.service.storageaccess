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

#ifndef _GDRIVE_PROVIDER_H_
#define _GDRIVE_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include <iostream>

using namespace std;

class GDriveProvider: public DocumentProvider
{
public:
    GDriveProvider();
    virtual ~GDriveProvider();
    ReturnValue attachCloud(AuthParam authParam);
    ReturnValue authenticateCloud(AuthParam authParam);
    ReturnValue listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit);
    ReturnValue getProperties(AuthParam authParam);
    ReturnValue copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue remove(AuthParam authParam, string storageId, string path);
    ReturnValue eject(string storageId);
    ReturnValue format(string storageId, string fileSystem, string volumeLabel);

private:
};

#endif /* _GDRIVE_PROVIDER_H_ */