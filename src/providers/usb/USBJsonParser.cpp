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

#include "USBJsonParser.h"
#include <SAFLog.h>
#include "USBErrors.h"

pbnjson::JValue USBPbnJsonParser::ParseListOfStorages(pbnjson::JValue rootObj)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    int numUSBDevices = 0;
    pbnjson::JValue replyObj = rootObj;
    pbnjson::JValue usbArray = pbnjson::Array();
    pbnjson::JValue responseArray = pbnjson::Array();
    pbnjson::JValue responseObj = pbnjson::Object();
    std::string sid = "";
    bool ret = replyObj["returnValue"].asBool();

    if(ret)
    {
        if(replyObj.hasKey("deviceListInfo") && replyObj["deviceListInfo"].isArray() && replyObj["deviceListInfo"].arraySize() > 1)
        {
            responseObj.put("returnValue", false);
            responseObj.put("errorCode", USBErrors::MORE_ATTACHED_STORAGES_THAN_USB);
            responseObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::MORE_ATTACHED_STORAGES_THAN_USB));
        }
        if(replyObj.hasKey("deviceListInfo") && replyObj["deviceListInfo"][0].hasKey("storageDeviceList"))
        {
            usbArray = replyObj["deviceListInfo"][0]["storageDeviceList"];
            numUSBDevices = usbArray.arraySize();
        }
    }
    LOG_DEBUG_SAF("Number of USB devices attached:%d", numUSBDevices);
    if(numUSBDevices == 0)
    {
        responseObj.put("USB", responseArray);
        return responseObj;
    }
    if(ret)
    {
        for(int i = 0; i < numUSBDevices; i++)
        {
            if(usbArray[i].hasKey("storageDriveList"))
            {
                pbnjson::JValue rObj = pbnjson::Object();
                for(int j = 0; j < usbArray[i]["storageDriveList"].arraySize(); j++)
                {
                    pbnjson::JValue rObj = pbnjson::Object();
                    if(usbArray[i]["storageDriveList"][j].hasKey("driveName"))
                        rObj.put("storgaeName", usbArray[i]["storageDriveList"][j]["driveName"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("uuid"))
                        sid = usbArray[i]["serialNumber"].asString() + "-" + usbArray[i]["storageDriveList"][j]["uuid"].asString();
                        rObj.put("storageId", sid);
                        sid = "";
                    if(usbArray[i]["storageDriveList"][j].hasKey("fsType"))
                        rObj.put("fsType", usbArray[i]["storageDriveList"][j]["fsType"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("mountName"))
                        rObj.put("mountPath", usbArray[i]["storageDriveList"][j]["mountName"]);
                    responseArray.append(rObj);
                }
            }
        }
        responseObj.put("USB", responseArray);
    }
    return responseObj;
}

pbnjson::JValue USBPbnJsonParser::ParseGetProperties(pbnjson::JValue rootObj, std::string deviceStorageId)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    pbnjson::JValue rawObj = rootObj;
    pbnjson::JValue respObj = pbnjson::Object();

    if(rawObj.isArray() == false)
    {
        if(rawObj["returnValue"].asBool() == false)
        {
            return rawObj;
        }
    }
    else
    {
        respObj.put("returnValue", true);
    }
    for(int i = 0; i < rootObj.arraySize(); ++i)
    {
        pbnjson::JValue eachObj = rootObj[i];
        if(eachObj.hasKey("storageType"))
        {
            respObj.put("storageType", eachObj["storageType"].asString());
        }
        else if(eachObj.hasKey("isWritable"))
        {
            respObj.put("isWritable", eachObj["isWritable"].asBool());
        }
        else if(eachObj.hasKey("spaceInfo"))
        {
            pbnjson::JValue infoObj = eachObj["spaceInfo"];
            respObj.put("totalSpace", infoObj["totalSize"]);
            respObj.put("freeSpace",  infoObj["freeSize"]);
        }
    }
    return respObj;
}


pbnjson::JValue USBPbnJsonParser::ParseEject(pbnjson::JValue rootObj)
{
    return rootObj[0];
}

pbnjson::JValue USBPbnJsonParser::ParseFormat(pbnjson::JValue rootObj)
{
    return rootObj[0];
}