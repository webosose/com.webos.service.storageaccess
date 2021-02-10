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
	namespace InternalErrors
	{
		enum InternalErrorEnum
		{
			UNKNOWN_ERROR = 5000,
			INVALID_PARAM,
			INVALID_PATH,
			INVALID_SOURCE_PATH,
			INVALID_DEST_PATH,
			FILE_ALREADY_EXISTS,
			NO_ERROR,
		};
	    static std::map<int, std::string> mInternalErrorTextTable =
	    {
	        { UNKNOWN_ERROR, "Unknown Error" },
	        { INVALID_PARAM, "Invalid Parameters" },
	        { INVALID_PATH, "Invalid Path" },
	        { INVALID_SOURCE_PATH, "Invalid Source Path" },
	        { INVALID_DEST_PATH, "Invalid Destination Path" },
	        { FILE_ALREADY_EXISTS, "File Already Exists" },
	        { NO_ERROR, "No Error" }
	    };
		std::string getInternalErrorString(int errorCode);
	}
	namespace CloudErrors
	{
		enum CloudErrorsEnum
		{
			UNKNOWN_ERROR = 6000,
			INVALID_PARAM,
			INVALID_PATH,
			INVALID_SOURCE_PATH,
			INVALID_DEST_PATH,
			FILE_ALREADY_EXISTS,
			ATTACH_NOT_DONE,
			AUTHENTICATION_NOT_DONE,
			ALREADY_AUTHENTICATED,
			INVALID_URL,
			INVALID_FILE
		};
	    static std::map<int, std::string> mCloudErrorTextTable =
	    {
	        { UNKNOWN_ERROR, "Unknown Error" },
	        { INVALID_PARAM, "Invalid Parameters" },
	        { INVALID_PATH, "Invalid Path" },
	        { INVALID_SOURCE_PATH, "Invalid Source Path" },
	        { INVALID_DEST_PATH, "Invalid Destination Path" },
	        { FILE_ALREADY_EXISTS, "File Already Exists" },
	        { ATTACH_NOT_DONE, "Attach Cloud Not Done" },
			{ AUTHENTICATION_NOT_DONE, "Authentication Not Done" },
			{ ALREADY_AUTHENTICATED, "Already Authenticated" },
			{ INVALID_URL, "Invalid URL" },
			{ INVALID_FILE, "File Not Supported" },
	    };
		std::string getCloudErrorString(int errorCode);
	}

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
        { STORAGE_TYPE_NOT_SUPPORTED, "Storage Type not supported"},
    };

    std::string getSAFErrorString(int errorCode);
}

using SAFErrorCodes = SAFErrors::SAFErrors;
using SAFConfigError = SAFErrors::SAFConfigErrors;

#endif /* SRC_INCLUDE_TTSERRORS_H_ */
