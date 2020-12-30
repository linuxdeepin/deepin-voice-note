#/*
#* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
#*
#* Author:     zhansgteng <zhangteng@uniontech.com>
#* Maintainer: zhangteng <zhangteng@uniontech.com>
#*
#* This program is free software: you can redistribute it and/or modify
#* it under the terms of the GNU General Public License as published by
#* the Free Software Foundation, either version 3 of the License, or
#* any later version.
#* This program is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#* GNU General Public License for more details.
#* You should have received a copy of the GNU General Public License
#* along with this program.  If not, see <http://www.gnu.org/licenses/>.
#*/

cmake_minimum_required(VERSION 3.0.0)

project(test_libfuzzer VERSION 0.1 LANGUAGES CXX C)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -g -fsanitize=undefined,address,leak -fsanitize=fuzzer -w")
set(CMAKE_C_FLAGS " ${CMAKE_C_FLAGS} -g -fsanitize=undefined,address,leak -fsanitize=fuzzer -w")

include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0)
include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0/gst)
include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0/gst/base)
include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0/gst/check)
include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0/gst/controller)
include_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/include/gstreamer-1.0/gst/net)
link_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/lib/)
link_directories(${CMAKE_SOURCE_DIR}/gstream-1.4/lib/gstreamer-1.0)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
file(GLOB_RECURSE VNOTE_SRC ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

file(GLOB_RECURSE GST_SRC ${CMAKE_CURRENT_LIST_DIR}/gst/*.c)

add_executable(${PROJECT_NAME} ${VNOTE_SRC})

target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/thirdlib/libFuzzer.a)
target_link_libraries(${PROJECT_NAME} ${GLIB_LIBRARIES})
target_link_libraries(${PROJECT_NAME} gstreamer-1.0)

execute_process(COMMAND /usr/bin/cp -rf ${CMAKE_SOURCE_DIR}/libfuzzer_in ${PROJECT_BINARY_DIR}/)
execute_process(COMMAND mkdir -p ${PROJECT_BINARY_DIR}/libfuzzer_out)

