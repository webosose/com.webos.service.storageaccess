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

class USBDriveInfo
{
public :
    string mDriveName;
    string mMountPath;
    string mFsType;
    string mUuid;
    string mVolumeLabel;
    bool mIsMounted;
};

class USBDeviceInfo
{
public :
    string mStorageType;
    string mSerialNumber;
    int mDeviceNumber;
    string mDeviceSetId;
    vector<shared_ptr<USBDriveInfo>> mStorageDriveList;
};

class USBAttached
{
public:
    std::map<std::string, std::shared_ptr<USBDeviceInfo>> usbStorages;
};

class USBStorageProvider: public DocumentProvider
{
public:
    USBStorageProvider();
    virtual ~USBStorageProvider();
	void addRequest(std::shared_ptr<RequestData>&);
    void dispatchHandler();
    void handleRequests(std::shared_ptr<RequestData>);
    void listStoragesMethod(std::shared_ptr<RequestData>);
    void getPropertiesMethod(std::shared_ptr<RequestData>);
    void ejectMethod(std::shared_ptr<RequestData>);
    void copyMethod(std::shared_ptr<RequestData>);
    void moveMethod(std::shared_ptr<RequestData>);
    void removeMethod(std::shared_ptr<RequestData>);
    void renameMethod(std::shared_ptr<RequestData>);
    void listFolderContentsMethod(std::shared_ptr<RequestData>);
    void populateDeviceInfo(pbnjson::JValue);
    void printUSBInfo();
    std::string getDriveName(std::string);
    std::string getDriveName(std::string, std::string);
    std::string getStorageType(std::string);
    bool isStorageIdExists(std::string);
    void cleanDeviceInfo();
    bool isStorageDriveMounted(std::string);
    int getStorageNumber(std::string);
    static bool onReply(LSHandle*, LSMessage*, void*);
    static bool onGetPropertiesReply(LSHandle*, LSMessage*, void*);
    static bool onListStoragesMethodReply(LSHandle*, LSMessage*, void*);

private:
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit;
    std::shared_ptr<USBAttached> deviceInfo;
};

#endif /* _USB_STORAGE_PROVIDER_H_ */
