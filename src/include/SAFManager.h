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

#ifndef SRC_CORE_SAFMANAGER_H_
#define SRC_CORE_SAFMANAGER_H_

#include <memory>
#include <glib-2.0/glib.h>
#include <SAFLunaService.h>

class SAFManager
{
public:
    SAFManager();
    virtual ~SAFManager();
    bool init(GMainLoop *mainLoop);
    void deInit();

private:

    GMainLoop* mainLoop;
    std::unique_ptr<SAFLunaService> mLunaService;
};

#endif /* SRC_CORE_SAFMANAGER_H_ */
