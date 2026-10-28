# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# webkitbridge — WebKitGTK 桥公共库（源自任务 V-6795 交付物，本仓库内 vendor 集成）
#
# 结构：
#   helper/webhelper.c          独立 WebKitGTK 进程（纯 C）
#   host/webkitbridgeclient.*   宿主侧 Qt 封装（唯一接入点）
# 集成扩展（voice-note）：
#   - 宿主侧 attach(QQuickItem*) 锚定（纯 QML 应用免 QWidget 容器）
#   - ExecuteEditCommand / SetZoom 方法 + ZoomChanged / ContextMenu 信号
#   - helper 内置 QtWebChannel 等价 transport 注入、右键菜单接管、文件拖放转发
#
# 依赖：Qt(Core/Gui/Widgets/Quick/DBus) + webkit2gtk-4.1 + gtk+-3.0
# Qt 版本与主工程保持一致（mips→Qt5，其余 Qt6）；主工程在 src/ 内才正式
# find_package，这里先行探测一次。
if(NOT DEFINED QT_VERSION_MAJOR)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
        set(QT_VERSION_MAJOR 5)
    else()
        set(QT_VERSION_MAJOR 6)
    endif()
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Quick DBus)

find_package(PkgConfig REQUIRED)
# mips 平台（Qt5）使用 webkit2gtk-4.0（与 debian/control 的架构限定一致），
# 其余平台使用 4.1；webhelper 仅使用两代共有 API，无 libsoup3 依赖。
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
    pkg_check_modules(WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
else()
    pkg_check_modules(WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
endif()
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
# webhelper 直接调用 XGetErrorText/XSetErrorHandler 等原生 X11 接口
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

# ---- webhelper：独立 WebKitGTK 进程 ----
add_executable(webhelper helper/webhelper.c)
target_compile_options(webhelper PRIVATE -Wno-unused-parameter -Wno-comment)
target_link_libraries(webhelper PRIVATE
    PkgConfig::WEBKITGTK
    PkgConfig::GTK3
    PkgConfig::X11
    m
    ${WEBKITGTK_LIBRARIES}
)
target_include_directories(webhelper PRIVATE ${WEBKITGTK_INCLUDE_DIRS} ${GTK3_INCLUDE_DIRS})
target_link_directories(webhelper PRIVATE ${WEBKITGTK_LIBRARY_DIRS} ${GTK3_LIBRARY_DIRS})

# ---- webkitbridge：宿主侧静态库 ----
add_library(webkitbridge STATIC host/webkitbridgeclient.cpp)
# Q_OBJECT 类需要 moc（V-6795 演示工程为全局 AUTOMOC，这里目标级开启）
set_target_properties(webkitbridge PROPERTIES AUTOMOC ON)
target_include_directories(webkitbridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/host)
target_link_libraries(webkitbridge PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::DBus
    PkgConfig::WEBKITGTK
    PkgConfig::GTK3
    PkgConfig::X11
)
target_link_directories(webkitbridge PUBLIC ${WEBKITGTK_LIBRARY_DIRS} ${GTK3_LIBRARY_DIRS})

# webhelper 与主程序同目录（helperPath 默认 applicationDirPath()/webhelper）
install(TARGETS webhelper DESTINATION bin)
