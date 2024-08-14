/* @@@LICENSE
 *
 * Copyright (c) 2021-2024 LG Electronics, Inc.
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
#include "SAFErrors.h"

pbnjson::JValue USBPbnJsonParser::ParseListOfStorages(pbnjson::JValue rootObj)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    int numUSBDevices = 0;
    pbnjson::JValue replyObj = std::move(rootObj);
    pbnjson::JValue usbArray = pbnjson::Array();
    pbnjson::JValue responseArray = pbnjson::Array();
    pbnjson::JValue responseObj = pbnjson::Object();
    std::string sid = "";
    bool ret = replyObj["returnValue"].asBool();

    if(ret)
    {
        if(replyObj.hasKey("deviceListInfo"))
        {
            if(replyObj["deviceListInfo"].isArray() && replyObj["deviceListInfo"].arraySize() > 1)
            {
                responseObj.put("returnValue", false);
                responseObj.put("errorCode", SAFErrors::USBErrors::MORE_ATTACHED_STORAGES_THAN_USB);
                responseObj.put("errorText", SAFErrors::USBErrors::getUSBErrorString(SAFErrors::USBErrors::MORE_ATTACHED_STORAGES_THAN_USB));
                return responseObj;
            }
            else if(replyObj["deviceListInfo"].isArray() && replyObj["deviceListInfo"].arraySize() == 1)
            {
                if(replyObj["deviceListInfo"][0].hasKey("storageDeviceList"))
                {
                    if(replyObj["deviceListInfo"][0]["storageDeviceList"].isArray())
                    {
                        usbArray = replyObj["deviceListInfo"][0]["storageDeviceList"];
                        numUSBDevices = usbArray.arraySize();
                    }
                }
            }
        }

        if(replyObj.hasKey("storageDeviceList"))
        {
            if(replyObj["storageDeviceList"].isArray())
            {
                usbArray = replyObj["storageDeviceList"];
                numUSBDevices = usbArray.arraySize();
            }
        }
    }

    LOG_DEBUG_SAF("Number of USB devices attached:%d", numUSBDevices);
    if(numUSBDevices == 0)
    {
        responseObj.put("usb", responseArray);
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
                        rObj.put("driveName", usbArray[i]["storageDriveList"][j]["driveName"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("uuid"))
                    {
                        sid = usbArray[i]["serialNumber"].asString() + "-" + usbArray[i]["storageDriveList"][j]["uuid"].asString();
                        rObj.put("driveId", sid);
                        sid = "";
                    }
                    if(usbArray[i]["storageDriveList"][j].hasKey("fsType"))
                        rObj.put("fileSystem", usbArray[i]["storageDriveList"][j]["fsType"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("mountName"))
                        rObj.put("path", usbArray[i]["storageDriveList"][j]["mountName"]);
                    responseArray.append(rObj);
                }
            }
        }
        responseObj.put("usb", responseArray);
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
            respObj.put("writable", eachObj["isWritable"].asBool());
            respObj.put("deletable", eachObj["isWritable"].asBool());
        }
        else if(eachObj.hasKey("spaceInfo"))
        {
            pbnjson::JValue infoObj = eachObj["spaceInfo"];
            if(infoObj.hasKey("totalSize"))
                respObj.put("totalSpace", infoObj["totalSize"]);
            if(infoObj.hasKey("driveSize"))
                respObj.put("totalSpace", infoObj["driveSize"]);
            respObj.put("freeSpace",  infoObj["freeSize"]);
        }
    }
    return respObj;
}


pbnjson::JValue USBPbnJsonParser::ParseEject(pbnjson::JValue rootObj)
{
    if(rootObj.isArray())
    {
        if(rootObj.arraySize() == 1)
        {
            return rootObj[0];
        }
    }
    return rootObj;
}

pbnjson::JValue USBPbnJsonParser::ParseFormat(pbnjson::JValue rootObj)
{
    return rootObj[0];
}
