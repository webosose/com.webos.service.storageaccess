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

#include <functional>
#include <future>
#include "InternalOperationHandler.h"
#include <fstream>
#include <bitset>
#include <iostream>
#include <filesystem>
#include "SAFLog.h"

namespace fs = std::filesystem;

FolderContent::FolderContent(std::string absPath) : mPath(absPath)
{
    init();
}

void FolderContent::init()
{
    if (mPath.empty())  return;
    mName = mPath.substr(mPath.rfind("/")+1);
    mType = getFileType(mPath);
    mSize = getFileSize(mPath);
}

std::string FolderContent::getFileType(std::string filePath)
{
    std::string type = "unknown";
    if (!fs::exists(filePath)) type = "unavailable";
    else if (fs::is_regular_file(filePath)) type = "regular";
    else if (fs::is_block_file(filePath)) type = "block";
    else if (fs::is_character_file(filePath)) type = "character";
    else if (fs::is_fifo(filePath)) type = "fifo";
    else if (fs::is_other(filePath)) type = "other";
    else if (fs::is_socket(filePath)) type = "socket";
    else if (fs::is_symlink(filePath)) type = "symlink";
    else if (fs::is_empty(filePath)) type = "empty";
    else if (fs::is_directory(filePath)) type = "directory";
    return type;
}

uint32_t FolderContent::getFileSize(std::string filePath)
{
    std::uint32_t size = 0;
    try {
        if (fs::is_regular_file(filePath))
            size = fs::file_size(filePath);
        else if(fs::is_directory(filePath))
        {
            try {
                const std::filesystem::directory_options options = (
                    fs::directory_options::follow_directory_symlink |
                    fs::directory_options::skip_permission_denied);
                for (const auto & entry : fs::recursive_directory_iterator(filePath, fs::directory_options(options)))
                {
                    size += getFileSize(entry.path());
                }
            }
            catch(fs::filesystem_error& e) {
                size = 0;
            }
        }
    }
    catch(fs::filesystem_error& e) {
        size = 0;
    }
    return size;
}

FolderContents::FolderContents(std::string fullPath) : mFullPath(fullPath), mStatus(0)
{
    init();
}

void FolderContents::init()
{
    if (mFullPath.empty())  return;
    mContents.clear();
    try {
        const std::filesystem::directory_options options = (
            fs::directory_options::follow_directory_symlink |
            fs::directory_options::skip_permission_denied);
        for (const auto & entry : fs::recursive_directory_iterator(mFullPath, fs::directory_options(options)))
        {
            std::shared_ptr<FolderContent> folderObj = std::shared_ptr<FolderContent>(new FolderContent(entry.path()));
            mContents.push_back(folderObj);
            //LOG_DEBUG_SAF("%s: Path: %s, Total: %d", __FUNCTION__, entry.path().c_str(), mContents.size());
        }
    }
    catch(fs::filesystem_error& e) {
        mTotalCount = 0;
        mStatus = -1;
    }
    mTotalCount = mContents.size();
}

InternalSpaceInfo::InternalSpaceInfo(std::string path) : mPath(path), mStatus(0)
{
    init();
}

void InternalSpaceInfo::init()
{
    try{
        fs::space_info dev = fs::space(mPath);
        mCapacity = dev.capacity;
        mFreeSpace = dev.free;
        mAvailSpace = dev.available;

        fs::perms ps = fs::status(mPath).permissions();
        mIsWritable = ((ps & fs::perms::owner_write) != fs::perms::none);
        mIsDeletable = ((ps & fs::perms::group_write) != fs::perms::none);
        mStatus = 100;
    }
    catch(fs::filesystem_error& e) {
        mStatus = -1;
    }
}

std::uint32_t InternalSpaceInfo::getCapacityMB()
{
    return mCapacity / 1000000;
}

std::uint32_t InternalSpaceInfo::getFreeSpaceMB()
{
    return mFreeSpace / 1000000;
}

std::uint32_t InternalSpaceInfo::getAvailSpaceMB()
{
    return mAvailSpace / 1000000;
}

bool InternalSpaceInfo::getIsWritable()
{
    return mIsWritable;
}

bool InternalSpaceInfo::getIsDeletable()
{
    return mIsDeletable;
}

int32_t InternalSpaceInfo::getStatus()
{
    return mStatus;
}


InternalCopy::InternalCopy(std::string src, std::string dest, bool overwrite)
    : mSrcPath(src), mDestPath(dest), mStatus(0), mOverwrite(overwrite)
{
    init();
}

void InternalCopy::init()
{
    mSrcSize = FolderContent(mSrcPath).getSize();
    mDestSize = FolderContent(mDestPath).getSize();
    std::async(std::launch::async, [this]()
        {
            try {
                auto copyOptions = fs::copy_options::recursive
                           | fs::copy_options::skip_symlinks;
                if (this->mOverwrite)
                    copyOptions |= fs::copy_options::overwrite_existing;
                else
                    copyOptions |= fs::copy_options::skip_existing;
                fs::copy(this->mSrcPath, this->mDestPath, copyOptions);
                this->mStatus = 100;
            }
            catch(fs::filesystem_error& e) {
                this->mStatus = -1;
            }
        });
}

std::uint32_t InternalCopy::getStatus()
{
    uint32_t size = FolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size);
    if ((mStatus != -1) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus != -1) && (change == 0))
        mStatus = 100;
    else
        mStatus = -1;
    return mStatus;
}

InternalRemove::InternalRemove(std::string path)
    : mPath(path), mStatus(0)
{
    init();
}

void InternalRemove::init()
{
    try {
        if (fs::exists(mPath))
        {
            fs::remove_all(mPath);
            mStatus = 100;
        }
        else
            mStatus = -1;
    }
    catch(fs::filesystem_error& e) {
        mStatus = -1;
    }
}

int32_t InternalRemove::getStatus()
{
    return mStatus;
}

InternalMove::InternalMove(std::string srcPath, std::string destPath, bool overwrite)
    : mSrcPath(srcPath), mDestPath(destPath), mStatus(0), mOverwrite(overwrite)
{
    init();
}

void InternalMove::init()
{
    InternalRename obj = InternalRename(mSrcPath, mDestPath);
    mStatus = obj.getStatus();
    if ((mStatus == -1) && mOverwrite)
    {
        InternalRemove remObj = InternalRemove(mDestPath);
        mStatus = remObj.getStatus();
        InternalRename retryObj = InternalRename(mSrcPath, mDestPath);
        mStatus = retryObj.getStatus();
    }
}

int32_t InternalMove::getStatus()
{
    return mStatus;
}

InternalRename::InternalRename(std::string oldAbsPath, std::string newAbsPath)
    : mOldAbsPath(oldAbsPath), mNewAbsPath(newAbsPath), mStatus(0)
{
    init();
}

void InternalRename::init ()
{
    try {
        fs::rename(mOldAbsPath, mNewAbsPath);
        mStatus = 100;
    }
    catch(fs::filesystem_error& e) {
        mStatus = -1;
    }
}

int32_t InternalRename::getStatus()
{
    return mStatus;
}

InternalOperationHandler::InternalOperationHandler()
{
}

InternalOperationHandler& InternalOperationHandler::getInstance()
{
    static InternalOperationHandler obj = InternalOperationHandler();
    return obj;
}

std::unique_ptr<FolderContents> InternalOperationHandler::getListFolderContents(std::string path)
{
    std::unique_ptr<FolderContents> obj = std::unique_ptr<FolderContents>( new FolderContents(path));
    return std::move(obj);
}

std::unique_ptr<InternalSpaceInfo> InternalOperationHandler::getProperties(std::string path)
{
    if (path.empty())   path = "/tmp";
    std::unique_ptr<InternalSpaceInfo> obj = std::unique_ptr<InternalSpaceInfo>( new InternalSpaceInfo(path));
    return std::move(obj);
}

std::unique_ptr<InternalCopy> InternalOperationHandler::copy(std::string srcPath,
    std::string destPath, bool overwrite)
{
    std::unique_ptr<InternalCopy> obj = std::unique_ptr<InternalCopy>(new InternalCopy(srcPath, destPath, overwrite));
    return std::move(obj);
}

std::unique_ptr<InternalRemove> InternalOperationHandler::remove(std::string path)
{
    std::unique_ptr<InternalRemove> obj = std::unique_ptr<InternalRemove>(new InternalRemove(path));
    return std::move(obj);
}

std::unique_ptr<InternalMove> InternalOperationHandler::move(std::string srcPath, std::string destPath, bool overwrite)
{
    std::unique_ptr<InternalMove> obj = std::unique_ptr<InternalMove>(new InternalMove(srcPath, destPath, overwrite));
    return std::move(obj);
}

std::unique_ptr<InternalRename> InternalOperationHandler::rename(std::string srcPath, std::string destPath)
{
    std::unique_ptr<InternalRename> obj = std::unique_ptr<InternalRename>(new InternalRename(srcPath, destPath));
    return std::move(obj);
}

