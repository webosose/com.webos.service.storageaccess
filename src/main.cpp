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
#include <glib-2.0/glib.h>
#include <memory>
#include <SAFManager.h>
#include <SAFLog.h>

static GMainLoop *mainLoop = nullptr;
//std::unique_ptr<SAFLunaService> safLunaService;


void term_handler(int signum)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    const char *str = nullptr;

    switch (signum) {
        case SIGTERM:
            str = "SIGTERM";
            break;

        case SIGABRT:
            str = "SIGABRT";
            break;

        default:
            str = "Unknown";
            break;
    }

    LOG_DEBUG_SAF("signal received.. signal[%s]", str);
    g_main_loop_quit(mainLoop);
}

int main(int argc, char **argv)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    (void)signal(SIGTERM, term_handler);
    (void)signal(SIGABRT, term_handler);

    GMainLoop *mainLoop = g_main_loop_new(nullptr, FALSE);
    if (!mainLoop) {
        LOG_DEBUG_SAF("Failed to create g_main_loop!");
        return EXIT_FAILURE;
    }

    std::unique_ptr<SAFManager>safManager(new SAFManager());
    try{
          bool init_result = false;
          if(safManager){
            init_result = safManager->init(mainLoop);
          }
          if(!safManager || !init_result)
          {
             LOG_DEBUG_SAF("SAF Manager registration failed");
             g_main_loop_unref(mainLoop);
             return EXIT_FAILURE;
          }
    }
    catch (LS::Error &lunaError) {
        LOG_ERROR_SAF( "safManager:", 0, "ERROR: init thrown an exception" );
    }
    g_main_loop_run(mainLoop);
    safManager.reset();
    g_main_loop_unref(mainLoop);

    return 0;
}

