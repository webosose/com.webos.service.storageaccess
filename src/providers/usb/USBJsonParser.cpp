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
    LOG_DEBUG_SAF("=====================Entering ParseListOfStorages============================");
    int numUSBDevices = 0;
    pbnjson::JValue rawObj = rootObj;
    pbnjson::JValue replyObj = rawObj[0];
    pbnjson::JValue usbArray = pbnjson::Array();
    pbnjson::JValue responseArray = pbnjson::Array();
    pbnjson::JValue responseObj = pbnjson::Object();

    bool ret = replyObj["returnValue"].asBool();

    if(ret)
    {
        if(replyObj.hasKey("deviceListInfo") && replyObj["deviceListInfo"][0].hasKey("storageDeviceList"))
        {
            usbArray = replyObj["deviceListInfo"][0]["storageDeviceList"];
            numUSBDevices = usbArray.arraySize();
        }
    }
    LOG_DEBUG_SAF("Number of USB devices attached:%d", numUSBDevices);
    if(numUSBDevices == 0)
    {
        return rootObj[0];
    }
    if(ret)
    {
        for(int i = 0; i < numUSBDevices; i++)
        {
            pbnjson::JValue rObj = pbnjson::Object();
            if(usbArray[i].hasKey("serialNumber"))
                rObj.put("storgaeId", usbArray[i]["serialNumber"]);
            if(usbArray[i].hasKey("deviceType"))
                rObj.put("storgaeType", usbArray[i]["deviceType"]);
            if(usbArray[i].hasKey("deviceSetId"))
                rObj.put("storageSetId", usbArray[i]["deviceSetId"]);
            if(usbArray[i].hasKey("deviceNum"))
                rObj.put("storageNumber", usbArray[i]["deviceNum"]);

            if(usbArray[i].hasKey("storageDriveList"))
            {
                pbnjson::JValue partitionArray = pbnjson::Array();
                for(int j = 0; j < usbArray[i]["storageDriveList"].arraySize(); j++)
                {
                    pbnjson::JValue rObj2 = pbnjson::Object();
                    if(usbArray[i]["storageDriveList"][j].hasKey("driveName"))
                        rObj2.put("storgaeName", usbArray[i]["storageDriveList"][j]["driveName"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("uuid"))
                        rObj2.put("uuid", usbArray[i]["storageDriveList"][j]["uuid"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("fsType"))
                        rObj2.put("fsType", usbArray[i]["storageDriveList"][j]["fsType"]);
                    if(usbArray[i]["storageDriveList"][j].hasKey("mountPath"))
                        rObj2.put("mountPath", usbArray[i]["storageDriveList"][j]["mountPath"]);
                    partitionArray.append(rObj2);
                }
                rObj.put("storageDriveList", partitionArray);
            }
            responseArray.append(rObj);
        }
        responseObj.put("USB", responseArray);
    }
    return responseObj;
}

pbnjson::JValue USBPbnJsonParser::ParseGetProperties(pbnjson::JValue rootObj, std::string deviceStorageId)
{
    LOG_DEBUG_SAF("=====================Entering ParseGetProperties============================");
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
        if(eachObj.hasKey("deviceListInfo") && eachObj["deviceListInfo"].arraySize() == 1)
        {
            pbnjson::JValue storageDeviceList = eachObj["deviceListInfo"][0]["storageDeviceList"];
            for (int i=0; i<storageDeviceList.arraySize(); ++i)
            {
                if(storageDeviceList[i]["serialNumber"].asString().compare(deviceStorageId.substr(0,deviceStorageId.find("-"))) == 0)
                {
                    respObj.put("storageType", storageDeviceList[i]["deviceType"].asString());
                    break;
                }
            }
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
        else
        {
            respObj.put("returnValue", false);
            respObj.put("errorCode", USBErrors::MORE_ATTACHED_STORAGES_THAN_USB);
            respObj.put("errorText", USBErrors::getUSBErrorString(USBErrors::MORE_ATTACHED_STORAGES_THAN_USB));
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