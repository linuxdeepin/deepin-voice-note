set(APP_BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/)
set(BIN_NAME ${CMAKE_PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DQT_QML_DEBUG_NO_WARNING)
endif()

# using for linglnog
add_definitions(-DINSTALL_LIB_PATH="${CMAKE_INSTALL_LIBDIR}")

list(APPEND RESOURCES ${CMAKE_PROJECT_NAME}.qrc deepin-voice-note-web.qrc)

# 检测架构并选择对应的 Qt 版本
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
    message(STATUS "Building for MIPS architecture, using Qt5")
    add_definitions(-DUSE_QT5)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets Quick DBus Sql WebEngine WebEngineWidgets Multimedia)
    find_package(Dtk COMPONENTS Core Widget Gui REQUIRED)
else()
    message(STATUS "Building for Non-MIPS architecture, using Qt6")
    find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets Quick DBus Sql WebEngineQuick Multimedia)
    find_package(Dtk6 COMPONENTS Core Widget Gui REQUIRED)
endif()

find_package(PkgConfig REQUIRED)

# 保证 src 目录下头文件全局可见
include_directories(src)
include_directories(./dbusservice)
include_directories(./gui)
include_directories(./common)
include_directories(./db)
include_directories(./handler)
include_directories(./audio)

aux_source_directory(./ ALLSOURCE)
aux_source_directory(./dbusservice ALLSOURCE)
aux_source_directory(./common ALLSOURCE)
aux_source_directory(./db ALLSOURCE)
aux_source_directory(./gui ALLSOURCE)
aux_source_directory(./task ALLSOURCE)
aux_source_directory(./handler ALLSOURCE)
aux_source_directory(./audio ALLSOURCE)

add_executable(${BIN_NAME} ${ALLSOURCE} ${RESOURCES})

# 根据架构链接不同的 Qt 库
if(CMAKE_SYSTEM_PROCESSOR MATCHES "mips.*")
    target_link_libraries(${BIN_NAME} PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Quick
        Qt5::DBus
        Qt5::Sql
        Qt5::WebEngine
        Qt5::WebEngineWidgets
        Qt5::Multimedia
        Dtk::Core
        Dtk::Gui
        Dtk::Widget
    )
else()
    target_link_libraries(${BIN_NAME} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::DBus
        Qt6::Sql
        Qt6::WebEngineQuick
        Qt6::Multimedia
        Dtk6::Core
        Dtk6::Gui
        Dtk6::Widget
    )
endif()

if(SUPPROT_DMR)
    add_definitions(-DMPV_PLAYENGINE)

    message("-- Build with libdmr")
else()
    pkg_check_modules(LIBVLC REQUIRED libvlc)
    include_directories(${LIBVLC_INCLUDE_DIRS})
    target_link_libraries(${BIN_NAME} PRIVATE ${LIBVLC_LIBRARIES})

    message("-- Build with libvlc, found:" ${LIBVLC_FOUND})
endif()
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
include_directories(${GSTREAMER_INCLUDE_DIRS})
target_link_libraries(${BIN_NAME} PRIVATE ${GSTREAMER_LIBRARIES})

# Add install rule
install(TARGETS ${BIN_NAME} DESTINATION ${APP_BIN_INSTALL_DIR})
