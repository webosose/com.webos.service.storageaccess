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

FolderContents::FolderContents(std::string fullPath) : mFullPath(fullPath)
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
            FolderContent folderObj = FolderContent(entry.path());
            mContents.push_back(folderObj);
        }
    }
    catch(fs::filesystem_error& e) {
        mTotalCount = 0;
    }
    mTotalCount = mContents.size();
}

InternalSpaceInfo::InternalSpaceInfo(std::string path) : mPath(path)
{
    init();
}

void InternalSpaceInfo::init()
{
    fs::space_info dev = fs::space(mPath);
    mCapacity = dev.capacity;
    mFreeSpace = dev.free;
    mAvailSpace = dev.available;

    fs::perms ps = fs::status(mPath).permissions();
    mIsWritable = ((ps & fs::perms::owner_write) != fs::perms::none);
    mIsDeletable = ((ps & fs::perms::group_write) != fs::perms::none);
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

InternalCopy::InternalCopy(std::string src, std::string dest)
    : mSrcPath(src), mDestPath(dest), mStatus(0)
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
                const auto copyOptions = fs::copy_options::update_existing
                           | fs::copy_options::recursive
                           | fs::copy_options::skip_symlinks;
                fs::copy(this->mSrcPath, this->mDestPath, copyOptions); 
            }
            catch(fs::filesystem_error& e) {
                this->mStatus = -1;
            }
        });
}

std::uint32_t InternalCopy::getStatus()
{
    uint32_t size = FolderContent(mDestPath).getSize();
    uint32_t change = (size > mSrcSize)?(size - mSrcSize):(size - mDestSize);
    if (mStatus != -1 && mSrcSize)
        mStatus = int(change / mSrcSize * 100);
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
        fs::remove_all(mPath);
        mStatus = 100;
    }
    catch(fs::filesystem_error& e) {
        mStatus = -1;
    }
}

int32_t InternalRemove::getStatus()
{
    return mStatus;
}

InternalMove::InternalMove(std::string srcPath, std::string destPath)
    : mSrcPath(srcPath), mDestPath(destPath), mStatus(0)
{
    init();
}

void InternalMove::init()
{
    std::async(std::launch::async, [this]()
        {
            try {
                InternalCopy obj = InternalCopy(this->mSrcPath, this->mDestPath);
                this->mStatus = obj.getStatus();
                while (this->mStatus!= -1)
                {
                    if (this->mStatus >= 100)
                    {
                        InternalRemove rem = InternalRemove(this->mSrcPath);
                        this->mStatus = rem.getStatus();
                        break;
                    }
                    this->mStatus = obj.getStatus();
                }
            }
            catch(fs::filesystem_error& e) {
                this->mStatus = -1;
            }
        });
}

int32_t InternalMove::getStatus()
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

std::shared_ptr<FolderContents> InternalOperationHandler::getListFolderContents(std::string path)
{
    std::shared_ptr<FolderContents> obj = std::make_shared<FolderContents>(
path);
    dumpContents();
    return obj;
}

void InternalOperationHandler::dumpContents()
{
    std::cout << "FullPath: " << mFolderContentObj->getPath() << ", Total: " << 
mFolderContentObj->getTotalCount() << std::endl;
    for (auto content : mFolderContentObj->getContents())
    {
        std::cout << "FileName: " << content.getName() << ", Path: " << 
content.getPath()
            << ", Type: " << content.getType() << ", Size: " << content.
getSize() << std::endl;
    }
}

std::shared_ptr<InternalSpaceInfo> InternalOperationHandler::getProperties(std::string path)
{
    if (path.empty())
        path = "/tmp";
    mSpaceInfoObj = std::make_shared<InternalSpaceInfo>(path);
    return mSpaceInfoObj;
}

std::shared_ptr<InternalCopy> InternalOperationHandler::copy(std::string srcPath, std::string destPath)
{
    mCopyObj = std::make_shared<InternalCopy>(srcPath, destPath);
    return mCopyObj;
}

std::shared_ptr<InternalRemove> InternalOperationHandler::remove(std::string path)
{
    mRemoveObj = std::make_shared<InternalRemove>(path);
    return mRemoveObj;
}

std::shared_ptr<InternalMove> InternalOperationHandler::move(std::string srcPath, std::string destPath)
{
    mMoveObj = std::make_shared<InternalMove>(srcPath, destPath);
    return mMoveObj;
}

