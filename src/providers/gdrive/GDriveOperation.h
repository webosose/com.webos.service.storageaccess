/* @@@LICENSE
 *
 * Copyright (c) 2020 LG Electronics, Inc.
 *
 * Confidential computer software. Valid license from LG required for
 * possession, use or copying. Consistent with FAR 12.211 and 12.212,
 * Commercial Computer Software, Computer Software Documentation, and
 * Technical Data for Commercial Items are licensed to the U.S. Government
 * under vendor's standard commercial license.
 *
 * LICENSE@@@ */


#ifndef _GDRIVE_OPERATION_H_
#define _GDRIVE_OPERATION_H_
#include <map>
#include "gdrive/gdrive.hpp"

using namespace GDRIVE;

class GDriveOperation
{
public:
    void loadFileIds(std::shared_ptr<Credential>);
    std::string getFileId(std::string);
    std::map<std::string, std::string> getFileMap(std::string path);
private:
    std::map<std::string, std::string> mFileIds;
    void getChildren (std::string, std::shared_ptr<Drive>, std::string);
};
#endif
