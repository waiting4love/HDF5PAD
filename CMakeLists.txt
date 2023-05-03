cmake_minimum_required(VERSION 3.0.0)
project(HDF5Pad VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS REQUIRED Widgets Core)
find_package(hdf5 CONFIG REQUIRED)
find_package(HighFive CONFIG REQUIRED)

set(MAIN_SRCS main.cpp mainwindow.cpp pager.cpp)
set(APP_ICON "res/app.rc")
set(QRC_SOURCE_FILES hdf5pad.qrc)

qt5_add_resources(QRC_FILES ${QRC_SOURCE_FILES})
set_property(SOURCE ${QRC_FILES} PROPERTY SKIP_AUTOGEN ON)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
add_executable(${PROJECT_NAME} WIN32 ${MAIN_SRCS} ${QRC_FILES} ${APP_ICON})

target_precompile_headers(${PROJECT_NAME} PRIVATE prefix.h)
target_link_libraries(${PROJECT_NAME} Qt5::Widgets hdf5::hdf5-shared HighFive)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
