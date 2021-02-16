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

void InternalStorageProvider::listFolderContents(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
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
            contentObj.put("name", contVec[index]->getName());
            contentObj.put("path", contVec[index]->getPath());
            contentObj.put("type", contVec[index]->getType());
            contentObj.put("size", int(contVec[index]->getSize()));
            contenResArr.append(contentObj);
        }
        status = true;
    }
    respObj.put("returnValue", status);
    if (status)
    {
        respObj.put("files", contenResArr);
        respObj.put("totalCount", totalCount);
        respObj.put("fullPath", fullPath);
    }
    else
    {
        auto errorCode = getInternalErrorCode(contsPtr->getStatus());
        auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::getProperties(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string path;
    if (reqData->params.hasKey("path"))
        path = reqData->params["path"].asString();
    if (path.empty())
        path = DEFAULT_INTERNAL_PATH;
    auto pos = path.find(DEFAULT_INTERNAL_PATH);
    if ((pos == std::string::npos) || (pos != 0))
    {
        auto errorCode = SAFErrors::SAFErrors::INVALID_PATH;
        auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
        return;
    }
    std::unique_ptr<InternalSpaceInfo> propPtr = InternalOperationHandler::getInstance().getProperties(path);
    bool status = (propPtr->getStatus() < 0)?(false):(true);
    respObj.put("returnValue", status);
    if (status)
    {
        pbnjson::JValue attributesArr = pbnjson::Array();
        if (reqData->params.hasKey("storageType"))
            respObj.put("storageType", reqData->params["storageType"].asString());
        respObj.put("writable", propPtr->getIsWritable());
        respObj.put("deletable", propPtr->getIsDeletable());
        pbnjson::JValue attrObj = pbnjson::Object();
        attrObj.put("LastModTimeStamp", propPtr->getLastModTime());
        attributesArr.append(attrObj);
        respObj.put("attributes", attributesArr);
        if (path == DEFAULT_INTERNAL_PATH)
        {
            respObj.put("totalSpace", int(propPtr->getCapacityMB()));
            respObj.put("freeSpace", int(propPtr->getFreeSpaceMB()));
        }
    }
    else
    {
        auto errorCode = getInternalErrorCode(propPtr->getStatus());
        auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::copy(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["srcDriveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<InternalCopy> copyPtr = InternalOperationHandler::getInstance().copy(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -20;
    while(1)
    {
        retStatus = copyPtr->getStatus();
        LOG_DEBUG_SAF("%s: statussss : %d", __FUNCTION__, retStatus);
        bool status = (retStatus < 0)?(false):(true);
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("progress", retStatus);
        }
        else
        {
            auto errorCode = getInternalErrorCode(copyPtr->getStatus());
            auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
            respObj.put("errorCode", errorCode);
            respObj.put("errorText", errorStr);
        }
        if (retStatus != prevStatus)
            reqData->cb(respObj, reqData->subs);
        if ((retStatus >= 100) || (retStatus < 0))
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        prevStatus = retStatus;
    }
}

void InternalStorageProvider::move(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["srcDriveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<InternalMove> movePtr = InternalOperationHandler::getInstance().move(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -20;
    while(1)
    {
        retStatus = movePtr->getStatus();
        LOG_DEBUG_SAF("%s: statussss : %d", __FUNCTION__, retStatus);
        bool status = (retStatus < 0)?(false):(true);
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("progress", retStatus);
        }
        else
        {
            auto errorCode = getInternalErrorCode(movePtr->getStatus());
            auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
            respObj.put("errorCode", errorCode);
            respObj.put("errorText", errorStr);
        }
        if (retStatus != prevStatus)
            reqData->cb(respObj, reqData->subs);
        if ((retStatus >= 100) || (retStatus < 0))
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        prevStatus = retStatus;
    }
}

void InternalStorageProvider::remove(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string path = reqData->params["path"].asString();
    std::unique_ptr<InternalRemove> remPtr = InternalOperationHandler::getInstance().remove(path);
    bool status = (remPtr->getStatus() < 0)?(false):(true);
    respObj.put("returnValue", status);
    if (!status)
    {
        auto errorCode = getInternalErrorCode(remPtr->getStatus());
        auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::rename(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(DEFAULT_INTERNAL_STORAGE_ID != driveId)
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string srcPath = reqData->params["path"].asString();
    std::string destPath = reqData->params["newName"].asString();
    std::unique_ptr<InternalRename> renamePtr = InternalOperationHandler::getInstance().rename(srcPath, destPath);
    bool status = (renamePtr->getStatus() < 0)?(false):(true);
    respObj.put("returnValue", status);
    if (!status)
    {
        auto errorCode = getInternalErrorCode(renamePtr->getStatus());
        auto errorStr = SAFErrors::InternalErrors::getInternalErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::eject(std::shared_ptr<RequestData> reqData)
{
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", false);
    respObj.put("errorCode", SAFErrors::SAFErrors::UNKNOWN_ERROR);
    respObj.put("errorText", SAFErrors::InternalErrors::getInternalErrorString(SAFErrors::SAFErrors::UNKNOWN_ERROR));
    reqData->cb(respObj, reqData->subs);
}

void InternalStorageProvider::listStoragesMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    pbnjson::JValue internalResArr = pbnjson::Array();
    pbnjson::JValue internalRes = pbnjson::Object();
    internalRes.put("storageName", "INTERNAL");
    internalRes.put("driveId", DEFAULT_INTERNAL_STORAGE_ID);
    internalRes.put("path", DEFAULT_INTERNAL_PATH);
    internalResArr.append(internalRes);
    respObj.put("INTERNAL", internalResArr);
    reqData->params.put("response", respObj);
    reqData->cb(reqData->params, std::move(reqData->subs));
    return;
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
        case MethodType::RENAME_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::RENAME_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->rename(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_STORAGES_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::LIST_STORAGES_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->listStoragesMethod(reqData); });
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

