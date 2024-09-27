cmake_minimum_required(VERSION 3.16)

project(deepin-voice-note)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(APP_RES_DIR "assets")
set(APP_DESKTOP "${APP_RES_DIR}/deepin-voice-note.desktop")
set(APP_CONFIG "${APP_RES_DIR}/deepin-voice-note.conf")
set(APP_TRANS_SCRIPT "${APP_RES_DIR}/translate_generation.sh")
set(APP_LOGO "${APP_RES_DIR}/icons/deepin/builtin/deepin-voice-note.svg")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DWEB_PATH="${CMAKE_SOURCE_DIR}/src/web")
else()
    add_definitions(-DWEB_PATH="${CMAKE_INSTALL_PREFIX}/share/deepin-voice-note/web")
endif()

execute_process(COMMAND bash
                    ${APP_TRANS_SCRIPT}
                    translations
                    ${APP_TS_UPDATE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${APP_BIN_NAME}
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")

# 加速编译优化参数
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "mips64")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections -ftree-vectorize -march=loongson3a -mhard-float -mno-micromips -mno-mips16 -flax-vector-conversions -mloongson-ext2 -mloongson-mmi -z noexecstack")
    SET(CMAKE_EXE_LINKER_FLAGS  "-pie")
else()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -Wl,-O1 -Wl,--as-needed -Wl,-E -fPIE -ffunction-sections -fdata-sections -Wl,--gc-sections")
    SET(CMAKE_EXE_LINKER_FLAGS  "-pie")
endif()

include_directories(${PROJECT_BINARY_DIR})
include_directories(${PROJECT_SOURCE_DIR})

set(SCREENRECORDERCORE_QML_URI "com.deepin.voicenote.private")
set(SCREENRECORDERCORE_QML_PATH "qrc:/com/deepin/voicenote/private")

configure_file(${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h @ONLY)

# Application
add_subdirectory(src)

file(GLOB APP_QM_FILES "translations/*.qm")
install(FILES ${APP_QM_FILES} DESTINATION share/deepin-voice-note/translations)
install(FILES ${APP_DESKTOP} DESTINATION share/applications)
install(FILES ${APP_CONFIG} DESTINATION share/deepin-voice-note)
install(FILES ${APP_LOGO} DESTINATION  ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/web    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/deepin-voice-note)
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "sw_64")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/sw/deepin-voice-note     DESTINATION /usr/share/deepin-manual/manual-assets/application/)
else()
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/deepin-voice-note     DESTINATION /usr/share/deepin-manual/manual-assets/application/)
endif()

# Unit Tests
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # QML 改造早期屏蔽
    # add_subdirectory(tests)
endif()
