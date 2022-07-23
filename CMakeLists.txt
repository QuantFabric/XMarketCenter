cmake_minimum_required(VERSION 3.16)

PROJECT(XMarketCenter)
SET(AppVersion 0.1.0)
SET(XMarketCenter_Version ${PROJECT_NAME}_${AppVersion})

set(CMAKE_CXX_FLAGS "-fPIC")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/)
include_directories(${CMAKE_SOURCE_DIR}/../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/MarketAPI/)

# HP-Scoket
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/lib)

# SPDLog
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)

# YAML-CPP
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/lib)

set(SOURCES main.cpp 
            HPPackClient.cpp
            XMarketCenter.cpp 
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)

add_executable(${XMarketCenter_Version} ${SOURCES})

target_link_libraries(${XMarketCenter_Version} yaml-cpp spdlog pthread hpsocket4c dl)  

# 输出so至build目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/)
# 编译API SO
add_subdirectory(MarketAPI)