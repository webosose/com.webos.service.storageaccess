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

#include "SAFLog.h"
#include "DocumentProviderManager.h"
#include "SA_Common.h"

DocumentProviderManager::DocumentProviderManager()
{
}
DocumentProviderManager::~DocumentProviderManager()
{
}

void DocumentProviderManager::addRequest(std::shared_ptr<RequestData>& reqData)
{
    if(((reqData->methodType == MethodType::COPY_METHOD) || (reqData->methodType == MethodType::MOVE_METHOD))
        && (reqData->storageType != StorageType::GDRIVE) && (reqData->params["destStorageType"].asString() == "CLOUD"))
    {
        reqData->storageType = StorageType::GDRIVE;
    }

    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(reqData->storageType);
    return provider->addRequest(reqData);
}

