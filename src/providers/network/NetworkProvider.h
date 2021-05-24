/* @@@LICENSE
 *
 * Copyright (c) 2020-2021 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#ifndef _NETWORK_PROVIDER_H_
#define _NETWORK_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include <iostream>
#include "gdrive/gdrive.hpp"
#include <assert.h>
#include <vector>
#include <SAFLog.h>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <SAFErrors.h>
#include <sys/mount.h>

#define SAMBA_NAME "SAMBA"
#define UPNP_NAME  "UPNP"

class NetworkProvider: public DocumentProvider
{
public:
    NetworkProvider();
    virtual ~NetworkProvider();
    void addRequest(std::shared_ptr<RequestData>&);
    void dispatchHandler();
    void handleRequests(std::shared_ptr<RequestData>);
    void mountSambaServer(std::shared_ptr<RequestData> reqData);
    void discoverUPnPMediaServer(std::shared_ptr<RequestData> reqData);
    void list(std::shared_ptr<RequestData> reqData);
    void getProperties(std::shared_ptr<RequestData> reqData);
    void remove(std::shared_ptr<RequestData> reqData);
    void copy(std::shared_ptr<RequestData> reqData);
    void move(std::shared_ptr<RequestData> reqData);
    void rename(std::shared_ptr<RequestData> reqData);
    void listStoragesMethod(std::shared_ptr<RequestData> reqData);
    void extraMethod(std::shared_ptr<RequestData> reqData);
    void eject(std::shared_ptr<RequestData> reqData);

private:
    pbnjson::JValue parseMediaServer(std::string);
    std::map<std::string, std::string> mSambaDriveMap;
    std::map<std::string, std::string> mSambaSessionData;
    std::map<std::string, std::string> mSambaPathMap;
    std::string generateUniqueSambaDriveId();
    std::map<std::string, std::string> mUpnpDriveMap;
    std::map<std::string, std::string> mUpnpSessionData;
    std::map<std::string, std::string> mUpnpPathMap;
    std::string generateUniqueUpnpDriveId();
    std::string getTimestamp();
    std::map<std::string, std::string> mntpathmap;
    bool validateSambaOperation(std::string, std::string);
    bool validateUpnpOperation(std::string, std::string);
    void setErrorMessage(shared_ptr<ValuePairMap>, std::string);
    bool validateExtraCommand(std::vector<std::string>, std::shared_ptr<RequestData>);
    std::map<std::string, std::string> mimetypesMap;
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit = false;

};

#endif /* _NETWORK_PROVIDER_H_ */
