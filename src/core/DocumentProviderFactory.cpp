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


#include "DocumentProviderFactory.h"

DocumentProviderFactory::Factories DocumentProviderFactory::mFactories;

shared_ptr<vector<StorageType>> DocumentProviderFactory::getSupportedDocumentProviders()
{
    shared_ptr<vector<StorageType>> providers = make_shared<vector<StorageType>>();
    for (auto itr = mFactories.begin(); itr != mFactories.end(); ++itr) {
        providers->push_back(itr->first);
    }
    return providers;
}

shared_ptr<DocumentProvider> DocumentProviderFactory::createDocumentProvider(StorageType type)
{
    shared_ptr<DocumentProvider> docProvider;

    map<StorageType, Factory>::iterator it;
    it = mFactories.find(type);
    if(it != mFactories.end())
    {
        Factory factory = mFactories.find(type)->second;
        docProvider = factory->create();
    }
    return docProvider;
}

