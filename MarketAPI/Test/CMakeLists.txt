cmake_minimum_required(VERSION 3.16)
PROJECT(TestMarket)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.1.0)
SET(MarketGateWay_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/MarketAPI/)
include_directories(${CMAKE_SOURCE_DIR}/)

# SPDLog
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)

# YAML-CPP
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/lib)


set(SOURCES TestMarketGateWay.cpp
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)
add_library(${MarketGateWay_Version} SHARED ${SOURCES})

target_link_libraries(${MarketGateWay_Version} spdlog yaml-cpp)   