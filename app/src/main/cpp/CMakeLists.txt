# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)


include_directories(${CMAKE_CURRENT_SOURCE_DIR} )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/base/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libyuv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/x264/include )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libyuv)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}  players)
find_library( EGL-lib
        EGL )

find_library( GLESv2-lib
        GLESv2 )

file(GLOB RENDER_SRCS
        render/*.cpp
        render/GlScreen.cpp
        )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/render )
add_library(
        glrender
        STATIC
        ${RENDER_SRCS}
)

file(GLOB DEVICE_SRCS
        audio_device/*.cpp
        audio_device/OpenSLESRecorder.cpp
        audio_device/OpenSLESCommon.cpp
        )
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/audio_device )
add_library(
        audiodevice
        STATIC
        ${DEVICE_SRCS}
)


# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib
        # Sets the library as a shared library.
        SHARED
#        Demuxer.cpp
#        utils.cpp
#        # Provides a relative path to your source file(s).
#        native-lib.cpp
        ${players}
        VideoDecoder.cpp
        HWVideoEncoder.cpp
        UERead.cpp
        Muxer.cpp
        )


#add_library(ffmpeg STATIC IMPORTED)
#set_target_properties(ffmpeg PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/ffmpeg/libffmpeg.a)
#
#add_library(xplatfrom STATIC IMPORTED)
#set_target_properties(xplatfrom PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/ffmpeg/libxplatform_util.a)

#SET_TARGET_PROPERTIES(ffmpeg PROPERTIES COMPILE_FLAGS "-O3 -DNDEBUG  ")

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

link_directories(. ${CMAKE_SOURCE_DIR}/ffmpeg)
include_directories(ffmpeg . ffmpeg/android/config/arm64)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_library(zlib z)
find_package(Threads REQUIRED)

SET_TARGET_PROPERTIES(native-lib PROPERTIES COMPILE_FLAGS " -Wdeprecated-declarations")

target_link_libraries( # Specifies the target library.
        native-lib Threads::Threads
        glrender
        base
        yuv
        audiodevice
        android
        OpenSLES
        ${CMAKE_SOURCE_DIR}/ffmpeg/libavformat.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libavdevice.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libavcodec.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libavutil.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libavfilter.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libswscale.a
        ${CMAKE_SOURCE_DIR}/ffmpeg/libswresample.a
        ${CMAKE_SOURCE_DIR}/x264/lib/libx264.a
        # Links the target library to the log library
        # included in the NDK.
        mediandk
        ${zlib}
        ${log-lib}
        ${EGL-lib}
        ${GLESv2-lib})
#target_link_libraries( # Specifies the target library.
#        native-lib
#        ffmpeg
#        xplatfrom
#        # Links the target library to the log library
#        # included in the NDK.
#        ${log-lib})