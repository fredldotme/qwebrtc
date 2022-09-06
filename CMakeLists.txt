cmake_minimum_required(VERSION 3.1)
project(QWebRTC C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
#set(CMAKE_SKIP_RPATH 1)
set(CMAKE_MACOSX_RPATH 1)
add_definitions(-DWEBRTC_POSIX)
if (APPLE)
    add_definitions(-DWEBRTC_MAC)
endif()
add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

option(ENABLE_ASAN "ASAN build" OFF)
option(ENABLE_TSAN "TSAN build" OFF)
option(ENABLE_UBSAN "UBSAN build" OFF)

set(CMAKE_CXX_FLAGS "-Wall -Werror -Wno-deprecated-declarations -Wno-error=unused-variable -fno-rtti ${CMAKE_CXX_FLAGS}")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND "${CMAKE_GENERATOR}" STREQUAL "Ninja")
    set(CMAKE_CXX_FLAGS "-fcolor-diagnostics ${CMAKE_CXX_FLAGS}")
    set(START_GROUP )
    set(END_GROUP )
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(START_GROUP "-Wl,--start-group")
    set(END_GROUP "-Wl,--end-group")
    set(CMAKE_CXX_FLAGS "-fdiagnostics-color=always -Wno-error=maybe-uninitialized -Wno-error=unused-local-typedefs ${CMAKE_CXX_FLAGS}")
endif()

if(ENABLE_ASAN)
    set(CMAKE_C_FLAGS "-fsanitize=address ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=address ${CMAKE_CXX_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=address ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address ${CMAKE_EXE_LINKER_FLAGS}")
endif()
if(ENABLE_TSAN)
    set(CMAKE_C_FLAGS "-fsanitize=thread ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=thread ${CMAKE_CXX_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread ${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS}")
endif()
if(ENABLE_UBSAN)
    if(APPLE)
        set(CMAKE_C_FLAGS "-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error ${CMAKE_CXX_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error ${CMAKE_SHARED_LINKER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error ${CMAKE_EXE_LINKER_FLAGS}")
    else()
        set(CMAKE_C_FLAGS "-fsanitize=undefined ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=undefined ${CMAKE_SHARED_LINKER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=undefined ${CMAKE_EXE_LINKER_FLAGS}")
    endif()
endif()

include_directories(
    "${CMAKE_CURRENT_BINARY_DIR}/webrtc/src/"
)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#find_path(WebRTCOutDir webrtc
#    HINTS "${CMAKE_CURRENT_BINARY_DIR}/webrtc/src/out/Default/obj"
#    PATH_SUFFIXES webrtc)

find_package(Qt5 COMPONENTS Core Widgets Quick Multimedia)

set(INSTALL_DISABLED TRUE)

set(LIB_SOURCES
    "qwebrtc.cpp"
    "qwebrtcpeerconnectionfactory.cpp"
    "qwebrtcpeerconnection.cpp"
    "qwebrtcdatachannel.cpp"
    "qwebrtcmediastream.cpp"
    "qwebrtcsessiondescription.cpp"
    "qwebrtcmediatrack.cpp"
    "qwebrtcutilities.cpp"
    "qwebrtcquickvideoitem.cpp"
    "qwebrtcicecandidate.cpp"
    "qwebrtcdesktopvideosource.cpp"
)

set(PUBLIC_HEADER
    "qwebrtc.h"
    "qwebrtcpeerconnectionfactory.hpp"
    "qwebrtcpeerconnection.hpp"
    "qwebrtcdatachannel.hpp"
    "qwebrtcmediastream.hpp"
    "qwebrtcsessiondescription.hpp"
    "qwebrtcmediatrack.hpp"
    "qwebrtcutilities.hpp"
    "qwebrtcquickvideoitem.hpp"
    "qwebrtcicecandidate.hpp"
    "qwebrtcconfiguration.hpp"
)

install(FILES ${PUBLIC_HEADER} DESTINATION "${CMAKE_BINARY_DIR}/include")

add_definitions(-DQT_NO_KEYWORDS)

set(PRIVATE_HEADERS
    "qwebrtcpeerconnection_p.hpp"
    "qwebrtcmediatrack_p.hpp"
    "qwebrtcmediastream_p.hpp"
    "qwebrtcdatachannel_p.hpp"
    "qwebrtctypes_p.hpp"
    "qwebrtcquickvideoitem_p.hpp"
    "qwebrtcdesktopvideosource_p.hpp"
)

add_library(${PROJECT_NAME} SHARED
    ${LIB_SOURCES}
    ${PRIVATE_HEADERS}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC

    -Wl,--whole-archive
    "${WebRTCLibDir}/libwebrtc.a"
    -Wl,--no-whole-archive
    Qt5::Core
    Qt5::Quick
    Qt5::Multimedia
)

if(APPLE)
    file(GLOB_RECURSE FFMPEG_OBJ
        "${WebRTCOutDir}/../third_party/ffmpeg/ffmpeg_internal/*.o"
    )
    find_library(FoundationLibs Foundation)
    find_library(CoreAudioLibs CoreAudio)
    find_library(AudioToolboxLibs AudioToolbox)
    find_library(CoreGraphicsLibs CoreGraphics)
    find_library(CoreVideoLibs CoreVideo)
    find_library(CoreMediaLibs CoreMedia)
    find_library(VideoToolboxLibs VideoToolbox)
    find_library(AVFoundationLibs AVFoundation)
    find_library(OpenGlLibs OpenGl)
    find_library(CocoaLibs Cocoa)

    target_link_libraries(${PROJECT_NAME}
        ${FFMPEG_OBJ}
        ${FoundationLibs}
        ${CoreAudioLibs}
        ${AudioToolboxLibs}
        ${CoreGraphicsLibs}
        ${CoreVideoLibs}
        ${CoreMediaLibs}
        ${CocoaLibs}
       "${WebRTCOutDir}/../third_party/expat/libexpat.a"
        ${AVFoundationLibs}
        ${OpenGlLibs}
    )
else() #LINUX
    target_link_libraries(${PROJECT_NAME}
        PUBLIC

        stdc++
        hybris-common
        media
        X11
        yuv
        #"${WebRTCOutDir}/../base/third_party/libevent/libevent.a"
    )
endif()
