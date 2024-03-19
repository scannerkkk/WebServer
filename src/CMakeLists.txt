cmake_minimum_required(VERSION 3.8)
PROJECT(WebSever)   
# include_directories(/usr/include) ##本机MySQL下的include路径
# link_directories(/usr/lib)
# set(main.cpp log.cpp threadPoll.cpp blockQueue.cpp buffer.cpp epoller.cpp httpConnection.cpp httpRequest.cpp httpresponse.cpp sqlConnectionPool.cpp threadPool.h timerManage.cpp webServer.cpp)
# ADD_EXECUTABLE(WebSever main.cpp)
# target_link_libraries(WebSever libmysql)

find_path(MYSQL_ROOT_DIR mysql)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mysql REQUIRED_VARS MYSQL_ROOT_DIR)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${MYSQL_ROOT_DIR}/include)
target_link_directories(${PROJECT_NAME} PRIVATE ${MYSQL_ROOT_DIR}/lib)
target_link_libraries(${PROJECT_NAME} libmysql)