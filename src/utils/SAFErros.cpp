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

#include <SAFErrors.h>
#include <SAFLog.h>

std::string SAFErrors::getSAFErrorString(int errorCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    std::string errorText = "Unknown Error";
    if (mSAFErrorTextTable.find(errorCode) != mSAFErrorTextTable.end()) {
        errorText = mSAFErrorTextTable[errorCode];
    }
    return errorText;
}

std::string SAFErrors::InternalErrors::getInternalErrorString(int errorCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    std::string errorText = "Unknown Error";
    if (InternalErrors::mInternalErrorTextTable.find(errorCode) != 
        InternalErrors::mInternalErrorTextTable.end())
    {
        errorText = InternalErrors::mInternalErrorTextTable[errorCode];
    }
    return errorText;
}

std::string SAFErrors::CloudErrors::getCloudErrorString(int errorCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    std::string errorText = "Unknown Error";
    if (CloudErrors::mCloudErrorTextTable.find(errorCode) != 
        CloudErrors::mCloudErrorTextTable.end())
    {
        errorText = CloudErrors::mCloudErrorTextTable[errorCode];
    }
    return errorText;
}

std::string SAFErrors::USBErrors::getUSBErrorString(int errorCode)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);

    std::string errorText = "Unknown Error";
    if (mUSBErrorTextTable.find(errorCode) != mUSBErrorTextTable.end()) {
        errorText = mUSBErrorTextTable[errorCode];
    }
    return errorText;
}

