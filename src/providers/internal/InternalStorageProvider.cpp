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
#include <functional>
#include <future>
#include <pbnjson.hpp>
#include "SA_Common.h"
#include "SAFLunaService.h"
#include "InternalStorageProvider.h"
#include "InternalOperationHandler.h"

using namespace std;

InternalStorageProvider::InternalStorageProvider() : mQuit(false)
{
    mDispatcherThread = std::thread(std::bind(&InternalStorageProvider::dispatchHandler, this));
    mDispatcherThread.detach();
}

InternalStorageProvider::~InternalStorageProvider()
{
    mQuit = true;
    if (mDispatcherThread.joinable())
    {
        mDispatcherThread.join();
    }
}

ReturnValue InternalStorageProvider::attachCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::authenticateCloud(AuthParam authParam)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::listFolderContents(AuthParam authParam, string storageId, string path, int offset, int limit)
{
    return nullptr;
}

void InternalStorageProvider::listStoragesMethod(std::shared_ptr<RequestData> reqData)
{
}

void InternalStorageProvider::listFolderContents(std::shared_ptr<RequestData> reqData)
{
    std::string path = reqData->params["path"].asString();
    int offset = reqData->params["offset"].asNumber<int>();
    int limit = reqData->params["limit"].asNumber<int>();

    bool status = false;
    int totalCount = 0;
    std::string fullPath;
    pbnjson::JValue contenResArr = pbnjson::Array();
    std::unique_ptr<FolderContents> contsPtr = InternalOperationHandler::getInstance().getListFolderContents(path);
    fullPath = contsPtr->getPath();
    totalCount = contsPtr->getTotalCount();
    if (contsPtr->getStatus() >= 0)
    {
        auto contVec = contsPtr->getContents();
        int start = (offset > (int)contVec.size())?(contVec.size() + 1):(offset - 1);
        start = (start < 0)?(contVec.size() + 1):(start);
        int end = ((limit + offset - 1) >  contVec.size())?(contVec.size()):(limit + offset - 1);
        end = (end < 0)?(contVec.size()):(end);
        LOG_DEBUG_SAF("%s: start:%d, end: %d", __FUNCTION__,start,end);
        for (int index = start; index < end; ++index)
        {
            pbnjson::JValue contentObj = pbnjson::Object();
            contentObj.put("itemName", contVec[index]->getName());
            contentObj.put("itemPath", contVec[index]->getPath());
            contentObj.put("itemType", contVec[index]->getType());
            contentObj.put("size", int(contVec[index]->getSize()));
            contenResArr.append(contentObj);
        }
        status = true;
    }
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (status)
    {
        respObj.put("contents", contenResArr);
        respObj.put("totalCount", totalCount);
        respObj.put("fullPath", fullPath);
    }
    else
    {
        respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
        respObj.put("errorText", "ERRORRRRRRR");
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::getProperties(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string path = DEFAULT_INTERNAL_PATH;
    std::unique_ptr<InternalSpaceInfo> propPtr = InternalOperationHandler::getInstance().getProperties(path);
    bool status = (propPtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (status)
    {
        if (reqData->params.hasKey("storageType"))
            respObj.put("storageType", reqData->params["storageType"].asString());
        respObj.put("writable", propPtr->getIsWritable());
        respObj.put("deletable", propPtr->getIsDeletable());
        respObj.put("totalSpace(MB)", int(propPtr->getCapacityMB()));
        respObj.put("freeSpace(MB)", int(propPtr->getFreeSpaceMB()));
    }
    else
    {
        respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
        respObj.put("errorText", "ERRORRRRRRR");
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::copy(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<InternalCopy> copyPtr = InternalOperationHandler::getInstance().copy(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -2;
    while(1)
    {
        retStatus = copyPtr->getStatus();
        LOG_DEBUG_SAF("%s: statussss : %d", __FUNCTION__, retStatus);
        bool status = (retStatus < 0)?(false):(true);
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("status(%)", retStatus);
        }
        else
        {
            respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
            respObj.put("errorText", "ERRORRRRRRR");
        }
        if (retStatus != prevStatus)
            reqData->cb(respObj, reqData->subs);
        if ((retStatus == 100) || (retStatus == -1))
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        prevStatus = retStatus;
    }
}

void InternalStorageProvider::move(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<InternalMove> movePtr = InternalOperationHandler::getInstance().move(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -2;
    while(1)
    {
        retStatus = movePtr->getStatus();
        LOG_DEBUG_SAF("%s: statussss : %d", __FUNCTION__, retStatus);
        bool status = (retStatus < 0)?(false):(true);
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("status(%)", retStatus);
        }
        else
        {
            respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
            respObj.put("errorText", "ERRORRRRRRR");
        }
        if (retStatus != prevStatus)
            reqData->cb(respObj, reqData->subs);
        if ((retStatus == 100) || (retStatus == -1))
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        prevStatus = retStatus;
    }
}

void InternalStorageProvider::remove(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string path = reqData->params["path"].asString();
    std::unique_ptr<InternalRemove> remPtr = InternalOperationHandler::getInstance().remove(path);
    bool status = (remPtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
        respObj.put("errorText", "ERRORRRRRRR");
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::rename(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["path"].asString();
    std::string destPath = reqData->params["newName"].asString();
    std::unique_ptr<InternalRename> renamePtr = InternalOperationHandler::getInstance().rename(srcPath, destPath);
    bool status = (renamePtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
        respObj.put("errorText", "ERRORRRRRRR");
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::eject(std::shared_ptr<RequestData> reqData)
{
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", false);
    respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
    respObj.put("errorText", "ERRORRRRRRR");
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::format(std::shared_ptr<RequestData> reqData)
{
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", false);
    respObj.put("errorCode", SAFErrors::UNKNOWN_ERROR);
    respObj.put("errorText", "ERRORRRRRRR");
    reqData->cb(respObj, reqData->subs);
}

ReturnValue InternalStorageProvider::getProperties(AuthParam authParam) {
    return nullptr;
}

ReturnValue InternalStorageProvider::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath, AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::remove(AuthParam authParam, string storageId, string path)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::eject(string storageId)
{
    return nullptr;
}

ReturnValue InternalStorageProvider::format(string storageId, string fileSystem, string volumeLabel)
{
    return nullptr;
}

void InternalStorageProvider::testMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    std::string uri = "luna://com.webos.service.pdm/getAttachedStorageDeviceList";
    std::string payload = R"({"subscribe": true})";
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    pbnjson::JValue nextReqArray = pbnjson::Array();
#if 0
    pbnjson::JValue nextObj1 = pbnjson::Object();
    nextObj1.put("uri", uri);
    nextObj1.put("payload", payload);
    //nextObj1.put("params", payload);
    nextReqArray.append(nextObj1);
#endif
    pbnjson::JValue nextObj2 = pbnjson::Object();
    std::string uri2 = "luna://com.webos.service.pdm/isWritableDrive";
    std::string payload2 = R"({"driveName": "sdg1"})";
    nextObj2.put("uri", uri2);
    nextObj2.put("payload", payload2);
    //nextObj2.put("params", payload);
    nextReqArray.append(nextObj2);

    ctxPtr->reqData->params.put("nextReq", nextReqArray);
    LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                InternalStorageProvider::onReply, ctxPtr, NULL, &lserror);
    //ToDo: Handle Error Scenarios
}

void InternalStorageProvider::addRequest(std::shared_ptr<RequestData>& reqData)
{
    mQueue.push_back(std::move(reqData));
    mCondVar.notify_one();
}

bool InternalStorageProvider::onReply(LSHandle *sh, LSMessage *message , void *ctx)
{
    LOG_DEBUG_SAF("%s: [%s]", __FUNCTION__, LSMessageGetPayload(message));
    LSError lserror;
    (void)LSErrorInit(&lserror);
    LSCallCancel(sh, NULL, &lserror);
    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    pbnjson::JSchema parseSchema = pbnjson::JSchema::AllSchema();
    pbnjson::JDomParser parser;
    std::string payload = LSMessageGetPayload(message);
    if (parser.parse(payload, parseSchema))
    {
        pbnjson::JValue root = parser.getDom();
        if (!ctxPtr->reqData->params.hasKey("response"))
        {
            pbnjson::JValue respArray = pbnjson::Array();
            ctxPtr->reqData->params.put("response", respArray);
        }
        pbnjson::JValue respArray = ctxPtr->reqData->params["response"];
        respArray.append(root);
        ctxPtr->reqData->params.put("response", respArray);
        if (ctxPtr->reqData->params.hasKey("nextReq") &&
            ctxPtr->reqData->params["nextReq"].isArray() &&
            (ctxPtr->reqData->params["nextReq"].arraySize() > 0))
        {
            std::string uri = ctxPtr->reqData->params["nextReq"][0]["uri"].asString();
            std::string payload = ctxPtr->reqData->params["nextReq"][0]["payload"].asString();
            pbnjson::JValue nextReqArr = pbnjson::Array();
            for (int i=1; i< ctxPtr->reqData->params["nextReq"].arraySize(); ++i)
                nextReqArr.append(ctxPtr->reqData->params["nextReq"][i]);
            ctxPtr->reqData->params.put("nextReq", nextReqArr);
            LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                InternalStorageProvider::onReply, ctxPtr, NULL, &lserror);
        }
        else
            ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
    }
    return true;
}

void InternalStorageProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    switch(reqData->methodType)
    {
        case MethodType::TEST_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::TEST_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->testMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::LIST_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->listFolderContents(reqData); });
            (void)fut;
        }
        break;
        case MethodType::GET_PROP_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::GET_PROP_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->getProperties(reqData); });
            (void)fut;
        }
        break;
        case MethodType::COPY_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::COPY_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->copy(reqData); });
            (void)fut;
        }
        break;
        case MethodType::MOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::MOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->move(reqData); });
            (void)fut;
        }
        break;
        case MethodType::REMOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::REMOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->remove(reqData); });
            (void)fut;
        }
        break;
        case MethodType::EJECT_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EJECT_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->eject(reqData); });
            (void)fut;
        }
        break;
        case MethodType::FORMAT_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::FORMAT_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->format(reqData); });
            (void)fut;
        }
        break;
        case MethodType::RENAME_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::RENAME_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->rename(reqData); });
            (void)fut;
        }
        break;
        default:
            LOG_DEBUG_SAF("%s : MethodType::UNKNOWN", __FUNCTION__);
        break;
    }
}

void InternalStorageProvider::dispatchHandler()
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::unique_lock < std::mutex > lock(mMutex);
    do {
        mCondVar.wait(lock, [this] {
            return (mQueue.size() || mQuit);
        });
        LOG_DEBUG_SAF("Dispatch notif received : %d, mQuit: %d", mQueue.size(), mQuit);
        if (mQueue.size() && !mQuit)
        {
            lock.unlock();
            handleRequests(std::move(mQueue.front()));
            mQueue.erase(mQueue.begin());
            lock.lock();
        }
    } while (!mQuit);
}

