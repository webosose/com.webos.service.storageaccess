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

#ifndef _GDRIVE_PROVIDER_H_
#define _GDRIVE_PROVIDER_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"
#include "GDriveOperation.h"
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

#define GDRIVE_NAME "GDRIVE"

typedef struct GDriveUserData_
{
    GDriveOperation mGDriveOperObj;
    AuthParam mAuthParam;
    std::shared_ptr<GDRIVE::Credential> mCred;
    std::string mClientId;
    std::string mClientSecret;
} GDriveUserData;

class GDriveProvider: public DocumentProvider
{
public:
    GDriveProvider();
    virtual ~GDriveProvider();
    void addRequest(std::shared_ptr<RequestData>&);
    void dispatchHandler();
    void handleRequests(std::shared_ptr<RequestData>);
    void attachCloud(std::shared_ptr<RequestData> reqData);
    void authenticateCloud(std::shared_ptr<RequestData> reqData);
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
    void getFilesFromPath(std::vector<std::string> &, const std::string&);
    bool copyFilefromInternaltoGDrive(GDRIVE::Drive, std::string, std::string, std::string);
    bool copyFilefromGDrivetoInternal(AuthParam, GDRIVE::Drive, std::string, std::string);
    void copyFileinGDrive(GDRIVE::Drive, std::string, std::string, std::string);
    std::string getFileID(GDRIVE::Drive, const std::vector<std::string>&);
    void insertMimeTypes();
    std::string getMimeType(std::string);
    std::string getFileType(std::string);
    void setErrorMessage(std::shared_ptr<ValuePairMap>, std::string);
    bool validateExtraCommand(std::vector<std::string>, std::shared_ptr<RequestData>);
    map<std::string, std::string> mimetypesMap;
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit = false;

private:
    std::string generateDriveId();
    std::map<std::string, GDriveUserData> mDriveIdUserDataMap;
    std::map<std::string, std::string> mDriveIdSessionMap;
    std::map<std::string, std::string> mClientIdDriveId;
};

#endif /* _GDRIVE_PROVIDER_H_ */
