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

#ifndef _USB_OPERATION_HANDLER_H_
#define _USB_OPERATION_HANDLER_H_
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include "SAFErrors.h"

enum USBOperErrors
{
    NO_ERROR = 0,
    UNKNOWN = -1,
    INVALID_PATH = -2,
    INVALID_SOURCE_PATH = -3,
    INVALID_DEST_PATH = -4,
    FILE_ALREADY_EXISTS = -5,
    PERMISSION_DENIED = -6,
    SUCCESS = 100
};

int getUSBErrorCode(int errorCode);

class USBFolderContent
{
private:
    std::string mName;
    std::string mPath;
    std::string mType;
    uint32_t mSize;

    void init();
    std::string getFileType(std::string);
    uint32_t getFileSize(std::string);
public:
    USBFolderContent(std::string);
    std::string getName() { return mName; }
    std::string getPath() { return mPath; }
    std::string getType() { return mType; }
    uint32_t getSize() { return mSize; }
};

class USBFolderContents
{
private:
    std::string mFullPath;
    std::uint32_t mTotalCount;
    std::vector<std::shared_ptr<USBFolderContent>> mContents;
    int32_t mStatus;
    std::string mErrMsg;
    void init();
public:
    USBFolderContents(std::string);
    int32_t getStatus() { return mStatus; }
    std::string getPath() { return mFullPath; }
    std::uint32_t getTotalCount() { return mTotalCount; }
    std::string getErrorMsg() { return mErrMsg;}
    std::vector<std::shared_ptr<USBFolderContent>> getContents() { return mContents; }
};

class USBCopy
{
private:
    std::string mSrcPath;
    std::string mDestPath;
    uint32_t mSrcSize;
    uint32_t mDestSize;
    uint32_t mStatus;
    bool mOverwrite;
    std::string mErrMsg;
    void init();
public:
    USBCopy(std::string, std::string, bool);
    std::uint32_t getStatus();
    std::string getErrorMsg();
};

class USBRemove
{
private:
    std::string mPath;
    int32_t mStatus;
    std::string mErrMsg;
    void init();
public:
    USBRemove(std::string);
    int32_t getStatus();
    std::string getErrorMsg();
};

class USBMove
{
private:
    std::string mSrcPath;
    std::string mDestPath;
    uint32_t mSrcSize;
    uint32_t mDestSize;
    int32_t mStatus;
    bool mOverwrite;
    std::string mErrMsg;
    void init();
public:
    USBMove(std::string, std::string,bool);
    int32_t getStatus();
    std::string getErrorMsg();
};

class USBRename
{
private:
    std::string mOldAbsPath;
    std::string mNewAbsPath;
    int32_t mStatus;
    std::string mErrMsg;
    void init();
public:
    USBRename(std::string, std::string);
    int32_t getStatus();
    std::string getErrorMsg();
};

class USBSpaceInfo
{
private:
    std::string mPath;
    std::uint32_t mCapacity;
    std::uint32_t mFreeSpace;
    std::uint32_t mAvailSpace;
    bool mIsWritable;
    bool mIsDeletable;
    int32_t mStatus;
    void init();
public:
    USBSpaceInfo(std::string);
    std::uint32_t getCapacityMB();
    std::uint32_t getFreeSpaceMB();
    std::uint32_t getAvailSpaceMB();
    bool getIsWritable();
    bool getIsDeletable();
    int32_t getStatus();
};

class USBOperationHandler
{
private:
    USBOperationHandler();
public:
    static USBOperationHandler& getInstance();
    std::unique_ptr<USBCopy> copy(std::string, std::string, bool);
    std::unique_ptr<USBRemove> remove(std::string);
    std::unique_ptr<USBMove> move(std::string, std::string, bool);
    std::unique_ptr<USBFolderContents> getListFolderContents(std::string);
    std::unique_ptr<USBRename> rename(std::string,  std::string);
    std::unique_ptr<USBSpaceInfo> getProperties(std::string path = std::string());
};
#endif /*_USB_OPERATION_HANDLER_H_*/


