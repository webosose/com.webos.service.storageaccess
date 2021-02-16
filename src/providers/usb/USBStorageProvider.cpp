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
#include "SAFErrors.h"
#include "USBJsonParser.h"

#define SAF_USB_ATTACH_METHOD  "luna://com.webos.service.pdm/getAttachedStorageDeviceList"
#define SAF_USB_WRITE_Q_METHOD "luna://com.webos.service.pdm/isWritableDrive"
#define SAF_USB_SPACE_METHOD   "luna://com.webos.service.pdm/getSpaceInfo"
#define SAF_USB_FORMAT_METHOD  "luna://com.webos.service.pdm/format"
#define SAF_USB_EJECT_METHOD   "luna://com.webos.service.pdm/eject"

USBStorageProvider::USBStorageProvider() : mQuit(false)
{
    mDispatcherThread = std::thread(std::bind(&USBStorageProvider::dispatchHandler, this));
    mDispatcherThread.detach();
    deviceInfo = make_shared<USBAttached>();
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
    LSError lserror;
    (void)LSErrorInit(&lserror);
    ReqContext *ctxPtr = new ReqContext();
    ctxPtr->ctx = this;
    ctxPtr->reqData = std::move(data);

    if(isStorageIdExists(ctxPtr->reqData->params["driveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        ctxPtr->reqData->cb(respObj, ctxPtr->reqData->subs);
        return;
    }

    if(!isStorageDriveMounted(ctxPtr->reqData->params["driveId"].asString()))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        ctxPtr->reqData->cb(respObj, ctxPtr->reqData->subs);
        return;
    }

    std::string devDriveName = getDriveName(ctxPtr->reqData->params["driveId"].asString());
    if(devDriveName.compare("UUID") == 0)
    {
        pbnjson::JValue errObj = pbnjson::Object();
        errObj.put("returnValue", false);
        errObj.put("errorCode", SAFErrors::USBErrors::MORE_PARTITIONS_IN_USB);
        errObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::MORE_PARTITIONS_IN_USB));
        ctxPtr->reqData->cb(errObj, ctxPtr->reqData->subs);
        return;
    }
    else if(devDriveName.compare("NO SUB") == 0)
    {
        pbnjson::JValue errObj = pbnjson::Object();
        errObj.put("returnValue", false);
        errObj.put("errorCode", SAFErrors::USBErrors::USB_SUB_STORAGE_NOT_EXISTS);
        errObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_SUB_STORAGE_NOT_EXISTS));
        ctxPtr->reqData->cb(errObj, ctxPtr->reqData->subs);
        return;
    }

    pbnjson::JValue nextReqArray = pbnjson::Array();

    if(ctxPtr->reqData->params.hasKey("path"))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        pbnjson::JValue attributesArr = pbnjson::Array();
        pbnjson::JValue attrObj = pbnjson::Object();

        std::unique_ptr<USBSpaceInfo> propPtr = USBOperationHandler::getInstance().getProperties(ctxPtr->reqData->params["path"].asString());
        bool status = (propPtr->getStatus() < 0)?(false):(true);
        respObj.put("returnValue", status);
        if (status)
        {
            if (ctxPtr->reqData->params.hasKey("storageType"))
                respObj.put("storageType", ctxPtr->reqData->params["storageType"].asString());

            respObj.put("writable", propPtr->getIsWritable());
            respObj.put("deletable", propPtr->getIsDeletable());
            attrObj.put("LastModTimeStamp", propPtr->getLastModTime());
            attributesArr.append(attrObj);
            respObj.put("attributes", attributesArr);
        }
        else
        {
            auto errorCode = getUSBErrorCode(propPtr->getStatus());
            auto errorStr = SAFErrors::USBErrors::getUSBErrorString(errorCode);
            respObj.put("errorCode", errorCode);
            respObj.put("errorText", errorStr);
        }
        ctxPtr->reqData->cb(respObj, ctxPtr->reqData->subs);
        return;
    }
    else
    {
        pbnjson::JValue nextObj = pbnjson::Object();
        std::string uri = SAF_USB_WRITE_Q_METHOD;
        std::string payload = "{\"driveName\": \"" + devDriveName + "\"}";
        nextObj.put("uri", uri);
        nextObj.put("payload", payload);

        pbnjson::JValue spaceObj = pbnjson::Object();
        std::string spaceUri = SAF_USB_SPACE_METHOD;
        std::string spacePayload = "{\"driveName\": \"" + devDriveName + "\"}";
        spaceObj.put("uri", spaceUri);
        spaceObj.put("payload", spacePayload);
        nextReqArray.append(spaceObj);

        ctxPtr->reqData->params.put("nextReq", nextReqArray);
        LSCall(SAFLunaService::lsHandle, uri.c_str(), payload.c_str(),USBStorageProvider::onGetPropertiesReply, ctxPtr, NULL, &lserror);
    }
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
    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);
    pbnjson::JSchema parseSchema = pbnjson::JSchema::AllSchema();
    pbnjson::JDomParser parser;
    std::string payload = LSMessageGetPayload(message);
    if (parser.parse(payload, parseSchema))
    {
        pbnjson::JValue root = parser.getDom();
        self->cleanDeviceInfo();
        self->populateDeviceInfo(root);
        USBPbnJsonParser usbParser;
        pbnjson::JValue responseObj = usbParser.ParseListOfStorages(root);
        ctxPtr->reqData->params.put("response", responseObj);
        ctxPtr->reqData->params.put("storageType","USB");
        ctxPtr->reqData->cb(ctxPtr->reqData->params, ctxPtr->reqData->subs);
    }
    return true;
}

void USBStorageProvider::ejectMethod(std::shared_ptr<RequestData> data)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    std::string storageid = data->params["driveId"].asString();
    if(isStorageIdExists(storageid) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        data->cb(respObj, data->subs);
        return;
    }

    if(!isStorageDriveMounted(data->params["driveId"].asString()))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_DRIVE_ALREADY_EJECTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_DRIVE_ALREADY_EJECTED));
        data->cb(respObj, data->subs);
        return;
    }

    std::string driveId = data->params["driveId"].asString();
    std::string uri = SAF_USB_EJECT_METHOD;
    std::string payload = "{\"deviceNum\": " + std::to_string(getStorageNumber(driveId)) + "}";
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
    if(isStorageIdExists(reqData->params["srcDriveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if(reqData->params["destStorageType"] == "USB")
    {
        if(isStorageIdExists(reqData->params["destDriveId"].asString()) == false)
        {
            pbnjson::JValue respObj = pbnjson::Object();
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
            respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
            reqData->cb(respObj, reqData->subs);
            return;
        }
    }

    if(!isStorageDriveMounted(reqData->params["srcDriveId"].asString()) ||
       (reqData->params["destStorageType"] == "USB" && !isStorageDriveMounted(reqData->params["destDriveId"].asString())))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    bool overwrite = false;
    if (reqData->params.hasKey("overwrite"))
        overwrite = reqData->params["overwrite"].asBool();

    std::unique_ptr<USBCopy> copyPtr = USBOperationHandler::getInstance().copy(srcPath, destPath, overwrite);

    int retStatus = -1;
    int prevStatus = -20;
    pbnjson::JValue respObj = pbnjson::Object();
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
            auto errorCode = getUSBErrorCode(copyPtr->getStatus());
            auto errorStr  = SAFErrors::USBErrors::getUSBErrorString(errorCode);
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

void USBStorageProvider::moveMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();

    if(isStorageIdExists(reqData->params["srcDriveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if(reqData->params["destStorageType"] == "USB")
    {
        if(isStorageIdExists(reqData->params["destDriveId"].asString()) == false)
        {
            pbnjson::JValue respObj = pbnjson::Object();
            respObj.put("returnValue", false);
            respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
            respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
            reqData->cb(respObj, reqData->subs);
            return;
        }
    }

    if(!isStorageDriveMounted(reqData->params["srcDriveId"].asString()) ||
       (reqData->params["destStorageType"] == "USB" && !isStorageDriveMounted(reqData->params["destDriveId"].asString())))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

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
            respObj.put("progress", retStatus);
        }
        else
        {
            auto errorCode = getUSBErrorCode(movePtr->getStatus());
            auto errorStr = SAFErrors::USBErrors::getUSBErrorString(errorCode);
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

void USBStorageProvider::removeMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string path = reqData->params["path"].asString();
    if(isStorageIdExists(reqData->params["driveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if(!isStorageDriveMounted(reqData->params["driveId"].asString()))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::unique_ptr<USBRemove> remPtr = USBOperationHandler::getInstance().remove(path);
    bool status = (remPtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        auto errorCode = getUSBErrorCode(remPtr->getStatus());
        auto errorStr  = SAFErrors::USBErrors::getUSBErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void USBStorageProvider::listFolderContentsMethod(std::shared_ptr<RequestData> reqData)
{
    std::string path = reqData->params["path"].asString();
    int offset = reqData->params["offset"].asNumber<int>();
    int limit = reqData->params["limit"].asNumber<int>();

    if(isStorageIdExists(reqData->params["driveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if(!isStorageDriveMounted(reqData->params["driveId"].asString()))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

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
            contentObj.put("name", contVec[index]->getName());
            contentObj.put("path", contVec[index]->getPath());
            contentObj.put("type", contVec[index]->getType());
            contentObj.put("size", int(contVec[index]->getSize()));
            contenResArr.append(contentObj);
        }
        status = true;
    }
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (status)
    {
        respObj.put("files", contenResArr);
        respObj.put("totalCount", totalCount);
        respObj.put("fullPath", fullPath);
    }
    else
    {
        auto errorCode = getUSBErrorCode(contsPtr->getStatus());
        auto errorStr  = SAFErrors::USBErrors::getUSBErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
    }
    reqData->cb(respObj, reqData->subs);
}

void USBStorageProvider::renameMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string srcPath = reqData->params["path"].asString();
    std::string destPath = reqData->params["newName"].asString();

    if(isStorageIdExists(reqData->params["driveId"].asString()) == false)
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::USB_STORAGE_NOT_EXISTS));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    if(!isStorageDriveMounted(reqData->params["driveId"].asString()))
    {
        pbnjson::JValue respObj = pbnjson::Object();
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::USBErrors::DRIVE_NOT_MOUNTED);
        respObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::DRIVE_NOT_MOUNTED));
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::unique_ptr<USBRename> renamePtr = USBOperationHandler::getInstance().rename(srcPath, destPath);
    bool status = (renamePtr->getStatus() < 0)?(false):(true);
    pbnjson::JValue respObj = pbnjson::Object();
    respObj.put("returnValue", status);
    if (!status)
    {
        auto errorCode = getUSBErrorCode(renamePtr->getStatus());
        auto errorStr  = SAFErrors::USBErrors::getUSBErrorString(errorCode);
        respObj.put("errorCode", errorCode);
        respObj.put("errorText", errorStr);
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
    ReqContext *ctxPtr = static_cast<ReqContext*>(ctx);
    USBStorageProvider* self = static_cast<USBStorageProvider*>(ctxPtr->ctx);

    std::string deviceType = self->getStorageType(ctxPtr->reqData->params["driveId"].asString());

    pbnjson::JSchema parseSchema = pbnjson::JSchema::AllSchema();
    pbnjson::JDomParser parser;
    pbnjson::JValue typeObj = pbnjson::Object();
    typeObj.put("storageType", deviceType);

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
        respArray.append(typeObj);
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

void USBStorageProvider::populateDeviceInfo(pbnjson::JValue pbnObj)
{
    pbnjson::JValue infoObj = pbnjson::Array();

    if(pbnObj.hasKey("deviceListInfo"))
    {
        if(pbnObj["deviceListInfo"].isArray() && pbnObj["deviceListInfo"].arraySize() == 1)
        {
            if(pbnObj["deviceListInfo"][0].hasKey("storageDeviceList"))
            {
                if(pbnObj["deviceListInfo"][0]["storageDeviceList"].isArray())
                    infoObj = pbnObj["deviceListInfo"][0]["storageDeviceList"];
            }
        }
    }

    if(pbnObj.hasKey("storageDeviceList"))
    {
        if(pbnObj["storageDeviceList"].isArray())
        {
            infoObj = pbnObj["storageDeviceList"];
        }
    }

    for(int i = 0; i < infoObj.arraySize(); i++)
    {
        shared_ptr<USBDeviceInfo> devPtr = make_shared<USBDeviceInfo>();
        if(infoObj[i].hasKey("deviceType"))
            devPtr->mStorageType = infoObj[i]["deviceType"].asString();
        if(infoObj[i].hasKey("deviceNum"))
            infoObj[i]["deviceNum"].asNumber<int>(devPtr->mDeviceNumber);
        if(infoObj[i].hasKey("deviceSetId"))
            devPtr->mDeviceSetId = infoObj[i]["deviceSetId"].asString();

        if(infoObj[i].hasKey("storageDriveList"))
        {
            for(int j = 0; j < infoObj[i]["storageDriveList"].arraySize(); j++)
            {
                shared_ptr<USBDriveInfo> drivePtr = make_shared<USBDriveInfo>();
                if(infoObj[i]["storageDriveList"][j].hasKey("driveName"))
                    drivePtr->mDriveName = infoObj[i]["storageDriveList"][j]["driveName"].asString();
                if(infoObj[i]["storageDriveList"][j].hasKey("uuid"))
                    drivePtr->mUuid = infoObj[i]["storageDriveList"][j]["uuid"].asString();
                if(infoObj[i]["storageDriveList"][j].hasKey("fsType"))
                    drivePtr->mFsType = infoObj[i]["storageDriveList"][j]["fsType"].asString();
                if(infoObj[i]["storageDriveList"][j].hasKey("mountName"))
                    drivePtr->mMountPath = infoObj[i]["storageDriveList"][j]["mountName"].asString();
                if(infoObj[i]["storageDriveList"][j].hasKey("volumeLabel"))
                    drivePtr->mVolumeLabel = infoObj[i]["storageDriveList"][j]["volumeLabel"].asString();
                if(infoObj[i]["storageDriveList"][j].hasKey("isMounted"))
                    drivePtr->mIsMounted = infoObj[i]["storageDriveList"][j]["isMounted"].asBool();
                devPtr->mStorageDriveList.push_back(drivePtr);
            }
        }
        if(infoObj[i].hasKey("serialNumber"))
            deviceInfo->usbStorages[infoObj[i]["serialNumber"].asString()] = devPtr;
    }
}

void USBStorageProvider::printUSBInfo()
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        LOG_DEBUG_SAF("USB Serial %s::StorageType:%s StorageNumber:%d SetId:%s",
            (it->first).c_str(), (it->second->mStorageType).c_str(),
            it->second->mDeviceNumber, (it->second->mDeviceSetId).c_str());
        LOG_DEBUG_SAF("Drive Details:");
        for(int k = 0; k < it->second->mStorageDriveList.size(); k++)
        {
            LOG_DEBUG_SAF("DriveName:%s UUID:%s FsType:%s MountPath:%s Mounted:%d VolumeLabel:%s",
                (it->second->mStorageDriveList[k]->mDriveName).c_str(),
                (it->second->mStorageDriveList[k]->mUuid).c_str(),
                (it->second->mStorageDriveList[k]->mFsType).c_str(),
                (it->second->mStorageDriveList[k]->mMountPath).c_str(),
                (it->second->mStorageDriveList[k]->mIsMounted),
                (it->second->mStorageDriveList[k]->mVolumeLabel).c_str());
        }
    }
}

std::string USBStorageProvider::getDriveName(std::string driveId)
{
    std::string mainId = "";
    std::string subId  = "";
    int subIndex = driveId.find("-");
    if(subIndex != std::string::npos)
    {
        mainId = driveId.substr(0,subIndex);
        subId  = driveId.substr(subIndex + 1, driveId.length() - 1);
    }
    else
    {
        mainId = driveId;
    }
    return getDriveName(mainId, subId);
}

std::string USBStorageProvider::getDriveName(std::string driveId, std::string subId)
{
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    std::string usbDriveName = "";
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        if((it->first).compare(driveId) == 0)
        {
            if(subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                usbDriveName = it->second->mStorageDriveList[0]->mDriveName;
                break;
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                if((it->second->mStorageDriveList[0]->mUuid).compare(subId) == 0)
                    usbDriveName = it->second->mStorageDriveList[0]->mDriveName;
                else
                    usbDriveName = "NO SUB";
                break;
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() > 1)
            {
                for(int k = 0; k < it->second->mStorageDriveList.size(); k++)
                {
                    if((it->second->mStorageDriveList[k]->mUuid).compare(subId) == 0)
                    {
                        usbDriveName = it->second->mStorageDriveList[k]->mDriveName;
                        break;
                    }
                }
                if(usbDriveName.empty())
                {
                    usbDriveName = "NO SUB";
                    break;
                }
            }
            else if(subId.empty() && it->second->mStorageDriveList.size() > 1)
            {
                usbDriveName = "UUID";
            }
        }
    }
    return usbDriveName;
}

bool USBStorageProvider::isStorageIdExists(std::string driveId)
{
    std::string mainId = "";
    std::string subId  = "";
    int subIndex = driveId.find("-");
    if(subIndex != std::string::npos)
    {
        mainId = driveId.substr(0,subIndex);
        subId  = driveId.substr(subIndex + 1, driveId.length() - 1);
    }
    else
    {
        mainId = driveId;
    }
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        if((it->first).compare(mainId) == 0)
        {
            if(subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                return true;
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                if((it->second->mStorageDriveList[0]->mUuid).compare(subId) == 0)
                {
                    return true;
                }
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() > 1)
            {
                for(int k = 0; k < it->second->mStorageDriveList.size(); k++)
                {
                    if((it->second->mStorageDriveList[k]->mUuid).compare(subId) == 0)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool USBStorageProvider::isStorageDriveMounted(std::string actualDevId)
{
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
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        if((it->first).compare(mainId) == 0)
        {
            if(subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                return it->second->mStorageDriveList[0]->mIsMounted;
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() == 1)
            {
                if((it->second->mStorageDriveList[0]->mUuid).compare(subId) == 0)
                {
                    return it->second->mStorageDriveList[0]->mIsMounted;
                }
            }
            else if(!subId.empty() && it->second->mStorageDriveList.size() > 1)
            {
                for(int k = 0; k < it->second->mStorageDriveList.size(); k++)
                {
                    if((it->second->mStorageDriveList[k]->mUuid).compare(subId) == 0)
                    {
                        return it->second->mStorageDriveList[k]->mIsMounted;
                    }
                }
            }
        }
    }
    return false;
}

int USBStorageProvider::getStorageNumber(std::string actualDevId)
{
    std::string mainId = "";
    std::string subId  = "";
    int subIndex = actualDevId.find("-");
    if(subIndex != std::string::npos)
    {
        mainId = actualDevId.substr(0,subIndex);
    }
    else
    {
        mainId = actualDevId;
    }
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        if((it->first).compare(mainId) == 0)
        {
            return it->second->mDeviceNumber;
        }
    }
    return 0;
}

 std::string USBStorageProvider::getStorageType(std::string actualDevId)
 {
    std::string mainId = "";
    std::string subId  = "";
    int subIndex = actualDevId.find("-");
    if(subIndex != std::string::npos)
    {
        mainId = actualDevId.substr(0,subIndex);
    }
    else
    {
        mainId = actualDevId;
    }
    std::map<std::string, std::shared_ptr<USBDeviceInfo>>::iterator it;
    for (it = deviceInfo->usbStorages.begin(); it != deviceInfo->usbStorages.end(); ++it)
    {
        if((it->first).compare(mainId) == 0)
        {
            return it->second->mStorageType;
        }
    }
    return "";
 }

 void USBStorageProvider::cleanDeviceInfo()
 {
    deviceInfo.reset();
    deviceInfo = make_shared<USBAttached>();
 }

