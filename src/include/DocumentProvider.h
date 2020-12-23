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
    virtual ReturnValue listFolderContents(string storageId, string path, int offset, int limit) = 0;
    virtual ReturnValue getProperties() = 0;
    virtual ReturnValue copy(StorageType srcStorageType, string srcStorageId, string srcPath, StorageType destStorageType, string destStorageId, string destPath, bool overwrite) = 0;
    virtual ReturnValue move(StorageType srcStorageType, string srcStorageId, string srcPath, StorageType destStorageType, string destStorageId, string destPath, bool overwrite) = 0;
    virtual ReturnValue remove(string storageId, string path) = 0;
    virtual ReturnValue eject(string storageId) = 0;
    virtual ReturnValue format(string storageId, string fileSystem, string volumeLabel) = 0;
   /* virtual ReturnValue Authendication(string deviceId) = 0;*/
};

#endif /* _DOCUMENT_PROVIDER_H_ */
