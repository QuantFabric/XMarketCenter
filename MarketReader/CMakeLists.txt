cmake_minimum_required(VERSION 3.16)
PROJECT(MarketReader)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.1.0)
SET(MarketReader_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Utils/)
# SHMServer
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../SHMServer/)

set(SOURCES MarketReader.cpp)
add_executable(${MarketReader_Version} ${SOURCES})
target_link_libraries(${MarketReader_Version} pthread rt)  


