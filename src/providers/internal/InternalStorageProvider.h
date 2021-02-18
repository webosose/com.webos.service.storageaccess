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

#ifndef _INTERNAL_STORAGE_PROVIDER_H_
#define _INTERNAL_STORAGE_PROVIDER_H_

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

#define DEFAULT_INTERNAL_PATH "/tmp/internal"
#define DEFAULT_INTERNAL_STORAGE_ID	"INTERNAL_STORAGE"
class InternalStorageProvider: public DocumentProvider
{
public:
    InternalStorageProvider();
    virtual ~InternalStorageProvider();
	void addRequest(std::shared_ptr<RequestData>&);
	void dispatchHandler();
    void handleRequests(std::shared_ptr<RequestData>);
	void listStoragesMethod(std::shared_ptr<RequestData> reqData);
    void listFolderContents(std::shared_ptr<RequestData> reqData);
	void getProperties(std::shared_ptr<RequestData> reqData);
    void copy(std::shared_ptr<RequestData> reqData);
    void move(std::shared_ptr<RequestData> reqData);
    void remove(std::shared_ptr<RequestData> reqData);
	void rename(std::shared_ptr<RequestData> reqData);
    void eject(std::shared_ptr<RequestData> reqData);
    static bool onReply(LSHandle*, LSMessage*, void*);

private:
    std::vector<std::shared_ptr<RequestData>> mQueue;
    std::thread mDispatcherThread;
    std::mutex mMutex;
    std::condition_variable mCondVar;
    volatile bool mQuit;

};

#endif /* _INTERNAL_STORAGE_PROVIDER_H_ */
