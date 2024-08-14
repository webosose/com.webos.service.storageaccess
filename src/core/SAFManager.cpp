// Copyright (c) 2020-2022 LG Electronics, Inc.
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

bool SAFManager::init(GMainLoop *_mainLoop) {
    LOG_TRACE("Entering function %s", __FUNCTION__);

    mainLoop = _mainLoop;
    mLunaService.init();
    mLunaService.attachToLoop(mainLoop);
    LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "After attached to mainloop: %d", __LINE__);
    return true;
}
