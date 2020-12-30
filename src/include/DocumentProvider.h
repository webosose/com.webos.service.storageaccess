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

#ifndef _DOCUMENT_PROVIDER_H_
#define _DOCUMENT_PROVIDER_H_


#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "SA_Common.h"

using namespace std;

class DocumentProvider
{
public:
    DocumentProvider() = default;
    virtual ~DocumentProvider() = default;
    virtual ReturnValue attachCloud(AuthParam authParam) = 0;
    virtual ReturnValue authenticateCloud(AuthParam authParam) = 0;
    virtual ReturnValue listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit) = 0;
    virtual ReturnValue getProperties(AuthParam authParam) = 0;
    virtual ReturnValue copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite) = 0;
    virtual ReturnValue move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite) = 0;
    virtual ReturnValue remove(AuthParam srcAuthParam, string storageId, string path) = 0;
    virtual ReturnValue eject(string storageId) = 0;
    virtual ReturnValue format(string storageId, string fileSystem, string volumeLabel) = 0;
};

#endif /* _DOCUMENT_PROVIDER_H_ */
