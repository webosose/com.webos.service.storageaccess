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
#include "SAFErrors.h"

enum InternalOperErrors
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
int getInternalErrorCode(int errorCode);

class FolderContent
{
private:
    std::string mName;
    std::string mPath;
    std::string mType;
    std::string mModTime;
    uint32_t mSize;

    void init();
    std::string getFileType(std::string);
    std::string getLastWrite(std::string);
    uint32_t getFileSize(std::string);
    std::string getModTime(std::string);

public:
    FolderContent(std::string);
    std::string getName() { return mName; }
    std::string getPath() { return mPath; }
    std::string getType() { return mType; }
    uint32_t getSize() { return mSize; }
    std::string getLastModTime() { return mModTime; }
};

class FolderContents
{
private:
    std::string mFullPath;
    std::uint32_t mTotalCount;
    std::vector<std::shared_ptr<FolderContent>> mContents;
	int32_t mStatus;
    void init();
public:
    FolderContents(std::string);
	int32_t getStatus() { return mStatus; }
    std::string getPath() { return mFullPath; }
    std::uint32_t getTotalCount() { return mTotalCount; }
    std::vector<std::shared_ptr<FolderContent>> getContents() { return mContents; }
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
	int32_t mStatus;
    void init();
public:
    InternalSpaceInfo(std::string);
    std::uint32_t getCapacityMB();
    std::uint32_t getFreeSpaceMB();
    std::uint32_t getAvailSpaceMB();
    bool getIsWritable();
    bool getIsDeletable();
	std::string getLastModTime();
	int32_t getStatus();
};

class InternalCopy
{
private:
    std::string mSrcPath;
    std::string mDestPath;
    uint32_t mSrcSize;
    uint32_t mDestSize;
    int32_t mStatus;
	bool mOverwrite;
    void init();
public:
    InternalCopy(std::string, std::string, bool);
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
    uint32_t mSrcSize;
    uint32_t mDestSize;
    int32_t mStatus;
	bool mOverwrite;
    void init();
public:
    InternalMove(std::string, std::string, bool);
    int32_t getStatus();
};

class InternalRename
{
private:
	std::string mOldAbsPath;
	std::string mNewAbsPath;
	int32_t mStatus;
	void init();
public:
	InternalRename(std::string, std::string);
	int32_t getStatus();
};

class InternalCreateDir
{
private:
	std::string mPath;
	int32_t mStatus;

	void init();
public:
	InternalCreateDir(std::string);
	int32_t getStatus();
};

class InternalOperationHandler
{
private:
    InternalOperationHandler();
public:
    static InternalOperationHandler& getInstance();
    std::unique_ptr<FolderContents> getListFolderContents(std::string);
    std::unique_ptr<InternalSpaceInfo> getProperties(std::string path = std::string());
    std::unique_ptr<InternalCopy> copy(std::string, std::string, bool);
    std::unique_ptr<InternalRemove> remove(std::string);
    std::unique_ptr<InternalMove> move(std::string, std::string, bool);
	std::unique_ptr<InternalRename> rename(std::string, std::string);
};


#endif /*_INTERNAL_OPERATION_HANDLER_H_*/
