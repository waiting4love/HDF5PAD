cmake_minimum_required(VERSION 3.0.0)
project(HDF5Pad VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

SET(CMAKE_PREFIX_PATH D:\\Qt\\Qt5.6.0\\5.6\\msvc2015_64)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)

SET(MAIN_SRCS main.cpp mainwindow.cpp)
find_package(Qt5 COMPONENTS Widgets Core)

set(QRC_SOURCE_FILES hdf5pad.qrc)
qt5_add_resources(QRC_FILES ${QRC_SOURCE_FILES})
set_property(SOURCE ${QRC_FILES} PROPERTY SKIP_AUTOGEN ON)

add_compile_options(-utf-8)
set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/app.rc")
ADD_EXECUTABLE(${PROJECT_NAME} WIN32 ${MAIN_SRCS} ${QRC_FILES} ${APP_ICON})
target_include_directories(
    ${PROJECT_NAME} PRIVATE
    3rdparty/hdf5/include
    3rdparty/HighFive/include
)
target_link_directories(
    ${PROJECT_NAME} PRIVATE
    3rdparty/hdf5/bin
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} Qt5::Widgets hdf5)
target_precompile_headers(${PROJECT_NAME} PRIVATE prefix.h)
target_compile_definitions(${PROJECT_NAME} PRIVATE H5_BUILT_AS_DYNAMIC_LIB)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
