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

int getInternalErrorCode(int errorCode)
{
    static std::map<int, int> convMap = {
        {InternalOperErrors::NO_ERROR, SAFErrors::InternalErrors::InternalErrorEnum::NO_ERROR},
        {InternalOperErrors::UNKNOWN, SAFErrors::InternalErrors::InternalErrorEnum::UNKNOWN_ERROR},
        {InternalOperErrors::INVALID_PATH, SAFErrors::InternalErrors::InternalErrorEnum::INVALID_PATH},
        {InternalOperErrors::INVALID_SOURCE_PATH, SAFErrors::InternalErrors::InternalErrorEnum::INVALID_SOURCE_PATH},
        {InternalOperErrors::INVALID_DEST_PATH, SAFErrors::InternalErrors::InternalErrorEnum::INVALID_DEST_PATH},
        {InternalOperErrors::FILE_ALREADY_EXISTS, SAFErrors::InternalErrors::InternalErrorEnum::FILE_ALREADY_EXISTS},
        {InternalOperErrors::SUCCESS, SAFErrors::InternalErrors::InternalErrorEnum::NO_ERROR},
    };
    int retCode = SAFErrors::InternalErrors::InternalErrorEnum::UNKNOWN_ERROR;
    if (convMap.find(errorCode) != convMap.end())
        retCode = convMap[errorCode];
    return retCode;
}

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

FolderContents::FolderContents(std::string fullPath) : mFullPath(fullPath), mStatus(NO_ERROR)
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
        mStatus = INVALID_PATH;
    }
    mTotalCount = mContents.size();
}

InternalSpaceInfo::InternalSpaceInfo(std::string path) : mPath(path), mStatus(NO_ERROR)
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
        mStatus = SUCCESS;
    }
    catch(fs::filesystem_error& e) {
        mStatus = INVALID_PATH;
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
    : mSrcPath(src), mDestPath(dest), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void InternalCopy::init()
{
    if (!fs::exists(mSrcPath))
    {
        mStatus = INVALID_SOURCE_PATH;
        return;
    }
    if (!fs::exists(mDestPath))
    {
        mStatus = INVALID_DEST_PATH;
        return;
    }
    std::string desPath = mDestPath + mSrcPath.substr(mSrcPath.rfind("/"));
    if (fs::exists(desPath) & !mOverwrite)
    {
        mStatus = FILE_ALREADY_EXISTS;
        return;
    }
    if (fs::is_directory(mSrcPath))
    {
        if (!fs::exists(desPath))
            fs::create_directories(desPath);
        mDestPath = desPath;
    }
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
                this->mStatus = SUCCESS;
            }
            catch(fs::filesystem_error& e) {
                this->mStatus = INVALID_DEST_PATH;
            }
        });
}

std::uint32_t InternalCopy::getStatus()
{
    uint32_t size = FolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

InternalRemove::InternalRemove(std::string path)
    : mPath(path), mStatus(NO_ERROR)
{
    init();
}

void InternalRemove::init()
{
    try {
        if (fs::exists(mPath))
        {
            fs::remove_all(mPath);
            mStatus = SUCCESS;
        }
        else
            mStatus = INVALID_PATH;
    }
    catch(fs::filesystem_error& e) {
        mStatus = INVALID_PATH;
    }
}

int32_t InternalRemove::getStatus()
{
    return mStatus;
}

InternalMove::InternalMove(std::string srcPath, std::string destPath, bool overwrite)
    : mSrcPath(srcPath), mDestPath(destPath), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void InternalMove::init()
{
    try {
        if (!fs::exists(mSrcPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (!fs::exists(mDestPath))
        {
            mStatus = INVALID_DEST_PATH;
            return;
        }
        std::string desPath = mDestPath + mSrcPath.substr(mSrcPath.rfind("/"));
        if (fs::exists(desPath) & !mOverwrite)
        {
            mStatus = FILE_ALREADY_EXISTS;
            return;
        }
        if (fs::is_directory(mSrcPath))
        {
            if (!fs::exists(desPath))
                fs::create_directories(desPath);
            mDestPath = desPath;
        }
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
                    this->mStatus = SUCCESS;
                }
                catch(fs::filesystem_error& e) {
                    this->mStatus = INVALID_DEST_PATH;
                }
            });
        fs::remove_all(mSrcPath);
    }
    catch(fs::filesystem_error& e) {
        mStatus = UNKNOWN;
    }
}

int32_t InternalMove::getStatus()
{
    uint32_t size = FolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

InternalRename::InternalRename(std::string oldAbsPath, std::string newAbsPath)
    : mOldAbsPath(oldAbsPath), mNewAbsPath(newAbsPath), mStatus(NO_ERROR)
{
    init();
}

void InternalRename::init ()
{
    try
    {
        if (!fs::exists(mOldAbsPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        fs::rename(mOldAbsPath, mNewAbsPath);
        mStatus = SUCCESS;
    }
    catch(fs::filesystem_error& e) {
        mStatus = INVALID_DEST_PATH;
    }
}

int32_t InternalRename::getStatus()
{
    return mStatus;
}

InternalCreateDir::InternalCreateDir(std::string path) : mPath(path), mStatus(NO_ERROR)
{
    init();
}

void InternalCreateDir::init ()
{
    try {
        fs::create_directories(mPath);
        mStatus = SUCCESS;
    }
    catch(fs::filesystem_error& e) {
        mStatus = UNKNOWN;
    }
}

int32_t InternalCreateDir::getStatus()
{
    return mStatus;
}

InternalOperationHandler::InternalOperationHandler()
{
    if (!fs::exists("/tmp/internal"))
        fs::create_directories("/tmp/internal");
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

