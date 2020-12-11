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

#include <SAFLog.h>
#include <DocumentProviderManager.h>

DocumentProviderManager::DocumentProviderManager()
{
}
DocumentProviderManager::~DocumentProviderManager()
{
}

ReturnValue DocumentProviderManager::listOfStorages()
{
    shared_ptr<vector<StorageType>> storageProviders = DocumentProviderFactory::getSupportedDocumentProviders();
    shared_ptr<ContentList> storageList = make_shared<ContentList>();

    for (auto itr = storageProviders->begin(); itr != storageProviders->end(); ++itr) {
        switch(*itr) {
            case StorageType::USB:
                // For USB Storages, First need to find out it is attached or not
                // by calling PDM service, then populate the data with the result.
                storageList->push_back(make_shared<Storage>(*itr, "-1", "USB"));
            break;
            default:
                storageList->push_back(make_shared<Storage>(*itr, "-1", ""));
        }
    }

    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    valueMap->emplace("returnValue", pair<string,DataType>("true", DataType::BOOLEAN));
    valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
    valueMap->emplace("errorText",   pair<string,DataType>("true", DataType::STRING));

    return make_shared<ResultPair>(valueMap, storageList);
}

ReturnValue DocumentProviderManager::attachCloud(StorageType storageType, AuthParam authParam)
{
    LOG_DEBUG_SAF("DocumentProviderManager::attachCloud : storageType = %d", storageType);
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->attachCloud(authParam);
}

ReturnValue DocumentProviderManager::authenticateCloud(StorageType storageType, AuthParam authParam)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->authenticateCloud(authParam);
}

ReturnValue DocumentProviderManager::listFolderContents(AuthParam authParam, StorageType storageType, string storageId, string path, int offset, int limit)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->listFolderContents(authParam, storageId, path, offset, limit);
}

ReturnValue DocumentProviderManager::getProperties(AuthParam authParam, StorageType storageType)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->getProperties(authParam);
}

ReturnValue DocumentProviderManager::copy(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath,
                            AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    shared_ptr<DocumentProvider> provider;
    if (srcStorageType == StorageType::GDRIVE) {
        provider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
    } else {
        provider = DocumentProviderFactory::createDocumentProvider(destStorageType);
    }
    return provider->copy(srcAuthParam, srcStorageType, srcStorageId, srcPath, destAuthParam, destStorageType, destStorageId, destPath, overwrite);
}

ReturnValue DocumentProviderManager::move(AuthParam srcAuthParam, StorageType srcStorageType, string srcStorageId, string srcPath,
                            AuthParam destAuthParam, StorageType destStorageType, string destStorageId, string destPath, bool overwrite)
{
    shared_ptr<DocumentProvider> provider;
    if (srcStorageType == StorageType::GDRIVE) {
        provider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
    } else {
        provider = DocumentProviderFactory::createDocumentProvider(destStorageType);
    }
    return provider->move(srcAuthParam, srcStorageType, srcStorageId, srcPath, destAuthParam, destStorageType, destStorageId, destPath, overwrite);
}

ReturnValue DocumentProviderManager::remove(AuthParam authParam, StorageType storageType, string storageId, string path)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->remove(authParam, storageId, path);
}

ReturnValue DocumentProviderManager::eject(StorageType storageType, string storageId)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->eject(storageId);
}

ReturnValue DocumentProviderManager::format(StorageType storageType, string storageId, string fileSystem, string volumeLabel)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->format(storageId,fileSystem,volumeLabel);
}

