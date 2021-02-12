/* @@@LICENSE
 *
 * Copyright (c) 2020-2021 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */

#include<vector>
#include "GDriveOperation.h"
#include "SAFLog.h"

void GDriveOperation::loadFileIds(std::shared_ptr<Credential> cred)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    mFileIds.clear();
    mFileIds["/"] = "root";
    std::shared_ptr<Drive> service = std::shared_ptr<Drive>(new Drive(cred.get()));
    getChildren("root", service, "/");
    while (service.use_count() != 0)
        service.reset();
}

void GDriveOperation::getChildren (std::string fileId, std::shared_ptr<Drive> service, std::string parentPath)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    std::vector<GChildren>childs = service->children().Listall(fileId);
    for (int j = 0; j < childs.size(); j++)
    {
       FileGetRequest get = service->files().Get(childs[j].get_id());
       get.add_field("id,title");
       GFile file = get.execute();
       std::string path = parentPath + file.get_title();
       mFileIds[path] = file.get_id();
       LOG_DEBUG_SAF("getChildren  %s =>>>>:: %s", path.c_str(), file.get_title().c_str());
       getChildren(file.get_id(), service, (parentPath + file.get_title() + "/"));
    }
    return;
}

std::string GDriveOperation::getFileId(std::string path)
{
    auto itr = mFileIds.find(path);
    if (itr != mFileIds.end())
        return itr->second;
    return "";
}

std::map<std::string, std::string> GDriveOperation::getFileMap(std::string path)
{
    LOG_DEBUG_SAF("%s", __FUNCTION__);
    if(path.empty())
        return mFileIds;
    else
    {
        std::map<std::string, std::string>temp;
        for(auto entry : mFileIds)
        {
            if(entry.first == path)
                continue;
            if(entry.first.find(path) != std::string::npos)
                temp[entry.first] = entry.second;
        }
        return temp;
    }
}

