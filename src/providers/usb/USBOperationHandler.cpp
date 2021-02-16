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
#include <fstream>
#include <bitset>
#include <iostream>
#include <filesystem>
#include "USBOperationHandler.h"
#include "SAFLog.h"
#include <chrono>
#include <iomanip>
#include <fstream>

namespace fs = std::filesystem;

bool validateUSBPath(std::string& path)
{
    bool retVal = true;
    try
    {
        if (path.empty() || (path.find("/") != 0)
            || (path[path.size() - 1] == '/') || !fs::exists(path))
        {
            LOG_DEBUG_SAF("%s: invalid path [%s]", __FUNCTION__, path.c_str());
            retVal = false;
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        retVal = false;
    }
    return retVal;
}

int getUSBErrorCode(int errorCode)
{
    static std::map<int, int> convMap = {
        {USBOperErrors::NO_ERROR,              SAFErrors::NO_ERROR},
        {USBOperErrors::UNKNOWN,               SAFErrors::UNKNOWN_ERROR},
        {USBOperErrors::INVALID_PATH,          SAFErrors::INVALID_PATH},
        {USBOperErrors::INVALID_SOURCE_PATH,   SAFErrors::INVALID_SOURCE_PATH},
        {USBOperErrors::INVALID_DEST_PATH,     SAFErrors::INVALID_DEST_PATH},
        {USBOperErrors::FILE_ALREADY_EXISTS,   SAFErrors::FILE_ALREADY_EXISTS},
        {USBOperErrors::PERMISSION_DENIED,     SAFErrors::PERMISSION_DENIED},
        {USBOperErrors::SUCCESS,               SAFErrors::NO_ERROR},
    };

    int retCode = SAFErrors::UNKNOWN_ERROR;
    if (convMap.find(errorCode) != convMap.end())
        retCode = convMap[errorCode];
    return retCode;
}

USBFolderContent::USBFolderContent(std::string absPath) : mPath(absPath)
{
    init();
}

void USBFolderContent::init()
{
    if (mPath.empty())  return;
    mName = mPath.substr(mPath.rfind("/")+1);
    mType = getFileType(mPath);
    mSize = getFileSize(mPath);
    mModTime = getModTime();
}

std::string USBFolderContent::getFileType(std::string filePath)
{
    std::string type = "UNKNOWN";
    try
    {
        if (!validateUSBPath(filePath)) type = "UNKNOWN";
        else if (fs::is_regular_file(filePath)) type = "REGULAR";
        else if (fs::is_symlink(filePath)) type = "LINKFILE";
        else if (fs::is_directory(filePath)) type = "DIRECTORY";
#if 0
        if (!fs::exists(filePath)) type = "unavailable";
        else if (fs::is_block_file(filePath)) type = "block";
        else if (fs::is_character_file(filePath)) type = "character";
        else if (fs::is_fifo(filePath)) type = "fifo";
        else if (fs::is_other(filePath)) type = "other";
        else if (fs::is_socket(filePath)) type = "socket";
        else if (fs::is_empty(filePath)) type = "empty";
#endif
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        type = "UNKNOWN";
    }
    return type;
}

uint32_t USBFolderContent::getFileSize(std::string filePath)
{
    std::uint32_t size = 0;
    try
    {
        if (!validateUSBPath(filePath))
            size = 0;
        else if (fs::is_regular_file(filePath))
            size = fs::file_size(filePath);
        else if(fs::is_directory(filePath))
        {
            const std::filesystem::directory_options options = (
                fs::directory_options::follow_directory_symlink |
                fs::directory_options::skip_permission_denied);
            for (const auto & entry : fs::directory_iterator(filePath, fs::directory_options(options)))
            {
                std::string entryPath = entry.path();
                if (entryPath.find("/.") == std::string::npos)
                    size += getFileSize(entryPath);
            }
        }
        else
            size = 0;
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        size = 0;
    }
    return size;
}

std::string USBFolderContent::getModTime()
{
    std::string timeStamp;
    try
    {
        if (validateUSBPath(mPath))
        {
            using namespace std::chrono_literals;
            auto ftime = fs::last_write_time(mPath);
            auto timeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(ftime);
            timeStamp = ctime((time_t*)&timeMs);
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        timeStamp.clear();
    }
    return timeStamp;
}

USBFolderContents::USBFolderContents(std::string fullPath) : mFullPath(fullPath), mStatus(NO_ERROR)
{
    init();
}

void USBFolderContents::init()
{
    try
    {
        if (!validateUSBPath(mFullPath))
        {
            mStatus = INVALID_PATH;
            return;
        }
        mContents.clear();
        const std::filesystem::directory_options options = (
            fs::directory_options::follow_directory_symlink |
            fs::directory_options::skip_permission_denied);
        for (const auto & entry : fs::directory_iterator(mFullPath, fs::directory_options(options)))
        {
            std::string entryPath = entry.path();
            if (entryPath.find("/.") == std::string::npos)
            {
                std::shared_ptr<USBFolderContent> folderObj
                    = std::shared_ptr<USBFolderContent>(new USBFolderContent(entryPath));
                mContents.push_back(folderObj);
                //LOG_DEBUG_SAF("%s: Path: %s, Total: %d", __FUNCTION__, entry.path().c_str(), mContents.size());
            }
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mTotalCount = 0;
        mStatus = INVALID_PATH;
        this->mErrMsg = e.what();
    }
    mTotalCount = mContents.size();
}

USBCopy::USBCopy(std::string src, std::string dest, bool overwrite)
    : mSrcPath(src), mDestPath(dest), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void USBCopy::init()
{
    try
    {
        if (!validateUSBPath(mSrcPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (!validateUSBPath(mDestPath))
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
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
        return;
    }
     mSrcSize = USBFolderContent(mSrcPath).getSize();
     mDestSize = USBFolderContent(mDestPath).getSize();
     std::async(std::launch::async, [this]()
         {
            try
            {
                 auto copyOptions = fs::copy_options::recursive
                            | fs::copy_options::skip_symlinks;
                 if (this->mOverwrite)
                     copyOptions |= fs::copy_options::overwrite_existing;
                 else
                     copyOptions |= fs::copy_options::skip_existing;
                 fs::copy(this->mSrcPath, this->mDestPath, copyOptions);
                 this->mStatus = SUCCESS;
             }
            catch(fs::filesystem_error& e)
            {
                LOG_DEBUG_SAF("%s", e.what());
                this->mStatus = PERMISSION_DENIED;
             }
         });
}

std::int32_t USBCopy::getStatus()
{
    if(mStatus < 0)
    {
        return mStatus;
    }
    uint32_t size = USBFolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

std::string USBCopy::getErrorMsg()
{ 
    return mErrMsg;
}

USBRemove::USBRemove(std::string path)
    : mPath(path), mStatus(NO_ERROR)
{
    init();
}

void USBRemove::init()
{
    try
    {
        if (validateUSBPath(mPath))
        {
            fs::remove_all(mPath);
            mStatus = SUCCESS;
        }
        else
            mStatus = INVALID_PATH;
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
    }
}

int32_t USBRemove::getStatus()
{
    return mStatus;
}

std::string USBRemove::getErrorMsg()
{
    return mErrMsg;
}

USBMove::USBMove(std::string srcPath, std::string destPath, bool overwrite)
    : mSrcPath(srcPath), mDestPath(destPath), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void USBMove::init()
{
    try
    {
        if (!validateUSBPath(mSrcPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (!validateUSBPath(mDestPath))
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
        mSrcSize = USBFolderContent(mSrcPath).getSize();
        mDestSize = USBFolderContent(mDestPath).getSize();
        std::async(std::launch::async, [this]()
            {
                try
                {
                    auto copyOptions = fs::copy_options::recursive
                               | fs::copy_options::skip_symlinks;
                    if (this->mOverwrite)
                        copyOptions |= fs::copy_options::overwrite_existing;
                    else
                        copyOptions |= fs::copy_options::skip_existing;
                    fs::copy(this->mSrcPath, this->mDestPath, copyOptions);
                    this->mStatus = SUCCESS;
                }
                catch(fs::filesystem_error& e)
                {
                    LOG_DEBUG_SAF("%s", e.what());
                    this->mStatus = PERMISSION_DENIED;
                }
            });
        fs::remove_all(mSrcPath);
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
    }
}

int32_t USBMove::getStatus()
{
    uint32_t size = USBFolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

std::string USBMove::getErrorMsg()
{
    return mErrMsg;
}

USBRename::USBRename(std::string oldAbsPath, std::string newAbsPath)
    : mOldAbsPath(oldAbsPath), mNewAbsPath(newAbsPath), mStatus(NO_ERROR)
{
    init();
}

void USBRename::init ()
{
    try
    {
        if (!validateUSBPath(mOldAbsPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (mNewAbsPath.empty() || (mNewAbsPath.find("/") != std::string::npos))
        {
            mStatus = INVALID_DEST_PATH;
            return;
        }
        mNewAbsPath = mOldAbsPath.substr(0, mOldAbsPath.rfind("/") + 1) + mNewAbsPath;
        fs::rename(mOldAbsPath, mNewAbsPath);
        mStatus = SUCCESS;
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
    }
}

int32_t USBRename::getStatus()
{
    return mStatus;
}

std::string USBRename::getErrorMsg()
{
    return mErrMsg;
}

USBSpaceInfo::USBSpaceInfo(std::string path) : mPath(path), mStatus(NO_ERROR)
{
    init();
}

void USBSpaceInfo::init()
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

std::uint32_t USBSpaceInfo::getCapacityMB()
{
    return mCapacity / 1000000;
}

std::uint32_t USBSpaceInfo::getFreeSpaceMB()
{
    return mFreeSpace / 1000000;
}

std::uint32_t USBSpaceInfo::getAvailSpaceMB()
{
    return mAvailSpace / 1000000;
}

bool USBSpaceInfo::getIsWritable()
{
    return mIsWritable;
}

bool USBSpaceInfo::getIsDeletable()
{
    return mIsDeletable;
}

int32_t USBSpaceInfo::getStatus()
{
    return mStatus;
}

std::string USBSpaceInfo::getLastModTime()
{
    std::string timeStamp;
    try
    {
        if (fs::exists(mPath))
        {
            using namespace std::chrono_literals;
            auto ftime = fs::last_write_time(mPath);
            auto timeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(ftime);
            timeStamp = ctime((time_t*)&timeMs);
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        timeStamp.clear();
    }
    return timeStamp;
}

USBOperationHandler::USBOperationHandler()
{
}

USBOperationHandler& USBOperationHandler::getInstance()
{
    static USBOperationHandler obj = USBOperationHandler();
    return obj;
}

std::unique_ptr<USBCopy> USBOperationHandler::copy(std::string srcPath, std::string destPath, bool overwrite)
{
    std::unique_ptr<USBCopy> obj = std::unique_ptr<USBCopy>(new USBCopy(srcPath, destPath, overwrite));
    return std::move(obj);
}

std::unique_ptr<USBRemove> USBOperationHandler::remove(std::string path)
{
   std::unique_ptr<USBRemove> obj = std::unique_ptr<USBRemove>(new USBRemove(path));
    return std::move(obj);
}

std::unique_ptr<USBMove> USBOperationHandler::move(std::string srcPath, std::string destPath, bool overwrite)
{
    std::unique_ptr<USBMove> obj = std::unique_ptr<USBMove>(new USBMove(srcPath, destPath, overwrite));
    return std::move(obj);
}

std::unique_ptr<USBFolderContents> USBOperationHandler::getListFolderContents(std::string path)
{
    std::unique_ptr<USBFolderContents> obj = std::unique_ptr<USBFolderContents>( new USBFolderContents(path));
    return std::move(obj);
}

std::unique_ptr<USBRename> USBOperationHandler::rename(std::string oldPath, std::string newPath)
{
    std::unique_ptr<USBRename> obj = std::unique_ptr<USBRename>(new USBRename(oldPath, newPath));
    return std::move(obj);
}

std::unique_ptr<USBSpaceInfo> USBOperationHandler::getProperties(std::string path)
{
    std::unique_ptr<USBSpaceInfo> obj = std::unique_ptr<USBSpaceInfo>( new USBSpaceInfo(path));
    return std::move(obj);
}
