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


using namespace std;
using namespace GDRIVE;

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
    void attachServer(std::shared_ptr<RequestData> reqData);
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
    map<string,string> mntpathmap;
    void getFilesFromPath(vector<string> &, const string&);
    bool copyFilefromInternaltoGDrive(Drive, string, string, string);
    bool copyFilefromGDrivetoInternal(AuthParam, Drive, string, string);
    void copyFileinGDrive(Drive, string, string, string);
    string getFileID(Drive, const vector<string>&);
    void insertMimeTypes();
    string getMimeType(string);
    std::string getFileType(std::string);
    void setErrorMessage(shared_ptr<ValuePairMap>, string);
	bool validateExtraCommand(std::vector<std::string>, std::shared_ptr<RequestData>);
    map<string, string> mimetypesMap;
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit = false;

private:
    GDriveOperation mGDriveOperObj;
    AuthParam mAuthParam;
    std::shared_ptr<Credential> mCred;
    std::string mUser;
	std::string mClientId;
    std::string mClientSecret;
};

#endif /* _GDRIVE_PROVIDER_H_ */
