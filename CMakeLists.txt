# Copyright (c) 2020-2021 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0


cmake_minimum_required(VERSION 2.8.7)
project(storageaccess CXX)

add_definitions(-DNOT_SUPPORT_GFLAGS)
include(webOS/webOS)

webos_modules_init(1 0 0 QUALIFIER RC4)
webos_component(1 0 0)

option (USE_PMLOG "Enable PmLogLib logging" ON)

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(LS2 REQUIRED luna-service2++)
include_directories(${LS2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

pkg_check_modules(PBNJSON_CPP REQUIRED pbnjson_cpp)
include_directories(${PBNJSON_CPP_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${PBNJSON_CPP_CFLAGS_OTHER})

if (USE_PMLOG)
    pkg_check_modules(PMLOGLIB REQUIRED PmLogLib)
    include_directories(${PMLOGLIB_INCLUDE_DIRS})
    webos_add_compiler_flags(ALL ${PMLOGLIB_CFLAGS_OTHER} -DUSE_PMLOG)
endif()

set(INC_FILES
    src/include
    src/providers/gdrive/include
    src/providers/internal
    )
include_directories(${INC_FILES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
file(GLOB SAF_SOURCE
        ${CMAKE_SOURCE_DIR}/src/utils/*.cpp
        ${CMAKE_SOURCE_DIR}/src/luna/*.cpp
        ${CMAKE_SOURCE_DIR}/src/core/*.cpp
        ${CMAKE_SOURCE_DIR}/src/providers/usb/*.cpp
        ${CMAKE_SOURCE_DIR}/src/providers/internal/*.cpp
        ${CMAKE_SOURCE_DIR}/src/providers/gdrive/*.cpp
        ${CMAKE_SOURCE_DIR}/src/main.cpp
)

add_executable(${CMAKE_PROJECT_NAME} ${SAF_SOURCE})

set(LIBS
    ${LS2_LDFLAGS}
    ${GLIB2_LDFLAGS}
    ${PMLOGLIB_LDFLAGS}
    ${PBNJSON_CPP_LDFLAGS}
    -lpthread
    -lcurl
    -lstdc++fs
    -Wl,--no-undefined
    )


target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBS}   ${CMAKE_SOURCE_DIR}/libgdrive/lib/libjconer.a ${CMAKE_SOURCE_DIR}/libgdrive/libgdrive.a)

install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${WEBOS_INSTALL_SBINDIR})

webos_build_system_bus_files()
