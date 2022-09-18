cmake_minimum_required(VERSION 3.16)

PROJECT(XMarketCenter)
SET(AppVersion 0.8.0)
SET(XMarketCenter_Version ${PROJECT_NAME}_${AppVersion})

set(CMAKE_CXX_FLAGS "-fPIC")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

macro(Get_CommitID APP_COMMITID)   
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${APP_COMMITID}        # 输出字符串存入变量
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
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/ # 执行路径
        )
    endif()
endmacro() 

set(APP_COMMITID "")
Get_CommitID(APP_COMMITID)
message(STATUS "AppCommitID is ${APP_COMMITID}")

set(UTILS_COMMITID "")
Get_UtilsCommitID(UTILS_COMMITID)
message(STATUS "UtilsCommitID is ${UTILS_COMMITID}")

add_definitions(-DAPP_COMMITID=\"${APP_COMMITID}\")
add_definitions(-DUTILS_COMMITID=\"${UTILS_COMMITID}\") 

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
# 输出APP至build目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/)
# 输出静态库至build目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/)

# 编译API SO
add_subdirectory(MarketAPI)
add_subdirectory(Capture)

