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

#include "InternalStorageProvider.h"
#include "InternalStorageProviderFactory.h"

DocumentProviderFactory::Registrator<InternalStorageProviderFactory> factoryInternalStorage;

std::shared_ptr<DocumentProvider> InternalStorageProviderFactory::create(void) const
{
    return std::make_shared<InternalStorageProvider> ();
}

