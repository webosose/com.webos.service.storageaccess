/* @@@LICENSE
 *
 * Copyright (c) 2020 LG Electronics, Inc.
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
    responseObj.put("returnValue", ret);
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
                rObj.put("deviceNum", usbArray[i]["deviceNum"]);

            if(usbArray[i].hasKey("storageDriveList"))
            {
                if(usbArray[i]["storageDriveList"][0].hasKey("driveName"))
                    rObj.put("storgaeName", usbArray[i]["storageDriveList"][0]["driveName"]);
                if(usbArray[i]["storageDriveList"][0].hasKey("fsType"))
                    rObj.put("fsType", usbArray[i]["storageDriveList"][0]["fsType"]);
                if(usbArray[i]["storageDriveList"][0].hasKey("mountPath"))
                    rObj.put("mountPath", usbArray[i]["storageDriveList"][0]["mountPath"]);
            }
            responseArray.append(rObj);
        }
        responseObj.put("USB Storage", responseArray);
    }
    return responseObj;
}