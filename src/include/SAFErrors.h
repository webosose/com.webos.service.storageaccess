// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef SRC_INCLUDE_SAFERRORS_H_
#define SRC_INCLUDE_SAFERRORS_H_

#include <string>
#include <map>

namespace SAFErrors
{
    enum SAFErrors
    {
        UNKNOWN_ERROR = 8000,
        SERVICE_NOT_READY,
        INIT_ERROR,
        INVALID_PARAM,
        SAF_INTERNAL_ERROR,
        SAF_MEMORY_ERROR,
        PARAM_MISSING,
        LANG_NOT_SUPPORTED,
        SERVICE_ALREADY_RUNNING,
        INVALID_JSON_FORMAT,
        INPUT_TEXT_EMPTY,
        ERROR_NONE,
        INVALID_COMMAND,
        STORAGE_TYPE_NOT_SUPPORTED,
        INVALID_PATH,
        INVALID_SOURCE_PATH,
        INVALID_DEST_PATH,
        FILE_ALREADY_EXISTS,
        NO_ERROR,
        PERMISSION_DENIED,
        SAF_ERROR_NOT_SUPPORTED = 8282,
    };

    enum SAFConfigErrors
    {
        SAF_CONFIG_ERROR_NONE = 2000,
        SAF_CONFIG_ERROR_LOAD,
        SAF_CONFIG_ERROR_NOCATEGORY,
        SAF_CONFIG_ERROR_NOKEY
    };

    static std::map<int, std::string> mSAFErrorTextTable =
    {
        { SAF_ERROR_NOT_SUPPORTED, "Not supported yet" },
        { SAF_CONFIG_ERROR_NONE, "Configuration success" },
        { SAF_CONFIG_ERROR_LOAD, "Failed to load config file" },
        { SAF_CONFIG_ERROR_NOCATEGORY, "No category foud in config" },
        { SAF_CONFIG_ERROR_NOKEY, "No valid found in config" },
        { UNKNOWN_ERROR, "Unknown error" },
        { SERVICE_NOT_READY, "Service is not ready" },
        { INIT_ERROR, "Initialize error" },
        { INVALID_PARAM, "Invalid parameter" },
        { SAF_INTERNAL_ERROR, "Internal error" },
        { SAF_MEMORY_ERROR, "Memory Allocation Error" },
        { PARAM_MISSING, "Required parameter is missing" },
        { SERVICE_ALREADY_RUNNING, "Service is already running" },
        { INVALID_JSON_FORMAT, "Invalid JSON format" },
        { INPUT_TEXT_EMPTY, "Input text must not be empty" },
        { ERROR_NONE, "No error" },
        { INVALID_PATH, "No such file or directory"},
        { INVALID_SOURCE_PATH, "No such file or directory at source"},
        { INVALID_DEST_PATH, "No such file or directory at destination"},
        { FILE_ALREADY_EXISTS, "File Exists"},
        { NO_ERROR, "No Error"},
        { PERMISSION_DENIED, "Permission Denied"},
        { STORAGE_TYPE_NOT_SUPPORTED, "Operation not permitted"}
    };

    std::string getSAFErrorString(int errorCode);
    namespace InternalErrors
	{
	    static std::map<int, std::string> mInternalErrorTextTable =
	    {
	        { SAFErrors::UNKNOWN_ERROR, "Internal Unknown Error" },
	        { SAFErrors::INVALID_PARAM, "Invalid Internal Parameters" },
	        { SAFErrors::INVALID_PATH, "Invalid Internal Path" },
	        { SAFErrors::INVALID_SOURCE_PATH, "Invalid Internal Source Path" },
	        { SAFErrors::INVALID_DEST_PATH, "Invalid Internal Destination Path" },
	        { SAFErrors::FILE_ALREADY_EXISTS, "Internal File Already Exists" },
	        { SAFErrors::PERMISSION_DENIED, "Internal File Permission Denied" },
	        { SAFErrors::NO_ERROR, "Internal No Error" }
	    };
		std::string getInternalErrorString(int errorCode);
	}
	namespace CloudErrors
	{
		enum CloudErrorsEnum
		{
			ATTACH_NOT_DONE = 6000,
			AUTHENTICATION_NOT_DONE,
			ALREADY_AUTHENTICATED,
			INVALID_URL,
			INVALID_FILE
		};
	    static std::map<int, std::string> mCloudErrorTextTable =
	    {
	        { SAFErrors::UNKNOWN_ERROR, "Cloud Unknown Error" },
	        { SAFErrors::INVALID_PARAM, "Invalid Cloud Parameters" },
	        { SAFErrors::INVALID_PATH, "Invalid Cloud Path" },
	        { SAFErrors::INVALID_SOURCE_PATH, "Invalid Cloud Source Path" },
	        { SAFErrors::INVALID_DEST_PATH, "Invalid Cloud Destination Path" },
	        { SAFErrors::FILE_ALREADY_EXISTS, "Cloud File Already Exists" },
	        { ATTACH_NOT_DONE, "Attach Cloud Not Done" },
			{ AUTHENTICATION_NOT_DONE, "Permission Denied" },
			{ ALREADY_AUTHENTICATED, "Already Authenticated" },
			{ INVALID_URL, "Invalid URL" },
			{ INVALID_FILE, "File Not Supported" },
	    };
		std::string getCloudErrorString(int errorCode);
	}
	namespace USBErrors
	{
	    enum USBErrors
	    {
	        MORE_PARTITIONS_IN_USB = 9000,
	        USB_STORAGE_NOT_EXISTS,
	        USB_SUB_STORAGE_NOT_EXISTS,
	        MORE_ATTACHED_STORAGES_THAN_USB,
	        DRIVE_NOT_MOUNTED,
	        USB_DRIVE_ALREADY_EJECTED
	    };

	    static std::map<int, std::string> mUSBErrorTextTable =
	    {
	        { SAFErrors::SAF_ERROR_NOT_SUPPORTED, "USB not supported" },
	        { SAFErrors::UNKNOWN_ERROR, "USB unknown error" },
	        { MORE_PARTITIONS_IN_USB, "USB Storage has more partitions"},
	        { USB_STORAGE_NOT_EXISTS, "No USB Storage Exists with ID given"},
	        { USB_SUB_STORAGE_NOT_EXISTS, "No USB drive found"},
	        { MORE_ATTACHED_STORAGES_THAN_USB, "More attached storages apart from USB"},
	        { SAFErrors::INVALID_PATH, "Invalid USB Path" },
	        { SAFErrors::INVALID_SOURCE_PATH, "Invalid USB Source Path" },
	        { SAFErrors::INVALID_DEST_PATH, "Invalid USB Destination Path" },
	        { SAFErrors::FILE_ALREADY_EXISTS, "USB File Already Exists" },
	        { DRIVE_NOT_MOUNTED, "USB Drive Not Mounted"},
	        { SAFErrors::NO_ERROR, "USB No error" },
	        { USB_DRIVE_ALREADY_EJECTED, "Drive Already Ejected"}
	    };

	    std::string getUSBErrorString(int errorCode);
	}
}

using SAFErrorCodes = SAFErrors::SAFErrors;
using SAFConfigError = SAFErrors::SAFConfigErrors;

#endif /* SRC_INCLUDE_TTSERRORS_H_ */
