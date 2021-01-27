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

#ifndef _INTERNAL_STORAGE_PROVIDER_FACTORY_H_
#define _INTERNAL_STORAGE_PROVIDER_FACTORY_H_

#include "DocumentProvider.h"
#include "DocumentProviderFactory.h"

class InternalStorageProviderFactory : public DocumentProviderFactory
{
    public:
        virtual std::shared_ptr<DocumentProvider> create(void) const;
        StorageType getType() const { return StorageType::INTERNAL; }
};
#endif
