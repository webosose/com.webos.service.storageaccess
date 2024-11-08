/* @@@LICENSE
 *
 * Copyright (c) 2021-2024 LG Electronics, Inc.
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
#include "SAFLog.h"
#include <chrono>
#include <iomanip>
#include <fstream>
#include "SAFUtilityOperation.h"

namespace fs = std::filesystem;

bool validateInternalPath(std::string& path)
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
bool SAFUtilityOperation::validateInternalPath(std::string& path, std::string& sessionId)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    if ((path.find("/media") == 0) || (path.find("/tmp") == 0) || (path.find("/home/"+sessionId) == 0))
    {
        std::error_code ec;
        if(fs::exists(path, ec))
            return true;
        LOG_DEBUG_SAF("%s, errorCode: %d", __FUNCTION__, ec.value());
    }
    return false;
}

bool SAFUtilityOperation::validateSambaPath(std::string& path,std::string& driveId)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    std::error_code ec;
    if((mSambaDrivePathMap.find(driveId) != mSambaDrivePathMap.end()) && (path.find(mSambaDrivePathMap[driveId]) == 0)
        && fs::exists(path, ec))
        return true;
    LOG_DEBUG_SAF("%s, errorCode: %d", __FUNCTION__, ec.value());
    return false;
}

void SAFUtilityOperation::setDriveDetails(const std::string& type, std::map<std::string,std::string>& inputMap)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    if(type == "SAMBA")
    {
        mSambaDrivePathMap.clear();
        for(auto & entry : inputMap)
            mSambaDrivePathMap[entry.first] = entry.second;
    }

}

bool SAFUtilityOperation::validateInterProviderOperation(std::shared_ptr<RequestData> reqData)
{
    LOG_DEBUG_SAF("Entering function %s", __FUNCTION__);
    bool result = true;
    std::string srcDriveId = reqData->params["srcDriveId"].asString();
    std::string destDriveId = reqData->params["destDriveId"].asString();
    std::string srcPath = reqData->params["srcPath"].asString();
    std::string destPath = reqData->params["destPath"].asString();
    std::string srcStorageType = reqData->params["srcStorageType"].asString();
    std::string destStorageType = reqData->params["destStorageType"].asString();
    std::string sessionId = reqData->sessionId;
    pbnjson::JValue respObj = pbnjson::Object();
    if((destStorageType == "network") && (destDriveId.find("UPNP") != std::string::npos))
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::STORAGE_TYPE_NOT_SUPPORTED);
        respObj.put("errorText", "Operation not Permitted");
        reqData->cb(respObj, reqData->subs);
        result = false;
    }
    if((destStorageType == "network") && (!validateSambaPath(destPath, destDriveId)))
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::PERMISSION_DENIED);
        respObj.put("errorText", "No such file or directory at destination");
        reqData->cb(std::move(respObj), reqData->subs);
        result = false;
    }
    else if((destStorageType == "internal") && (!validateInternalPath(destPath,sessionId)))
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::PERMISSION_DENIED);
        respObj.put("errorText", "No such file or directory at destination");
        reqData->cb(std::move(respObj), reqData->subs);
        result = false;
    }
    else if((srcStorageType == "network") && (!validateSambaPath(srcPath, srcDriveId)))
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
        respObj.put("errorText", "No such file or directory at source");
        reqData->cb(std::move(respObj), reqData->subs);
        result = false;
    }
    else if((srcStorageType == "internal") && (!validateInternalPath(srcPath,sessionId)))
    {
        respObj.put("returnValue", false);
        respObj.put("errorCode", SAFErrors::INVALID_SOURCE_PATH);
        respObj.put("errorText", "No such file or directory at source");
        reqData->cb(std::move(respObj), reqData->subs);
        result = false;
    }
    return result;
}

std::string SAFUtilityOperation::getInternalPath(std::string sessionId)
{
    LOG_DEBUG_SAF("Exiting function %s", __FUNCTION__);
    std::string path = "/safInternal";
    if (sessionId.find("root") != std::string::npos)
        path = "/home/" + sessionId + path;
    else
        path = "/home/" + sessionId + "/rootfs" + path;
    LOG_DEBUG_SAF("%s: file %s initiaized", __FUNCTION__, path.c_str());
    std::error_code ec;
    if (!fs::exists(path, ec))
    {
        LOG_DEBUG_SAF("%s: Before creating file: %s", __FUNCTION__, path.c_str());
        try {
            fs::create_directories(path);
        }
        catch(fs::filesystem_error& e) {
            LOG_DEBUG_SAF("%s: Error in creating dir : %s", __FUNCTION__, e.what());
            path.clear();
            return path;
        }
        std::string cmd = "chown " + sessionId + ": " + path;
        LOG_DEBUG_SAF("%s: Before exe cmd: %s", __FUNCTION__, cmd.c_str());
        FILE *fp = popen(cmd.data(), "r");
        if (fp != NULL){
            LOG_DEBUG_SAF("%s: cmd: %s Success", __FUNCTION__, cmd.c_str());
            pclose(fp);
            fp = NULL;
        }
        else
            LOG_DEBUG_SAF("%s: cmd: %s Fail", __FUNCTION__, cmd.c_str());
        cmd = "chmod 700 " + path;
        LOG_DEBUG_SAF("%s: Before exe cmd: %s", __FUNCTION__, cmd.c_str());
        fp = popen(cmd.data(), "r");
        if (fp != NULL)
        {
            LOG_DEBUG_SAF("%s: cmd: %s Success", __FUNCTION__, cmd.c_str());
            pclose(fp);
            fp = NULL;
        }
        else
            LOG_DEBUG_SAF("%s: cmd: %s Fail", __FUNCTION__, cmd.c_str());
    }
    else
        LOG_DEBUG_SAF("%s: file: %s already exists", __FUNCTION__, path.c_str());
    LOG_DEBUG_SAF("%s, errorCode: %d", __FUNCTION__, ec.value());
    return path;
}

void SAFUtilityOperation::setPathPerm(std::string path, std::string sessionId)
{
    std::error_code ec;
    if (!fs::exists(path, ec))
    {
        LOG_DEBUG_SAF("%s, errorCode: %d", __FUNCTION__, ec.value());
        return;
    }
    std::string cmd = "chown " + sessionId + ": " + path;
    FILE *fp = popen(cmd.data(), "r");
    if (fp != NULL){
        LOG_DEBUG_SAF("%s: cmd: %s Success", __FUNCTION__, cmd.c_str());
        pclose(fp);
        fp = NULL;
    }
    else
        LOG_DEBUG_SAF("%s: cmd: %s Fail", __FUNCTION__, cmd.c_str());
    cmd = "chmod 700 " + path;
    fp = popen(cmd.data(), "r");
    if (fp != NULL)
    {
        LOG_DEBUG_SAF("%s: cmd: %s Success", __FUNCTION__, cmd.c_str());
        pclose(fp);
        fp = NULL;
    }
    else
        LOG_DEBUG_SAF("%s: cmd: %s Fail", __FUNCTION__, cmd.c_str());
}

int getInternalErrorCode(int errorCode)
{
    static std::map<int, int> convMap = {
        {InternalOperErrors::NO_ERROR, SAFErrors::NO_ERROR},
        {InternalOperErrors::UNKNOWN, SAFErrors::UNKNOWN_ERROR},
        {InternalOperErrors::INVALID_PATH, SAFErrors::INVALID_PATH},
        {InternalOperErrors::INVALID_SOURCE_PATH, SAFErrors::INVALID_SOURCE_PATH},
        {InternalOperErrors::INVALID_DEST_PATH, SAFErrors::INVALID_DEST_PATH},
        {InternalOperErrors::FILE_ALREADY_EXISTS, SAFErrors::FILE_ALREADY_EXISTS},
        {InternalOperErrors::PERMISSION_DENIED,     SAFErrors::PERMISSION_DENIED},
        {InternalOperErrors::SUCCESS, SAFErrors::NO_ERROR}
    };
    int retCode = SAFErrors::UNKNOWN_ERROR;
    if (convMap.find(errorCode) != convMap.end())
        retCode = convMap[errorCode];
    return retCode;
}

FolderContent::FolderContent(std::string absPath) : mPath(std::move(absPath))
{
    init();
}

void FolderContent::init()
{
    if (mPath.empty())  return;
    mName = mPath.substr(mPath.rfind("/")+1);
    mType = getFileType(mPath);
    mSize = getFileSize(mPath);
    mModTime = getModTime();
}

std::string FolderContent::getFileType(std::string filePath)
{
    std::string type = "unknown";
    try
    {
        if (!validateInternalPath(filePath)) type = "unknown";
        else if (fs::is_regular_file(filePath)) type = "regular";
        else if (fs::is_symlink(filePath)) type = "linkfile";
        else if (fs::is_directory(filePath)) type = "directory";
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
        type = "unknown";
    }
    return type;
}

uintmax_t FolderContent::getFileSize(std::string filePath)
{
    std::uintmax_t size = 0;
    try
    {
        if (!validateInternalPath(filePath))
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
                    size += getFileSize(std::move(entryPath));
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

std::string FolderContent::getModTime()
{
    std::string timeStamp;
    try
    {
        if (validateInternalPath(mPath))
        {
            using namespace std::chrono_literals;
            auto ftime = fs::last_write_time(mPath);
            auto timeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(ftime);
            char *sTimeMs = ctime((time_t*)&timeMs);
            if (sTimeMs)
                timeStamp = sTimeMs;
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        timeStamp.clear();
    }
    return timeStamp;
}

FolderContents::FolderContents(std::string fullPath) : mFullPath(std::move(fullPath)), mStatus(NO_ERROR)
{
    init();
}

void FolderContents::init()
{
    try
    {
        if (!validateInternalPath(mFullPath))
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
                std::shared_ptr<FolderContent> folderObj = std::shared_ptr<FolderContent>(new FolderContent(std::move(entryPath)));
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
    }
    mTotalCount = mContents.size();
}

InternalSpaceInfo::InternalSpaceInfo(std::string path) : mPath(std::move(path)), mStatus(NO_ERROR)
{
    init();
}

void InternalSpaceInfo::init()
{
    try
    {
        if (!validateInternalPath(mPath))
        {
            mStatus = INVALID_PATH;
            return;
        }
        fs::space_info dev = fs::space(mPath);
        mCapacity = dev.capacity;
        mFreeSpace = dev.free;
        mAvailSpace = dev.available;
        fs::perms ps = fs::status(mPath).permissions();
        mIsWritable = ((ps & fs::perms::owner_write) != fs::perms::none);
        mIsDeletable = ((ps & fs::perms::group_write) != fs::perms::none);
        mStatus = SUCCESS;
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
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

std::string InternalSpaceInfo::getLastModTime()
{
    std::string timeStamp;
    try
    {
        if (validateInternalPath(mPath))
        {
            using namespace std::chrono_literals;
            auto ftime = fs::last_write_time(mPath);
            auto timeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(ftime);
            char *sTimeMs = ctime((time_t*)&timeMs);
            if (sTimeMs)
                timeStamp = sTimeMs;
        }
        else
            mStatus = INVALID_PATH;
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        timeStamp.clear();
    }
    return timeStamp;
}

int32_t InternalSpaceInfo::getStatus()
{
    return mStatus;
}


InternalCopy::InternalCopy(std::string src, std::string dest, bool overwrite)
    : mSrcPath(std::move(src)), mDestPath(std::move(dest)), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void InternalCopy::init()
{
    try
    {
        if (!validateInternalPath(mSrcPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (!validateInternalPath(mDestPath))
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
            mDestPath = std::move(desPath);
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
        return;
    }
    mSrcSize = FolderContent(mSrcPath).getSize();
    mDestSize = FolderContent(mDestPath).getSize();
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

std::int32_t InternalCopy::getStatus()
{
    if(mStatus < 0)
    {
        return mStatus;
    }
    uint32_t size = FolderContent(mDestPath).getSize();
    mDestSize = size;
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

InternalRemove::InternalRemove(std::string path)
    : mPath(std::move(path)), mStatus(NO_ERROR)
{
    init();
}

void InternalRemove::init()
{
    try
    {
        if (validateInternalPath(mPath))
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

int32_t InternalRemove::getStatus()
{
    return mStatus;
}

InternalMove::InternalMove(std::string srcPath, std::string destPath, bool overwrite)
    : mSrcPath(std::move(srcPath)), mDestPath(std::move(destPath)), mStatus(NO_ERROR), mOverwrite(overwrite)
{
    init();
}

void InternalMove::init()
{
    try
    {
        if (!validateInternalPath(mSrcPath))
        {
            mStatus = INVALID_SOURCE_PATH;
            return;
        }
        if (!validateInternalPath(mDestPath))
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
            mDestPath = std::move(desPath);
        }
        mSrcSize = FolderContent(mSrcPath).getSize();
        mDestSize = FolderContent(mDestPath).getSize();
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

int32_t InternalMove::getStatus()
{
    uint32_t size = FolderContent(mDestPath).getSize();
    mDestSize = size;
    uint32_t change = (mSrcSize - size + mDestSize);
    if ((mStatus >= NO_ERROR) && (change > 0))
        mStatus = int(change / mSrcSize * 100);
    else if ((mStatus >= NO_ERROR) && (change == 0))
        mStatus = SUCCESS;
    return mStatus;
}

InternalRename::InternalRename(std::string oldAbsPath, std::string newAbsPath)
    : mOldAbsPath(std::move(oldAbsPath)), mNewAbsPath(std::move(newAbsPath)), mStatus(NO_ERROR)
{
    init();
}

void InternalRename::init ()
{
    try
    {
        if (!validateInternalPath(mOldAbsPath))
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

int32_t InternalRename::getStatus()
{
    return mStatus;
}

InternalCreateDir::InternalCreateDir(std::string path) : mPath(std::move(path)), mStatus(NO_ERROR)
{
    init();
}

void InternalCreateDir::init ()
{
    try
    {
        mStatus = INVALID_PATH;
        if (validateInternalPath(mPath))
        {
            fs::create_directories(mPath);
            mStatus = SUCCESS;
        }
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
        mStatus = PERMISSION_DENIED;
    }
}

int32_t InternalCreateDir::getStatus()
{
    return mStatus;
}

SAFUtilityOperation::SAFUtilityOperation()
{
    try
    {
        if (!fs::exists("/tmp/internal"))
            fs::create_directories("/tmp/internal");
    }
    catch(fs::filesystem_error& e)
    {
        LOG_DEBUG_SAF("%s: %s", __FUNCTION__, e.what());
    }
}

SAFUtilityOperation& SAFUtilityOperation::getInstance()
{
    static SAFUtilityOperation obj = SAFUtilityOperation();
    return obj;
}

std::unique_ptr<FolderContents> SAFUtilityOperation::getListFolderContents(std::string path)
{
    std::unique_ptr<FolderContents> obj = std::unique_ptr<FolderContents>( new FolderContents(std::move(path)));
    return std::move(obj);
}

std::unique_ptr<InternalSpaceInfo> SAFUtilityOperation::getProperties(std::string path)
{
    if (path.empty())   path = "/tmp";
    std::unique_ptr<InternalSpaceInfo> obj = std::unique_ptr<InternalSpaceInfo>( new InternalSpaceInfo(std::move(path)));
    return std::move(obj);
}

std::unique_ptr<InternalCopy> SAFUtilityOperation::copy(std::string srcPath,
    std::string destPath, bool overwrite)
{
    std::unique_ptr<InternalCopy> obj = std::unique_ptr<InternalCopy>(new InternalCopy(std::move(srcPath), std::move(destPath), overwrite));
    return std::move(obj);
}

std::unique_ptr<InternalRemove> SAFUtilityOperation::remove(std::string path)
{
    std::unique_ptr<InternalRemove> obj = std::unique_ptr<InternalRemove>(new InternalRemove(std::move(path)));
    return std::move(obj);
}

std::unique_ptr<InternalMove> SAFUtilityOperation::move(std::string srcPath, std::string destPath, bool overwrite)
{
    std::unique_ptr<InternalMove> obj = std::unique_ptr<InternalMove>(new InternalMove(std::move(srcPath), std::move(destPath), overwrite));
    return std::move(obj);
}

std::unique_ptr<InternalRename> SAFUtilityOperation::rename(std::string srcPath, std::string destPath)
{
    std::unique_ptr<InternalRename> obj = std::unique_ptr<InternalRename>(new InternalRename(std::move(srcPath), std::move(destPath)));
    return std::move(obj);
}

