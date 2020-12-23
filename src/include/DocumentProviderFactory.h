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

#ifndef _DOCUMENT_PROVIDER_FACTORY_H_
#define _DOCUMENT_PROVIDER_FACTORY_H_

#include <map>
#include <memory>
#include <vector>
#include "DocumentProvider.h"

using namespace std;

class DocumentProviderFactory
{
    public:
        virtual ~DocumentProviderFactory() {}

        virtual  shared_ptr<DocumentProvider> create() const = 0;
        virtual StorageType getType() const = 0;

        static shared_ptr<DocumentProvider> createDocumentProvider(StorageType type);
        static shared_ptr<vector<StorageType>> getSupportedDocumentProviders();
        typedef shared_ptr<DocumentProviderFactory> Factory;
        typedef map<StorageType, Factory> Factories;

        template <typename T>
        struct Registrator
        {
            Registrator()
            {
                Factory factory { new T() };
                StorageType key(factory->getType());
                pair<map<StorageType, Factory>::iterator, bool > result;
                result = mFactories.emplace(pair<StorageType, Factory>(key, factory));
            }
        };

    private:
        static Factories mFactories;
};

#endif
