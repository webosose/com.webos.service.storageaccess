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

#ifndef _INTERNAL_OPERATION_HANDLER_H_
#define _INTERNAL_OPERATION_HANDLER_H_
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <stdint.h>

class FolderContent
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
    FolderContent(std::string);
    std::string getName() { return mName; }
    std::string getPath() { return mPath; }
    std::string getType() { return mType; }
    uint32_t getSize() { return mSize; }
};

class FolderContents
{
private:
    std::string mFullPath;
    std::uint32_t mTotalCount;
    std::vector<FolderContent> mContents;
    void init();
public:
    FolderContents(std::string);
    std::string getPath() { return mFullPath; }
    std::uint32_t getTotalCount() { return mTotalCount; }
    std::vector<FolderContent> getContents() { return mContents; }
};

class InternalSpaceInfo
{
private:
    std::string mPath;
    std::uint32_t mCapacity;
    std::uint32_t mFreeSpace;
    std::uint32_t mAvailSpace;
    bool mIsWritable;
    bool mIsDeletable;
    void init();
public:
    InternalSpaceInfo(std::string);
    std::uint32_t getCapacityMB();
    std::uint32_t getFreeSpaceMB();
    std::uint32_t getAvailSpaceMB();
    bool getIsWritable();
    bool getIsDeletable();
};

class InternalCopy
{
private:
    std::string mSrcPath;
    std::string mDestPath;
    uint32_t mSrcSize;
    uint32_t mDestSize;
    int32_t mStatus;
    void init();
public:
    InternalCopy(std::string, std::string);
    std::uint32_t getStatus();
};

class InternalRemove
{
private:
    std::string mPath;
    int32_t mStatus;
    void init();
public:
    InternalRemove(std::string);
    int32_t getStatus();
};

class InternalMove
{
private:
    std::string mSrcPath;
    std::string mDestPath;
    int32_t mStatus;
    void init();
public:
    InternalMove(std::string, std::string);
    int32_t getStatus();
};


class InternalOperationHandler
{
private:
    std::shared_ptr<FolderContents> mFolderContentObj;
    std::shared_ptr<InternalSpaceInfo> mSpaceInfoObj;
    std::shared_ptr<InternalCopy> mCopyObj;
    std::shared_ptr<InternalRemove> mRemoveObj;
    std::shared_ptr<InternalMove> mMoveObj;
    void dumpContents();
    InternalOperationHandler();
public:
    static InternalOperationHandler& getInstance();
    std::shared_ptr<FolderContents> getListFolderContents(std::string);
    std::shared_ptr<InternalSpaceInfo> getProperties(std::string path = std::string());
    std::shared_ptr<InternalCopy> copy(std::string, std::string);
    std::shared_ptr<InternalRemove> remove(std::string);
    std::shared_ptr<InternalMove> move(std::string, std::string);
};


#endif /*_INTERNAL_OPERATION_HANDLER_H_*/
