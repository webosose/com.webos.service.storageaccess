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
#include "USBStorageProvider.h"
#include "SA_Common.h"
#include "SAFLunaService.h"
#include "USBOperationHandler.h"
#include "USBErrors.h"
#include "InternalOperationHandler.h"

#define SAF_USB_ATTACH_METHOD  "luna://com.webos.service.pdm/getAttachedStorageDeviceList"
#define SAF_USB_WRITE_Q_METHOD "luna://com.webos.service.pdm/isWritableDrive"
#define SAF_USB_SPACE_METHOD   "luna://com.webos.service.pdm/getSpaceInfo"
#define SAF_USB_FORMAT_METHOD  "luna://com.webos.service.pdm/format"
#define SAF_USB_EJECT_METHOD   "luna://com.webos.service.pdm/eject"

string USBStorageProvider::storageId = "";
string USBStorageProvider::driveName = "";

USBStorageProvider::USBStorageProvider() : mQuit(false)
{
    mDispatcherThread = std::thread(std::bind(&USBStorageProvider::dispatchHandler, this));
    mDispatcherThread.detach();
}

USBStorageProvider::~USBStorageProvider()
{
    mQuit = true;
    if (mDispatcherThread.joinable())
    {
        mDispatcherThread.join();
    }
}

void USBStorageProvider::getPropertiesMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    USBStorageProvider::storageId = data->params["storageId"].asString();
    LOG_DEBUG_SAF("USB STORAGE ID: %s", USBStorageProvider::storageId.c_str());
    std::string uri = SAF_USB_ATTACH_METHOD;
    std::string payload = R"({"subscribe": true})";
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    pbnjson::JValue nextReqArray = pbnjson::Array();
    pbnjson::JValue nextObj = pbnjson::Object();

    //append write info luna call
    std::string uri1 = SAF_USB_WRITE_Q_METHOD;
    std::string payload1 = R"({"driveName": ""})";
    nextObj.put("uri", uri1);
    nextObj.put("payload", payload1);
    nextObj.put("parentReq", uri);
    nextReqArray.append(nextObj);

    //append space related luna call
    pbnjson::JValue spaceObj = pbnjson::Object();
    std::string spaceUri = SAF_USB_SPACE_METHOD;
    std::string spacePayload = R"({"driveName": ""})";
    spaceObj.put("uri", spaceUri);
    spaceObj.put("payload", spacePayload);
    spaceObj.put("parentReq", uri);
    nextReqArray.append(spaceObj);

    ctxPtr->reqData->params.put("nextReq", nextReqArray);
    LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),USBStorageProvider::onGetPropertiesReply, ctxPtr, NULL, &lserror);
    //ToDo: Handle Error Scenarios
}

void USBStorageProvider::listStoragesMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    std::string uri = SAF_USB_ATTACH_METHOD;
    std::string payload = R"({"subscribe": true})";
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    pbnjson::JValue nextReqArray = pbnjson::Array();
    pbnjson::JValue nextObj = pbnjson::Object();

    nextObj.put("uri", uri);
    nextObj.put("payload", payload);
    nextReqArray.append(nextObj);

    LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                USBStorageProvider::onListStoragesMethodReply, ctxPtr, NULL, &lserror);
}


bool USBStorageProvider::onListStoragesMethodReply(LSHandle *sh, LSMessage *message , void *ctx)
{
    LOG_DEBUG_SAF("%s: [%s]", __FUNCTION__, LSMessageGetPayload(message));
    LSError lserror;
    (void)LSErrorInit(&lserror);
    LSCallCancel(sh, NULL, &lserror);
    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);
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
            LOG_DEBUG_SAF("======================Call:%s=========Payload:%s", uri.c_str(), payload.c_str());
            LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
        }
        else
            ctxPtr->reqData->cb(ctxPtr->reqData->params, std::move(ctxPtr->reqData->subs));
    }
    return true;
}
void USBStorageProvider::ejectMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    int storageNumber = 0;
    data->params["storageNumber"].asNumber<int>(storageNumber);

    std::string uri = SAF_USB_EJECT_METHOD;
    std::string payload = "{\"deviceNum\": " + std::to_string(storageNumber) + "}";
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    pbnjson::JValue nextReqArray = pbnjson::Array();
    pbnjson::JValue nextObj = pbnjson::Object();

    nextObj.put("uri", uri);
    nextObj.put("payload", payload);
    nextReqArray.append(nextObj);
    LOG_DEBUG_SAF("LS Call:%s and Payload:%s", uri.c_str(), payload.c_str());
    LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
}

void USBStorageProvider::formatMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    std::string uri = SAF_USB_FORMAT_METHOD;
    std::string payload = "{\"driveName\": \"" + data->params["storageName"].asString() + "\"}";
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    pbnjson::JValue nextReqArray = pbnjson::Array();
    pbnjson::JValue nextObj = pbnjson::Object();

    nextObj.put("uri", uri);
    nextObj.put("payload", payload);
    nextReqArray.append(nextObj);
    LOG_DEBUG_SAF("LS Call:%s and Payload:%s", uri.c_str(), payload.c_str());
    LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
}

void USBStorageProvider::copyMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
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
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("status(%)", retStatus);
        }
        else
        {
            respObj.put("errorCode", USBErrors::USB_COPY_FAILED);
            respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_COPY_FAILED));
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

void USBStorageProvider::moveMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<USBMove> movePtr = USBOperationHandler::getInstance().move(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -20;
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
            respObj.put("errorCode", USBErrors::USB_MOVE_FAILED);
            respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_MOVE_FAILED));
        }
        if (retStatus != prevStatus)
            reqData->cb(respObj, reqData->subs);
        if ((retStatus >= 100) || (retStatus == -1))
            break;
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        prevStatus = retStatus;
    }
}

void USBStorageProvider::removeMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string path = reqData->params["path"].asString();
    std::unique_ptr<USBRemove> remPtr = USBOperationHandler::getInstance().remove(path);
    bool status = (remPtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        respObj.put("errorCode", USBErrors::USB_REMOVE_FAILED);
        respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_REMOVE_FAILED));
    }
    reqData->cb(respObj, reqData->subs);
}

void USBStorageProvider::listFolderContentsMethod(std::shared_ptr<RequestData> reqData)
{
    std::string path = reqData->params["path"].asString();
    int offset = reqData->params["offset"].asNumber<int>();
    int limit = reqData->params["limit"].asNumber<int>();

    bool status = false;
    int totalCount = 0;
    std::string fullPath;
    pbnjson::JValue contenResArr = pbnjson::Array();
    std::shared_ptr<USBFolderContents> contsPtr = USBOperationHandler::getInstance().getListFolderContents(path);
    fullPath = contsPtr->getPath();
    totalCount = contsPtr->getTotalCount();
    if (contsPtr->getStatus() >= 0)
    {
        auto contVec = contsPtr->getContents();
        int start = (offset > contVec.size())?(contVec.size() + 1):(offset - 1);
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
        respObj.put("errorCode", USBErrors::USB_LIST_CONTENTS_FAILED);
        respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_LIST_CONTENTS_FAILED));
    }
    reqData->cb(respObj, reqData->subs);
}

void USBStorageProvider::renameMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["path"].asString();
    std::string destPath = reqData->params["newName"].asString();
    std::unique_ptr<USBRename> renamePtr = USBOperationHandler::getInstance().rename(srcPath, destPath);
    bool status = (renamePtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        respObj.put("errorCode", USBErrors::USB_RENAME_FAILED);
        respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_RENAME_FAILED));
    }
    reqData->cb(respObj, reqData->subs);
}

void USBStorageProvider::addRequest(std::shared_ptr<RequestData>& reqData)
{
    mQueue.push_back(std::move(reqData));
    mCondVar.notify_one();
}

bool USBStorageProvider::onReply(LSHandle *sh, LSMessage *message , void *ctx)
{
    LOG_DEBUG_SAF("%s: [%s]", __FUNCTION__, LSMessageGetPayload(message));
    LSError lserror;
    (void)LSErrorInit(&lserror);
    LSCallCancel(sh, NULL, &lserror);
    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);
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
            LOG_DEBUG_SAF("======================Call:%s=========Payload:%s", uri.c_str(), payload.c_str());
            LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),
                USBStorageProvider::onReply, ctxPtr, NULL, &lserror);
        }
        else
            ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
    }
    return true;
}

bool USBStorageProvider::onGetPropertiesReply(LSHandle *sh, LSMessage *message , void *ctx)
{
    LOG_DEBUG_SAF("%s: [%s]", __FUNCTION__, LSMessageGetPayload(message));
    LSError lserror;
    (void)LSErrorInit(&lserror);
    LSCallCancel(sh, NULL, &lserror);

    std::string actualDevId = USBStorageProvider::storageId;
    std::string mainId = "";
    std::string subId  = "";
    int subIndex = actualDevId.find("-");
    if(subIndex != std::string::npos)
    {
        mainId = actualDevId.substr(0,subIndex);
        subId  = actualDevId.substr(subIndex + 1, actualDevId.length() - 1);
    }
    else
    {
        mainId = actualDevId;
    }

    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);
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
            std::string parentReq;
            if (ctxPtr->reqData->params["nextReq"][0].hasKey("parentReq"))
            {
                parentReq = ctxPtr->reqData->params["nextReq"][0]["parentReq"].asString();
            }
            pbnjson::JValue nextReqArr = pbnjson::Array();
            for (int i=1; i< ctxPtr->reqData->params["nextReq"].arraySize(); ++i)
                nextReqArr.append(ctxPtr->reqData->params["nextReq"][i]);

            if (!parentReq.empty() && (parentReq.find("getAttachedStorageDeviceList") != std::string::npos))
            {
                std::string devPayload;
                if(root.hasKey("deviceListInfo")) {
                pbnjson::JValue storageDeviceList = root["deviceListInfo"][0]["storageDeviceList"];
                for (int i=0; i<storageDeviceList.arraySize(); ++i)
                {
                    if(storageDeviceList[i]["serialNumber"].asString().compare(mainId) == 0)
                    {
                        if(storageDeviceList[i]["storageDriveList"].arraySize() > 1)
                        {
                            LOG_DEBUG_SAF("Num of StorageDrives:%d", storageDeviceList[i]["storageDriveList"].arraySize());
                            if(actualDevId.find("-") == std::string::npos)
                            {
                                pbnjson::JValue errObj = pbnjson::Object();
                                errObj.put("returnValue", false);
                                errObj.put("errorCode", USBErrors::MORE_PARTITIONS_IN_USB);
                                errObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::MORE_PARTITIONS_IN_USB));
                                ctxPtr->reqData->params.put("response", errObj);
                                ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
                                return true;
                           }
                           for(int j = 0; j < storageDeviceList[i]["storageDriveList"].arraySize(); j++)
                           {
                                if(storageDeviceList[i]["storageDriveList"][j]["uuid"].asString().compare(subId) == 0)
                                {
                                    USBStorageProvider::driveName = storageDeviceList[i]["storageDriveList"][j]["driveName"].asString();
                                    devPayload = "{\"driveName\": \"" + USBStorageProvider::driveName + "\"}";
                                    break;
                                }
                           }
                        }
                        else if(storageDeviceList[i]["storageDriveList"].arraySize() == 1)
                        {
                            USBStorageProvider::driveName = storageDeviceList[i]["storageDriveList"][0]["driveName"].asString();
                            devPayload = "{\"driveName\": \"" + USBStorageProvider::driveName + "\"}";
                        }
                    }
                }
                }
                if(root.hasKey("isWritable")) {
                    devPayload = "{\"driveName\": \"" + USBStorageProvider::driveName + "\"}";
                }
                payload = devPayload;
                if(payload.empty())
                {
                    pbnjson::JValue errObj = pbnjson::Object();
                    errObj.put("returnValue", false);
                    errObj.put("errorCode", USBErrors::USB_STORAGE_NOT_EXISTS);
                    errObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::USB_STORAGE_NOT_EXISTS));
                    ctxPtr->reqData->params.put("response", errObj);
                    ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
                    return true;
                }
            }
            ctxPtr->reqData->params.put("nextReq", nextReqArr);
            LOG_DEBUG_SAF("LS Call: %s,  Payload:%s", uri.c_str(), payload.c_str());
            LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),USBStorageProvider::onGetPropertiesReply, ctxPtr, NULL, &lserror);
        }
        else
            ctxPtr->reqData->cb(ctxPtr->reqData->params["response"], ctxPtr->reqData->subs);
    }
    return true;
}



void USBStorageProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    switch(reqData->methodType)
    {
        case MethodType::LIST_STORAGES_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::LIST_STORAGES_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->listStoragesMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::GET_PROP_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::GET_PROPERTIES_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->getPropertiesMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::EJECT_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EJECT_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->ejectMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::COPY_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::COPY_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->copyMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::MOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::MOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->moveMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::REMOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::REMOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->removeMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::RENAME_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::RENAME_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->renameMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::LIST_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->listFolderContentsMethod(reqData); });
            (void)fut;
        }
        break;
        default:
        break;
    }
}

void USBStorageProvider::dispatchHandler()
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
