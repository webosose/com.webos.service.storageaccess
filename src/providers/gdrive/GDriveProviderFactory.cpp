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

#include "GDriveProvider.h"
#include "GDriveProviderFactory.h"
#include <iostream>
#include<SAFLog.h>

DocumentProviderFactory::Registrator<GDriveProviderFactory> factoryGDriveStorage;

std::shared_ptr<DocumentProvider> GDriveProviderFactory::create(void) const
{
    LOG_DEBUG_SAF("GDriveProviderFactory::create : Function called");
    return std::make_shared<GDriveProvider> ();
}

