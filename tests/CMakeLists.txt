# Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
# SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

if (NOT DEFINED VERSION)
    set(VERSION 1.2.2)
endif ()

set(APP_QRC "../assets/images.qrc")

# 开启 Qt MOC/UIC/RCC，与 src/CMakeLists.txt 保持一致（目录级变量不跨兄弟目录继承）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 同步 src/CMakeLists.txt 中的宏定义（add_definitions 不跨兄弟目录继承）
add_definitions(-DINSTALL_LIB_PATH="${CMAKE_INSTALL_LIBDIR}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DQT_QML_DEBUG_NO_WARNING)
endif()

ADD_COMPILE_OPTIONS(-fno-access-control)

set(CMAKE_SAFETYTEST "${CMAKE_SAFETYTEST_ARG}")

if(CMAKE_SAFETYTEST STREQUAL "")
    set(CMAKE_SAFETYTEST "CMAKE_SAFETYTEST_ARG_OFF")
endif()
#安全编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong -D_FORTITY_SOURCE=1 -z noexecstack -pie -fPIC -z lazy")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${CMAKE_SAFETYTEST}")

if(CMAKE_SAFETYTEST STREQUAL "CMAKE_SAFETYTEST_ARG_ON")
  #安全测试选项
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fsanitize=undefined,address -O2")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=undefined,address -O2")
endif()

# 设置添加gocv相关信息的输出
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fprofile-arcs -ftest-coverage")

#安全加固参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -z relro -z now -z noexecstack -pie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -z relro -z now -z noexecstack -pie")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector-all")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fstack-protector-all")

# 检测架构并选择对应的 Qt 版本，与 src/CMakeLists.txt 保持一致
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
    message(STATUS "Testing for MIPS architecture, using Qt5")
    add_definitions(-DUSE_QT5)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets DBus Sql Multimedia Test Xml Svg WebChannel WebEngineWidgets)
    find_package(Dtk COMPONENTS Core Widget Gui REQUIRED)
else()
    message(STATUS "Testing for Non-MIPS architecture, using Qt6")
    find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Widgets DBus Sql Multimedia Test Xml Svg WebChannel WebEngineWidgets)
    find_package(Dtk6 COMPONENTS Core Widget Gui REQUIRED)
endif()

find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML2 REQUIRED libxml-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(LIBVLC REQUIRED libvlc)
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${LIBXML2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${LIBVLC_INCLUDE_DIRS})

# 保证 src 目录及其子目录的头文件全局可见，与 src/CMakeLists.txt 保持一致
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/dbusservice)
include_directories(${CMAKE_SOURCE_DIR}/src/dbus)
include_directories(${CMAKE_SOURCE_DIR}/src/gui)
include_directories(${CMAKE_SOURCE_DIR}/src/common)
include_directories(${CMAKE_SOURCE_DIR}/src/importolddata/tiptapmigration)
include_directories(${CMAKE_SOURCE_DIR}/src/db)
include_directories(${CMAKE_SOURCE_DIR}/src/task)
include_directories(${CMAKE_SOURCE_DIR}/src/handler)
include_directories(${CMAKE_SOURCE_DIR}/src/audio)
include_directories(src)

set(PROJECT_NAME_TEST
    ${PROJECT_NAME}-test)

# 定义 source 文件
#FILE (GLOB allTestSource
#    "./src/*.cpp"  "./src/*.c"
#    "./src/*.h"
#    )

#Include all app own subdirectorys

file(GLOB_RECURSE VNOTE_SRC_TEST ${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp)
file(GLOB_RECURSE VNOTE_SRC_TEST1 ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp)

# 排除 QML 改造后已删除模块的测试（views/widgets/dialog 引用的类已不存在）
list(FILTER VNOTE_SRC_TEST1 EXCLUDE REGEX "/src/views/")
list(FILTER VNOTE_SRC_TEST1 EXCLUDE REGEX "/src/widgets/")
list(FILTER VNOTE_SRC_TEST1 EXCLUDE REGEX "/src/dialog/")
# 排除 importolddata 根目录测试（对应源码未编译），保留 tiptapmigration 子目录
list(FILTER VNOTE_SRC_TEST1 EXCLUDE REGEX "/src/importolddata/ut_[^/]+\\.cpp$")
# 排除 API 不匹配的历史测试（主代码重构后需重写）
list(REMOVE_ITEM VNOTE_SRC_TEST1
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_actionmanager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_audiowatcher.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_gstreamrecorder.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_standarditemcommon.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_utils.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_vlcplayer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/common/ut_vntaskworker.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/task/ut_vnmainwnddelayinittask.cpp"
)

list(REMOVE_ITEM VNOTE_SRC_TEST "${CMAKE_CURRENT_LIST_DIR}/../src/main.cpp")

# 排除主程序 src/CMakeLists.txt 不编译的历史遗留文件
# src 只扫描 ./importolddata/tiptapmigration，根目录下的旧升级代码未参与构建
list(REMOVE_ITEM VNOTE_SRC_TEST
    "${CMAKE_CURRENT_LIST_DIR}/../src/importolddata/olddataloadwokers.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/importolddata/olddbvisistors.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/importolddata/upgradedbutil.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/importolddata/upgradeview.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/importolddata/vnoteolddatamanager.cpp"
)

# 生成测试可执行程序
add_executable(${PROJECT_NAME_TEST}
    ${VNOTE_SRC_TEST}
    ${VNOTE_SRC_TEST1}

    ${APP_QRC}
    )

# 生成测试可执行程序的依赖库
target_link_libraries(${PROJECT_NAME_TEST} gtest gtest_main -pthread)

# 根据架构链接不同的 Qt 库，与 src/CMakeLists.txt 保持一致
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
    target_include_directories(${PROJECT_NAME_TEST} PUBLIC ${DtkWidget_INCLUDE_DIRS}
                                                           ${DtkCore_INCLUDE_DIRS}
                                                            ${DtkGui_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME_TEST}
        ${DtkWidget_LIBRARIES}
        ${DtkCore_LIBRARIES}
        ${GSTREAMER_LIBRARIES}
        ${LIBVLC_LIBRARIES}
        ${GTEST_LIBRARYS}
        ${GTEST_MAIN_LIBRARYS}
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::DBus
        Qt5::Sql
        Qt5::Multimedia
        Qt5::Test
        Qt5::WebChannel
        Qt5::WebEngineWidgets
        Qt5::Svg
        Qt5::Xml
        ${LIBXML2_LIBRARIES}
    )
else()
    target_link_libraries(${PROJECT_NAME_TEST}
        Dtk6::Core
        Dtk6::Gui
        Dtk6::Widget
        ${GSTREAMER_LIBRARIES}
        ${LIBVLC_LIBRARIES}
        ${GTEST_LIBRARYS}
        ${GTEST_MAIN_LIBRARYS}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::DBus
        Qt6::Sql
        Qt6::Multimedia
        Qt6::Test
        Qt6::WebChannel
        Qt6::WebEngineWidgets
        Qt6::Svg
        Qt6::Xml
        ${LIBXML2_LIBRARIES}
    )
endif()

##------------------------------ 创建'make test'指令---------------------------------------
add_custom_target(test
    COMMAND echo " =================== TEST BEGIN ==================== "
    COMMAND ${CMAKE_BINARY_DIR}/tests/${PROJECT_NAME_TEST}
    COMMAND echo " =================== TEST END ==================== "
)

##'make test'命令依赖与我们的测试程序
add_dependencies(test ${PROJECT_NAME_TEST})
