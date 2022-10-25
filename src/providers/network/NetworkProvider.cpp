/* @@@LICENSE
 *
 * Copyright (c) 2020-2022 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include <algorithm>
#include <cctype>
#include <SAFLog.h>
#include "NetworkProvider.h"
#include <future>
#include <chrono>
#include <iomanip>
#include "gdrive/gdrive.hpp"
#include "SAFUtilityOperation.h"
#include "UpnpDiscover.h"
#include "UpnpOperation.h"


NetworkProvider::NetworkProvider() : mQuit(false)
{
    LOG_DEBUG_SAF(" NetworkProvider:: Constructor Created");
    mDispatcherThread = std::thread(std::bind(&NetworkProvider::dispatchHandler, this));
    mDispatcherThread.detach();
}

NetworkProvider::~NetworkProvider()
{
    mQuit = true;
    if (mDispatcherThread.joinable())
    {
        mDispatcherThread.join();
    }
}

void NetworkProvider::setErrorMessage(shared_ptr<ValuePairMap> valueMap, string errorText)
{
    valueMap->emplace("errorCode", pair<string, DataType>("-1", DataType::NUMBER));
    valueMap->emplace("errorText", pair<string, DataType>(errorText, DataType::STRING));
    valueMap->emplace("returnValue", pair<string, DataType>("false", DataType::BOOLEAN));
}

bool NetworkProvider::validateExtraCommand(std::vector<std::string> extraParams, std::shared_ptr<RequestData> reqData)
{
    pbnjson::JValue payload = reqData->params["operation"]["payload"];
    for (auto paramName : extraParams)
    {
        if (!payload.hasKey(paramName) || payload[paramName].asString().empty())
            return false;
    }
    return true;
}

std::string NetworkProvider::getTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream nowSs;
    nowSs << std::put_time(std::localtime(&nowAsTimeT), "%T") << "_" << nowMs.count();
    return nowSs.str();
}

void NetworkProvider::extraMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    pbnjson::JValue operation = reqData->params["operation"];
    std::string type = operation["type"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if ((type == "mountSambaServer") && validateExtraCommand({"ip","sharePath",}, reqData))
    {
        mountSambaServer(reqData);
    }
    else if ((type == "discoverUPnPMediaServer"))
    {
        discoverUPnPMediaServer(reqData);
    }
    else
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", SAFErrors::CloudErrors::getCloudErrorString(SAFErrors::INVALID_PARAM));
        reqData->cb(respObj, reqData->subs);
    }
}

bool NetworkProvider::validateSambaOperation(std::string driveId,
    std::string sessionId)
{
    if ((mSambaSessionData.find(driveId) != mSambaSessionData.end())
        && (mSambaSessionData[driveId] == sessionId))
        return true;
    return false;
}

bool NetworkProvider::validateUpnpOperation(std::string driveId,
    std::string sessionId)
{
    if ((mUpnpSessionData.find(driveId) != mUpnpSessionData.end())
        && (mUpnpSessionData[driveId] == sessionId))
        return true;
    return false;
}

std::string NetworkProvider::generateUniqueSambaDriveId()
{
    static unsigned int id = 0;
    return ("SAMBA_" + std::to_string(++id));
}

std::string NetworkProvider::generateUniqueUpnpDriveId()
{
    static unsigned int id = 0;
    return ("UPNP_" + std::to_string(++id));
}

void NetworkProvider::mountSambaServer(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
        pbnjson::JValue respObj = pbnjson::Object();
        std::string ip = reqData->params["operation"]["payload"]["ip"].asString();
        std::string userName;
        std::string password;

        if(reqData->params["operation"]["payload"].hasKey("userName"))
            userName = reqData->params["operation"]["payload"]["userName"].asString();
        else
        {
            respObj.put("returnValue", false);
            respObj.put("errorText", "User name not specified for Drive");
            reqData->cb(respObj, reqData->subs);
            return;
        }

        if(reqData->params["operation"]["payload"].hasKey("password"))
            password = reqData->params["operation"]["payload"]["password"].asString();
        else
        {
            respObj.put("returnValue", false);
            respObj.put("errorText", "Password not specified for Drive");
            reqData->cb(respObj, reqData->subs);
            return;
        }

        std::string sharePath = reqData->params["operation"]["payload"]["sharePath"].asString();
        auto gid = getgid();
        auto uid = getuid();
        std::string securityMode;

        if(reqData->params["operation"]["payload"].hasKey("securityMode"))
            securityMode = reqData->params["operation"]["payload"]["securityMode"].asString();
        else
        {
            respObj.put("returnValue", false);
            respObj.put("errorText", "Security Mode not specified for Drive");
            reqData->cb(respObj, reqData->subs);
            return;
        }

        std::string path = "/tmp/" + ip + "_" +  getTimestamp();
        std::string uniqueKey = ip + "_" + userName + "_" + sharePath;
        if (mSambaDriveMap.find(uniqueKey) != mSambaDriveMap.end())
        {
            respObj.put("returnValue", false);
            respObj.put("error", "Already Mounted ");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        FILE *fp;
        std::string pathCreate = "mkdir -p " + path;
        fp = popen(pathCreate.c_str(),"r");
        if(fp == NULL)
        {
            LOG_DEBUG_SAF("mountSambaServer::File Open Failed");
            respObj.put("returnValue", false);
            respObj.put("errorText", "Cannot create mount path  directory");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        else
          pclose(fp);
       std::string src  = "//" + ip + "/" + userName;
       const unsigned long mntflags = 0;
       const char* type = "cifs";
       if(mntpathmap.find(path) == mntpathmap.end()){
       std::string data = "ip=" + ip + ",unc=\\\\" + ip + "\\" + userName + ",user=" + userName + ",pass=" + password + ",sec=" + securityMode
                      + ",uid=" +std::to_string(uid) + ",gid="+std::to_string(gid);
       int result = mount (src.c_str(), path.c_str(), type, mntflags , data.c_str());
       LOG_DEBUG_SAF("Entering function data :%s", data.c_str());

       if (result == 0)
       {
          SAFUtilityOperation::getInstance().setPathPerm(path, reqData->sessionId);
          mSambaDriveMap[uniqueKey] = generateUniqueSambaDriveId();
          mSambaSessionData[mSambaDriveMap[uniqueKey]] = reqData->sessionId;
          mSambaPathMap[mSambaDriveMap[uniqueKey]] = path;
          SAFUtilityOperation::getInstance().setDriveDetails(SAMBA_NAME, mSambaPathMap);
          mntpathmap[path] = path;
          pbnjson::JValue responsePayObjArr = pbnjson::Array();
          pbnjson::JValue responsePayObj = pbnjson::Object();
          pbnjson::JValue mountObj = pbnjson::Object();
          mountObj.put("mountPath", path);
          responsePayObj.put("type", reqData->params["operation"]["type"].asString());
          responsePayObj.put("payload", mountObj);
          responsePayObjArr.append(responsePayObj);
          respObj.put("returnValue", true);
          respObj.put("responsePayload", responsePayObjArr);
          reqData->cb(respObj, reqData->subs);
          return;
       }
       else
       {
          respObj.put("returnValue", false);
          respObj.put("error", "unable to mount: " + path);
       }
   }
   else{
          pbnjson::JValue responsePayObjArr = pbnjson::Array();
          pbnjson::JValue responsePayObj = pbnjson::Object();
          pbnjson::JValue payloadObj = pbnjson::Object();
          pbnjson::JValue mountObj = pbnjson::Object();
          mountObj.put("mountPath", path);
          responsePayObj.put("type", reqData->params["operation"]["type"].asString());
          responsePayObj.put("payload", mountObj);
          responsePayObjArr.append(responsePayObj);
          respObj.put("returnValue", true);
          respObj.put("responsePayload", responsePayObjArr);
          reqData->cb(respObj, reqData->subs);
          return;
   }
   reqData->cb(respObj, reqData->subs);
}

pbnjson::JValue NetworkProvider::parseMediaServer(std::string url)
{
    LOG_DEBUG_SAF("Entering function %s___%s", __FUNCTION__,url.c_str());
    std::string temp = url.substr((url.find("//") + 2));
    std::string ip = temp.substr(0, (temp.find(":")));
    temp = temp.substr(0, temp.rfind("/"));
    std::string port = temp.substr(temp.find(":") + 1);
    pbnjson::JValue responsePayObj = pbnjson::Object();
    LOG_DEBUG_SAF("Entering function %s___%s", ip.c_str(),port.c_str());
    responsePayObj.put("ip", ip);
    responsePayObj.put("port", port);
    responsePayObj.put("descriptionUrl", url);
    responsePayObj.put("name", "MEDIA SERVER");
    responsePayObj.put("urn", "urn:schemas-upnp-org:service:ContentDirectory:1");
    LOG_DEBUG_SAF("Exiting function ");
    return responsePayObj;
}

void NetworkProvider::discoverUPnPMediaServer(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    pbnjson::JValue mediaObjArr = pbnjson::Array();
    std::string serverType = reqData->params["operation"]["payload"]["serverType"].asString();
    UpnpDiscover::getInstance().startScan();
    auto &devs = UpnpDiscover::getInstance().getScannedDevices();
    LOG_DEBUG_SAF("UPnP device size : %d", devs.size());
    for (const auto& dev : devs)
    {
        LOG_DEBUG_SAF("UPnP device %s", dev.c_str());
        pbnjson::JValue mediaObj = pbnjson::Object();
        //mediaObj.put("mediaSever",dev);
        mediaObj = parseMediaServer(dev);
        std::string uniqueKey = mediaObj["ip"].asString() + "_" + mediaObj["port"].asString();
        LOG_DEBUG_SAF("UPnP uniqueKey : %s", uniqueKey.c_str());
        if (mUpnpDriveMap.find(uniqueKey) == mUpnpDriveMap.end())
        {
            mUpnpDriveMap[uniqueKey] = generateUniqueUpnpDriveId();
            mUpnpSessionData[mUpnpDriveMap[uniqueKey]] = reqData->sessionId;
            mUpnpPathMap[mUpnpDriveMap[uniqueKey]] = mediaObj["descriptionUrl"].asString();
        }
        mediaObjArr.append(mediaObj);
    }

    pbnjson::JValue responsePayObjArr = pbnjson::Array();
    pbnjson::JValue responsePayObj = pbnjson::Object();
    pbnjson::JValue payloadObj = pbnjson::Object();
    payloadObj.put("mediaServer", mediaObjArr);
    responsePayObj.put("type", reqData->params["operation"]["type"].asString());
    responsePayObj.put("payload", payloadObj);
    responsePayObjArr.append(responsePayObj);
    respObj.put("returnValue", true);
    respObj.put("responsePayload", responsePayObjArr);
    reqData->cb(respObj, reqData->subs);
}

void NetworkProvider::list(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    std::string path = reqData->params["path"].asString();
    std::string type = SAMBA_NAME;
    if (driveId.find(UPNP_NAME) != std::string::npos)   type = UPNP_NAME;
    if (((type == SAMBA_NAME) && (mSambaSessionData.find(driveId) == mSambaSessionData.end()))
    || ((type == UPNP_NAME) && (mUpnpSessionData.find(driveId) == mUpnpSessionData.end())))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(type == SAMBA_NAME)
    {
        if(!validateSambaOperation(driveId,reqData->sessionId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PARAM);
            respObj.put("errorText", "Invalid DriveID");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        if(!SAFUtilityOperation::getInstance().validateSambaPath(path, driveId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PATH);
            respObj.put("errorText", "Invalid Path");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        int offset = reqData->params["offset"].asNumber<int>();
        int limit = reqData->params["limit"].asNumber<int>();
        bool status = false;
        int totalCount = 0;
        std::string fullPath;
        pbnjson::JValue contenResArr = pbnjson::Array();
        std::unique_ptr<FolderContents> contsPtr = SAFUtilityOperation::getInstance().getListFolderContents(path);
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

    else
    {
        if(!validateUpnpOperation(driveId,reqData->sessionId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PARAM);
            respObj.put("errorText", "Invalid DriveID");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        if (path.empty() || (path.find("/") != 0)
            || (path[path.size() - 1] == '/'))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PATH);
            respObj.put("errorText", "Invalid Path");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        pbnjson::JValue contenResArr = pbnjson::Array();
        pbnjson::JValue respObj = pbnjson::Object();
        bool status = false;
        int totalCount = 0;
        LOG_DEBUG_SAF("UPnP Description URL  : %s", mUpnpPathMap[driveId].c_str());
        auto containerId = UpnpOperation::getInstance().getContainerId(mUpnpPathMap[driveId], path);
        LOG_DEBUG_SAF("UPnP container Id : %d", containerId);
        auto devs = UpnpOperation::getInstance().listDirContents(containerId);
        LOG_DEBUG_SAF("UPnP devs size : %d", devs.size());
        for (auto dev : devs)
        {
            LOG_DEBUG_SAF("UPnP file name %s", dev.title.c_str());
            pbnjson::JValue contentObj = pbnjson::Object();
            contentObj.put("name",dev.title);
            contentObj.put("id",dev.id);
            contentObj.put("childCount",dev.childCount);
            contentObj.put("mimeType",dev.className);
            contentObj.put("url",dev.resUrl);
            contenResArr.append(contentObj);
            status = true;
            totalCount++;
        }
        respObj.put("returnValue", status);
        if (status)
        {
            respObj.put("files", contenResArr);
            respObj.put("totalCount", totalCount);
            respObj.put("fullPath", path);
        }
        else
        {
            respObj.put("errorCode", 101);
            respObj.put("errorText", "UPNP List Faild ");
        }
        reqData->cb(respObj, reqData->subs);
    }
}

void NetworkProvider::getProperties(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    std::string type = SAMBA_NAME;
    if (driveId.find(UPNP_NAME) != std::string::npos)   type = UPNP_NAME;
    if (((type == SAMBA_NAME) && (mSambaSessionData.find(driveId) == mSambaSessionData.end()))
    || ((type == UPNP_NAME) && (mUpnpSessionData.find(driveId) == mUpnpSessionData.end())))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }

    std::string path;
    if (type == UPNP_NAME)
    {
        if(!validateUpnpOperation(driveId,reqData->sessionId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PARAM);
            respObj.put("errorText", "Invalid DriveID");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        path = mUpnpPathMap[driveId];
    }
    else
    {
        if(!validateSambaOperation(driveId,reqData->sessionId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PARAM);
            respObj.put("errorText", "Invalid DriveID");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        path = mSambaPathMap[driveId];
    }
    if (reqData->params.hasKey("path"))
    {
        path = reqData->params["path"].asString();
    }

    respObj.put("returnValue", true);
    respObj.put("storageType", "network");
    pbnjson::JValue attributesArr = pbnjson::Array();
    if (type == UPNP_NAME)
    {
        auto containerId = UpnpOperation::getInstance().getContainerId(mUpnpPathMap[driveId], path);
        LOG_DEBUG_SAF("UPnP container Id : %d", containerId);
        auto devs = UpnpOperation::getInstance().listDirContents(containerId);
        LOG_DEBUG_SAF("UPnP devs size : %d", devs.size());
        if(!devs.empty())
        {
            auto dev = devs[0];
            pbnjson::JValue contentObj = pbnjson::Object();
            contentObj.put("id", dev.id);
            contentObj.put("restricted", dev.restricted);
            contentObj.put("mimeType", dev.className);
            contentObj.put("title", dev.title);
            contentObj.put("childCount", dev.childCount);
            contentObj.put("url", dev.resUrl);
            attributesArr.append(contentObj);
        }
    }
    else if (type == SAMBA_NAME)
    {
        if(!SAFUtilityOperation::getInstance().validateSambaPath(path, driveId))
        {
            respObj.put("errorCode", SAFErrors::INVALID_PATH);
            respObj.put("errorText", "Invalid Path");
            reqData->cb(respObj, reqData->subs);
            return;
        }
        std::unique_ptr<InternalSpaceInfo> propPtr = SAFUtilityOperation::getInstance().getProperties(path);
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
            if (path == mSambaPathMap[driveId])
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
            reqData->cb(respObj, reqData->subs);
            return;
        }
    }
    respObj.put("attributes", attributesArr);
    reqData->cb(respObj, reqData->subs);
}

void NetworkProvider::copy(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    if(!SAFUtilityOperation::getInstance().validateInterProviderOperation(reqData))
        return;

    std::string srcdriveId = reqData->params["srcDriveId"].asString();
    std::string destDriveId = reqData->params["destDriveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if((!validateSambaOperation(srcdriveId,reqData->sessionId)))
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

    std::unique_ptr<InternalCopy> copyPtr = SAFUtilityOperation::getInstance().copy(srcPath, destPath, overwrite);

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

void NetworkProvider::move(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    if(!SAFUtilityOperation::getInstance().validateInterProviderOperation(reqData))
        return;

    std::string srcdriveId = reqData->params["srcDriveId"].asString();
    std::string destDriveId = reqData->params["destDriveID"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if((!validateSambaOperation(srcdriveId,reqData->sessionId)))
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

    std::unique_ptr<InternalMove> movePtr = SAFUtilityOperation::getInstance().move(srcPath, destPath, overwrite);

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

void NetworkProvider::remove(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    std::string path = reqData->params["path"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    if(!validateSambaOperation(driveId,reqData->sessionId))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(!SAFUtilityOperation::getInstance().validateSambaPath(path, driveId))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PATH);
        respObj.put("errorText", "Invalid Path");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::unique_ptr<InternalRemove> remPtr = SAFUtilityOperation::getInstance().remove(path);
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

void NetworkProvider::rename(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::string driveId = reqData->params["driveId"].asString();
    pbnjson::JValue respObj = pbnjson::Object();
    std::string srcPath = reqData->params["path"].asString();
    if(!validateSambaOperation(driveId,reqData->sessionId))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    if(!SAFUtilityOperation::getInstance().validateSambaPath(srcPath, driveId))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PATH);
        respObj.put("errorText", "Invalid Path");
        reqData->cb(respObj, reqData->subs);
        return;
    }
    std::string destPath = reqData->params["newName"].asString();
    std::unique_ptr<InternalRename> renamePtr = SAFUtilityOperation::getInstance().rename(srcPath, destPath);
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

void NetworkProvider::listStoragesMethod(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    pbnjson::JValue networkResArr = pbnjson::Array();
    for (auto data : mSambaSessionData)
    {
        if (data.second != reqData->sessionId)  continue;
        pbnjson::JValue networkRes = pbnjson::Object();
        networkRes.put("driveName", "SAMBA");
        networkRes.put("driveId", data.first);
        networkRes.put("path", mSambaPathMap[data.first]);
        networkResArr.append(networkRes);
    }
    for (auto data : mUpnpSessionData)
    {
        if (data.second != reqData->sessionId)  continue;
        pbnjson::JValue networkRes = pbnjson::Object();
        networkRes.put("driveName", "UPNP");
        networkRes.put("driveId", data.first);
        networkRes.put("path", mUpnpPathMap[data.first]);
        networkResArr.append(networkRes);
    }
    respObj.put("network", networkResArr);
    reqData->params.put("response", respObj);
    reqData->cb(reqData->params, std::move(reqData->subs));
}

void NetworkProvider::eject(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    pbnjson::JValue respObj = pbnjson::Object();
    std::string driveId = reqData->params["driveId"].asString();
    if(!validateSambaOperation(driveId,reqData->sessionId))
    {
        respObj.put("errorCode", SAFErrors::INVALID_PARAM);
        respObj.put("errorText", "Invalid DriveID");
        reqData->params.put("response", respObj);
        reqData->cb(reqData->params, std::move(reqData->subs));
        return;
    }
    for(auto entry : mSambaDriveMap)
    {
        if(entry.second == driveId)
        {
            mSambaDriveMap.erase(entry.first);
            break;
        }
    }
    respObj.put("returnValue", true);
    if(mSambaPathMap.find(driveId) != mSambaPathMap.end()){
     int result =  umount2(mSambaPathMap[driveId].c_str(),MNT_FORCE);
     if (result ==0){
         SAFUtilityOperation::getInstance().remove(mSambaPathMap[driveId]);
         mntpathmap.erase(mSambaPathMap[driveId]);
         mSambaSessionData.erase(driveId);
         mSambaPathMap.erase(driveId);
         SAFUtilityOperation::getInstance().setDriveDetails(SAMBA_NAME, mSambaPathMap);
         respObj.put("returnValue", true);
         respObj.put("unmountPath: ", driveId);
     }
     else{
             respObj.put("returnValue", false);
             respObj.put("error", "Unable to eject : " + driveId);
     }
    }
    else{
         respObj.put("returnValue", false);
         respObj.put("error", "Not Mounted : " + driveId);
    }
    reqData->params.put("response", respObj);
    reqData->cb(reqData->params, std::move(reqData->subs));
}

void NetworkProvider::addRequest(std::shared_ptr<RequestData>& reqData)
{
    LOG_DEBUG_SAF("NetworkProvider :: Entering function %s", __FUNCTION__);
    mQueue.push_back(std::move(reqData));
    mCondVar.notify_one();
}

void NetworkProvider::handleRequests(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    switch(reqData->methodType)
    {
        case MethodType::EXTRA_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EXTRA_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->extraMethod(reqData); });
            (void)fut;
        }
        break;
        case MethodType::LIST_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::GET_PROP_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->list(reqData); });
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
        case MethodType::REMOVE_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::REMOVE_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->remove(reqData); });
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
        case MethodType::COPY_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::COPY_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->copy(reqData); });
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
        case MethodType::EJECT_METHOD:
        {
            LOG_DEBUG_SAF("%s : MethodType::EJECT_METHOD", __FUNCTION__);
            auto fut = std::async(std::launch::async, [this, reqData]() { return this->eject(reqData); });
            (void)fut;
        }
        break;
        default:
        break;
    }
}

void NetworkProvider::dispatchHandler()
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::unique_lock < std::mutex > lock(mMutex);
    do {
        mCondVar.wait(lock, [this] {
            return (mQueue.size() || mQuit);
        });
        LOG_DEBUG_SAF("Dispatch notify received : %d, mQuit: %d", mQueue.size(), mQuit);
        if (mQueue.size() && !mQuit)
        {
            lock.unlock();
            handleRequests(std::move(mQueue.front()));
            mQueue.erase(mQueue.begin());
            lock.lock();
        }
    } while (!mQuit);
}

