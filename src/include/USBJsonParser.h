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

#ifndef __USB_JSON_PARSER_H__
#define __USB_JSON_PARSER_H__

#include <string>
#include <pbnjson.hpp>

class USBPbnJsonParser
{
public:
    pbnjson::JValue ParseListOfStorages(pbnjson::JValue rootObj);
};

#endif