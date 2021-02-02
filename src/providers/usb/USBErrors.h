// Copyright (c) 2021 LG Electronics, Inc.
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

#ifndef SRC_INCLUDE_USBERRORS_H_
#define SRC_INCLUDE_USBERRORS_H_

#include <string>
#include <map>

namespace USBErrors
{

    enum USBErrors
    {
        USB_UNKNOWN_ERROR = 9000,
        ERROR_NONE,
        MORE_PARTITIONS_IN_USB,
        USB_STORAGE_NOT_EXISTS,
        MORE_ATTACHED_STORAGES_THAN_USB,
        USB_COPY_FAILED,
        USB_MOVE_FAILED,
        USB_REMOVE_FAILED,
        USB_LIST_CONTENTS_FAILED,
        USB_RENAME_FAILED
        USB_ERROR_NOT_SUPPORTED = 9282
    };

    static std::map<int, std::string> mUSBErrorTextTable =
    {
        { USB_ERROR_NOT_SUPPORTED, "Not supported yet" },
        { USB_UNKNOWN_ERROR, "Unknown error" },
        { MORE_PARTITIONS_IN_USB, "Drive Contains more than one partition, please append uuid to storageId"},
        { USB_STORAGE_NOT_EXISTS, "No Storage Device exists with ID given"},
        { MORE_ATTACHED_STORAGES_THAN_USB, "More attached storages apart from USB"},
        { USB_COPY_FAILED, "USB Copy failed"},
        { USB_MOVE_FAILED, "USB Move failed"},
        { USB_REMOVE_FAILED, "USB Remove failed"},
        { USB_LIST_CONTENTS_FAILED, "Listing folder contents failed"},
        { USB_RENAME_FAILED, "Rename Failed"},
        { ERROR_NONE, "No error" },
    };

    std::string getUSBErrorString(int errorCode);
}

using USBErrorCodes = USBErrors::USBErrors;

#endif /* SRC_INCLUDE_USBERRORS_H_ */
