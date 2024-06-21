set(APP_BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin/)
set(BIN_NAME ${CMAKE_PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DQT_QML_DEBUG_NO_WARNING)
endif()

list(APPEND RESOURCES ${CMAKE_PROJECT_NAME}.qrc)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Widgets Quick DBus Sql WebEngineQuick)

find_package(PkgConfig REQUIRED)

# 保证 src 目录下头文件全局可见
include_directories(src)
include_directories(./dbusservice)
include_directories(./gui)
include_directories(./common)
include_directories(./db)

aux_source_directory(./ ALLSOURCE)
aux_source_directory(./dbusservice ALLSOURCE)
aux_source_directory(./common ALLSOURCE)
aux_source_directory(./db ALLSOURCE)
aux_source_directory(./gui ALLSOURCE)
aux_source_directory(./task ALLSOURCE)

add_executable(${BIN_NAME} ${ALLSOURCE} ${RESOURCES})

target_link_libraries(${BIN_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::DBus
    Qt6::Sql
    Qt6::WebEngineQuick
)

# Add install rule
install(TARGETS ${BIN_NAME} DESTINATION ${APP_BIN_INSTALL_DIR})
# install Desktop
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/${CMAKE_PROJECT_NAME}.desktop DESTINATION share/applications)
# install Manual App icon
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/${CMAKE_PROJECT_NAME}.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
# install service
