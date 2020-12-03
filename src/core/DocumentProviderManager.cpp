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

#include "DocumentProviderManager.h"

DocumentProviderManager::DocumentProviderManager()
{
}
DocumentProviderManager::~DocumentProviderManager()
{
}

ReturnValue DocumentProviderManager::listOfStoreages()
{
    shared_ptr<vector<string>> storageProviders = DocumentProviderFactory::getSupportedDocumentProviders();
    shared_ptr<ContentList> storageList = make_shared<ContentList>();

    for (auto itr = storageProviders->begin(); itr != storageProviders->end(); ++itr) {
        if (*itr == "USB") {
            // For USB Storages, First need to find out it is attached or not
            // by calling PDM service, then populate the data with the result.
            storageList->push_back(make_shared<Storage>(*itr, "-1", *itr));
        } else{
            storageList->push_back(make_shared<Storage>(*itr, "-1", *itr));
        }
    }

    shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();
    valueMap->emplace("returnValue", pair<string,DataType>("true", DataType::BOOLEAN));
    valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
    valueMap->emplace("errorText",   pair<string,DataType>("true", DataType::STRING));

    return make_shared<ResultPair>(valueMap, storageList);
}

ReturnValue DocumentProviderManager::listFolderContents(string storageType, string storageId, string path, int offset, int limit)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->listFolderContents(storageId, path, offset, limit);
}

ReturnValue DocumentProviderManager::getProperties(string storageType)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->getProperties();
}

ReturnValue DocumentProviderManager::copy(string srcStorageType, string srcStorageId, string srcPath,
                            string destStorageType, string destStorageId, string destPath, bool overwrite)
{
    if (srcStorageType == destStorageType) {
        shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
        return provider->copy(srcStorageId, srcPath, destStorageId, destPath, overwrite);
    } else {
        shared_ptr<DocumentProvider> srcProvider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
        shared_ptr<DocumentProvider> destProvider = DocumentProviderFactory::createDocumentProvider(destStorageType);

        shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();

        fstream inputFile  = srcProvider->getFileStream(srcPath, ios::in|ios::binary);
        fstream outputFile = srcProvider->getFileStream(destPath, ios::out|ios::binary);
        if (!inputFile) {
            valueMap->emplace("returnValue", pair<string,DataType>("false", DataType::BOOLEAN));
            valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
            valueMap->emplace("errorText",   pair<string,DataType>("Input file not available", DataType::STRING));
            return make_shared<ResultPair>(valueMap, nullptr);
        }
        if (!outputFile) {
            valueMap->emplace("returnValue", pair<string,DataType>("false", DataType::BOOLEAN));
            valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
            valueMap->emplace("errorText",   pair<string,DataType>("Unable to open output file", DataType::STRING));
            return make_shared<ResultPair>(valueMap, nullptr);
        }
        char* buffer = new char[1024];
        while(true) {
            inputFile.read(buffer, 1024);
            outputFile.write(buffer, 1024);
            if (inputFile.eof()) break;
        }
        valueMap->emplace("returnValue", pair<string,DataType>("true", DataType::BOOLEAN));
        valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
        valueMap->emplace("errorText",   pair<string,DataType>("", DataType::STRING));
        return make_shared<ResultPair>(valueMap, nullptr);
    }
}

ReturnValue DocumentProviderManager::move(string srcStorageType, string srcStorageId, string srcPath,
                            string destStorageType, string destStorageId, string destPath, bool overwrite)
{
    if (srcStorageType == destStorageType) {
        shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
        return provider->copy(srcStorageId, srcPath, destStorageId, destPath, overwrite);
    } else {
        shared_ptr<DocumentProvider> srcProvider = DocumentProviderFactory::createDocumentProvider(srcStorageType);
        shared_ptr<DocumentProvider> destProvider = DocumentProviderFactory::createDocumentProvider(destStorageType);

        shared_ptr<ValuePairMap> valueMap = make_shared<ValuePairMap>();

        fstream inputFile  = srcProvider->getFileStream(srcPath, ios::in|ios::binary);
        fstream outputFile = srcProvider->getFileStream(destPath, ios::out|ios::binary);
        if (!inputFile) {
            valueMap->emplace("returnValue", pair<string,DataType>("false", DataType::BOOLEAN));
            valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
            valueMap->emplace("errorText",   pair<string,DataType>("Unable to open output file", DataType::STRING));
            return make_shared<ResultPair>(valueMap, nullptr);
        }
        if (!outputFile) {
            valueMap->emplace("returnValue", pair<string,DataType>("false", DataType::BOOLEAN));
            valueMap->emplace("errorCode",   pair<string,DataType>("-1",   DataType::NUMBER));
            valueMap->emplace("errorText",   pair<string,DataType>("Unable to open output file", DataType::STRING));
            return make_shared<ResultPair>(valueMap, nullptr);
        }

        char* buffer = new char[1024];
        while(true) {
            inputFile.read(buffer, 1024);
            outputFile.write(buffer, 1024);
            if (inputFile.eof()) break;
        }
        return srcProvider->remove(srcStorageId, srcPath);
    }
}

ReturnValue DocumentProviderManager::remove(string storageType, string storageId, string path)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->remove(storageId, path);
}

ReturnValue DocumentProviderManager::eject(string storageType, string storageId)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->eject(storageId);
}

ReturnValue DocumentProviderManager::format(string storageType, string storageId, string fileSystem, string volumeLabel)
{
    shared_ptr<DocumentProvider> provider = DocumentProviderFactory::createDocumentProvider(storageType);
    return provider->format(storageId,fileSystem,volumeLabel);
}
