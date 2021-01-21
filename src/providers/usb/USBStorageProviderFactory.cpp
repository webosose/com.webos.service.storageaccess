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

#include "USBStorageProvider.h"
#include "USBStorageProviderFactory.h"

DocumentProviderFactory::Registrator<USBStorageProviderFactory> factoryUSBStorage;

std::shared_ptr<DocumentProvider> USBStorageProviderFactory::create(void) const
{
	static std::shared_ptr<DocumentProvider> obj = std::make_shared<USBStorageProvider> ();
    return obj;
}

