cmake_minimum_required(VERSION 3.20)
PROJECT(CTPMarket)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.5.0)
SET(MarketGateWay_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

macro(Get_CommitID SO_COMMITID)
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${SO_COMMITID}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # 执行路径
        )
    endif()
endmacro()

macro(Get_UtilsCommitID GIT_UTILSCOMMITID)
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${GIT_UTILSCOMMITID}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/ # 执行路径
        )
    endif()
endmacro()

SET(APIVersion CTP6.7.8)

set(SO_COMMITID "")
Get_CommitID(SO_COMMITID)
message(STATUS "SoCommitID is ${SO_COMMITID}")

set(SO_UTILS_COMMITID "")
Get_UtilsCommitID(SO_UTILS_COMMITID)
message(STATUS "SoUtilsCommitID is ${SO_UTILS_COMMITID}")

add_definitions(-DSO_COMMITID=\"${SO_COMMITID}\")
add_definitions(-DSO_UTILS_COMMITID=\"${SO_UTILS_COMMITID}\")
add_definitions(-DAPI_VERSION=\"${APIVersion}\")

include_directories(${CMAKE_SOURCE_DIR}/../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/MarketAPI/)
include_directories(${CMAKE_SOURCE_DIR}/)

# SPDLog
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/SPDLog/1.8.5/lib)
# YAML-CPP
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/YAML-CPP/0.6.3/include/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/YAML-CPP/0.6.3/lib)

# CTP 6.7.8
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/CTP/6.7.8/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../XAPI/CTP/6.7.8/lib)

set(SOURCES CTPMarketGateWay.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/Logger.cpp)
add_library(${MarketGateWay_Version} SHARED ${SOURCES})

target_link_libraries(${MarketGateWay_Version} spdlog yaml-cpp thostmduserapi_se)   