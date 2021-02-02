/* @@@LICENSE
 *
 * Copyright (c) 2021 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#ifndef _USB_STORAGE_PROVIDER_H_
#define _USB_STORAGE_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <luna-service2/lunaservice.hpp>
#include <pbnjson.hpp>

class USBStorageProvider: public DocumentProvider
{
public:
    USBStorageProvider();
    virtual ~USBStorageProvider();
    ReturnValue attachCloud(AuthParam authParam);
    ReturnValue authenticateCloud(AuthParam authParam);
    ReturnValue listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit);
    ReturnValue getProperties(AuthParam authParam);
    ReturnValue copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite);
    ReturnValue remove(AuthParam authParam, string storageId, string path);
    ReturnValue eject(string storageId);
    ReturnValue format(string storageId, string fileSystem, string volumeLabel);

    void dispatchHandler();
    void handleRequests(std::shared_ptr<RequestData>);
    void addRequest(std::shared_ptr<RequestData>&);
    void testMethod(std::shared_ptr<RequestData>);
    void listStoragesMethod(std::shared_ptr<RequestData>);
    void getPropertiesMethod(std::shared_ptr<RequestData>);
    void ejectMethod(std::shared_ptr<RequestData>);
    void formatMethod(std::shared_ptr<RequestData>);
    void copyMethod(std::shared_ptr<RequestData>);
    void moveMethod(std::shared_ptr<RequestData>);
    void removeMethod(std::shared_ptr<RequestData>);
    void renameMethod(std::shared_ptr<RequestData>);
    void listFolderContentsMethod(std::shared_ptr<RequestData>);
    static bool onReply(LSHandle*, LSMessage*, void*);
    static bool onGetPropertiesReply(LSHandle*, LSMessage*, void*);

private:
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit;
    static string storageId;
    static string driveName;
};

#endif /* _USB_STORAGE_PROVIDER_H_ */
