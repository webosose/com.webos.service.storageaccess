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


#ifndef _DOCUMENT_PROVIDER_MANAGER_H_
#define _DOCUMENT_PROVIDER_MANAGER_H_

#include <string>
#include <vector>
#include <memory>
#include "SA_Common.h"
#include "DocumentProviderFactory.h"

class DocumentProviderManager {
public:
    DocumentProviderManager();
    ~DocumentProviderManager();

    ReturnValue listOfStorages();
    ReturnValue attachCloud(StorageType storageType, AuthParam authParam);
    ReturnValue authenticateCloud(StorageType storageType, AuthParam authParam);
    ReturnValue listFolderContents(AuthParam authParam, StorageType storageType, string storageId, string path, int offset, int limit);
    ReturnValue getProperties(AuthParam authParam, StorageType storageType);
    ReturnValue copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue remove(AuthParam srcAuthParam, StorageType storageType, string storageId, string path);
    ReturnValue eject(StorageType storageType, string storageId);
    ReturnValue format(StorageType storageType, string storageId, string fileSystem, string volumeLabel);

	void addRequest(std::shared_ptr<RequestData>&);
};

#endif /* _DOCUMENT_PROVIDER_MANAGER_H_ */
