// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <SAFLog.h>
#include <SAFManager.h>
#include <SAFLunaService.h>

SAFManager::SAFManager(): mainLoop(nullptr), mLunaService(nullptr)
{

}

SAFManager::~SAFManager()
{
    deInit();
}

bool SAFManager::init(GMainLoop *mainLoop)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    this->mainLoop = mainLoop;

    try {
        mLunaService = std::unique_ptr<SAFLunaService>(new SAFLunaService());
        if (!mLunaService)
            return false;
    } catch (std::bad_alloc& eh) {
        LOG_ERROR_SAF( "safManager:", 0, "ERROR: SAFLunaservice bad_alloc caught" );
        return false;
    }

    mLunaService->attachToLoop(mainLoop);
    return true;
}

void SAFManager::deInit()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    try {
        if (mLunaService) {
            mLunaService->detach();
        }
    } catch (LS::Error &lunaError) {
        LOG_ERROR_SAF( "safManager:", 0, "ERROR: mLunaService detach thrown an exception" );
    }
}
