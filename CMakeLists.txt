cmake_minimum_required(VERSION 3.16)

PROJECT(XMarketCenter)
SET(AppVersion 1.0)
SET(XMarketCenter_Version ${PROJECT_NAME}_CTP_${AppVersion})

set(CMAKE_CXX_FLAGS "-fPIC")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Utils/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/MarketAPI/)

# SPDLog
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)

# YAML-CPP
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../XAPI/YAML-CPP/0.8.0/include/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../XAPI/YAML-CPP/0.8.0/lib)

set(SOURCES main.cpp 
            XMarketCenter.cpp 
            ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/Logger.cpp)

add_executable(${XMarketCenter_Version} ${SOURCES})

target_link_libraries(${XMarketCenter_Version} yaml-cpp spdlog pthread dl)  

# 编译API SO
add_subdirectory(MarketAPI)

