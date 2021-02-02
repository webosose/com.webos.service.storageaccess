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
namespace fs = std::filesystem;

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
}

std::string USBFolderContent::getFileType(std::string filePath)
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

uint32_t USBFolderContent::getFileSize(std::string filePath)
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

USBFolderContents::USBFolderContents(std::string fullPath) : mFullPath(fullPath), mStatus(0)
{
    init();
}

void USBFolderContents::init()
{
    if (mFullPath.empty())  return;
    mContents.clear();
    try {
        const std::filesystem::directory_options options = (
            fs::directory_options::follow_directory_symlink |
            fs::directory_options::skip_permission_denied);
        for (const auto & entry : fs::recursive_directory_iterator(mFullPath, fs::directory_options(options)))
        {
            std::shared_ptr<USBFolderContent> folderObj = std::shared_ptr<USBFolderContent>(new USBFolderContent(entry.path()));
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

USBCopy::USBCopy(std::string src, std::string dest, bool overwrite)
    : mSrcPath(src), mDestPath(dest), mStatus(0), mOverwrite(overwrite)
{
    init();
}

void USBCopy::init()
{
    mSrcSize  = USBFolderContent(mSrcPath).getSize();
    mDestSize = USBFolderContent(mDestPath).getSize();
    std::async(std::launch::async, [this]()
    {
        try {
            auto copyOptions = fs::copy_options::recursive | fs::copy_options::skip_symlinks;
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

std::uint32_t USBCopy::getStatus()
{
    uint32_t size = USBFolderContent(mDestPath).getSize();
    uint32_t change = (mSrcSize - size);
    if ((mStatus != -1) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus != -1) && (change == 0))
        mStatus = 100;
    else
        mStatus = -1;
    return mStatus;
}

std::string USBCopy::getErrorMsg()
{ 
    return mErrMsg;
}

USBRemove::USBRemove(std::string path)
    : mPath(path), mStatus(0)
{
    init();
}

void USBRemove::init()
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
        this->mErrMsg = e.what();
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
    : mSrcPath(srcPath), mDestPath(destPath), mStatus(0), mOverwrite(overwrite)
{
    init();
}

void USBMove::init()
{
    USBRename obj = USBRename(mSrcPath, mDestPath);
    mStatus = obj.getStatus();
    if ((mStatus == -1) && mOverwrite)
    {
        USBRemove remObj = USBRemove(mDestPath);
        mStatus = remObj.getStatus();
        USBRename retryObj = USBRename(mSrcPath, mDestPath);
        mStatus = retryObj.getStatus();
    }
}

int32_t USBMove::getStatus()
{
    return mStatus;
}

std::string USBMove::getErrorMsg()
{
    return mErrMsg;
}

USBRename::USBRename(std::string oldAbsPath, std::string newAbsPath)
    : mOldAbsPath(oldAbsPath), mNewAbsPath(newAbsPath), mStatus(0)
{
    init();
}

void USBRename::init ()
{
    try {
        fs::rename(mOldAbsPath, mNewAbsPath);
        mStatus = 100;
    }
    catch(fs::filesystem_error& e) {
        mStatus = -1;
    }
}

int32_t USBRename::getStatus()
{
    return mStatus;
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
