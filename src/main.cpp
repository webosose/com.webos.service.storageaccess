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
#include <glib-2.0/glib.h>
#include <memory>
#include <SAFManager.h>
#include <SAFLog.h>

static GMainLoop *mainLoop = nullptr;

void term_handler(int signum) {
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

    LOG_ERROR_SAF("SAF_ERROR", 0, "signal received.. signal[%s]", str);
    g_main_loop_quit(mainLoop);
}

int main(int argc, char **argv) {
    LOG_TRACE("Entering function %s", __FUNCTION__);

    try {
        (void) signal(SIGTERM, term_handler);
        (void) signal(SIGABRT, term_handler);

        mainLoop = g_main_loop_new(NULL, FALSE);
        if (!mainLoop) {
            LOG_ERROR_SAF("SAF_ERROR", 0, "Failed to create g_main_loop!");
            return EXIT_FAILURE;
        }

        SAFManager safManager;
        bool init_result = safManager.init(mainLoop);
        if (!init_result) {
            LOG_ERROR_SAF("SAF_ERROR", 0, "SAF Manager registration failed");
            g_main_loop_unref(mainLoop);
            return EXIT_FAILURE;
        }

        g_main_loop_run(mainLoop);
        LOG_INFO_SAF(MSGID_FUNCTION_CALL, 0, "Going to Unref mainloop: %d", __LINE__);
        g_main_loop_unref(mainLoop);
    } catch (const std::length_error &le) {
        LOG_ERROR_SAF("SAF_ERROR", 0, "Failed to Length error: %s", le.what());
    } catch (const LS::Error &error) {
        LOG_ERROR_SAF("SAF_ERROR", 0, "ERROR: init thrown an exception: %s",
                error.what());
    }

    return 0;
}

