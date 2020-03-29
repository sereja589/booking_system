cmake_minimum_required(VERSION 3.5)

project(booking_system LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

if(ANDROID)
  add_library(booking_system SHARED
    booking_system.cpp
    booking_system.h
    clock.h
    emulator.cpp
    emulator.h
    main.cpp
    start_window.cpp
    start_window.h
    start_window.ui
  )
else()
  add_executable(booking_system
    booking_system.cpp
    booking_system.h
    clock.h
    emulator.cpp
    emulator.h
    main.cpp
    start_window.cpp
    start_window.h
    start_window.ui
  )
endif()

target_link_libraries(booking_system PRIVATE Qt5::Widgets)
