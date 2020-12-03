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

    ReturnValue listOfStoreages();
    ReturnValue listFolderContents(string storageType, string storageId, string path, int offset, int limit);
    ReturnValue getProperties(string storageType);
    ReturnValue copy(string srcStorageType, string srcStorageId, string srcPath, string destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue move(string srcStorageType, string srcStorageId, string srcPath, string destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue remove(string storageType, string storageId, string path);
    ReturnValue eject(string storageType, string storageId);
    ReturnValue format(string storageType, string storageId, string fileSystem, string volumeLabel);
    /*virtual bool Authendication(string deviceId) = 0;*/
};

#endif /* _DOCUMENT_PROVIDER_MANAGER_H_ */
