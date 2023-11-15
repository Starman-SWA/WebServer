cmake_minimum_required(VERSION 3.22)
project(WebServer)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE WEBSERVER_SOURCE_PATH src/Server/*.cpp)

add_executable(WebServer ${WEBSERVER_SOURCE_PATH})

add_executable(testTimerHeap Test/testTimerHeap.cpp src/Server/Timer/TimerHeap.cpp src/Server/Timer/TimerTask.cpp src/Server/Server/Logger.cpp)

#add_executable(ThreadSafeQueue_test Test/ThreadSafeQueue_test.cpp)
#
#add_executable(EchoClient src/Client/EchoClient.cpp)
#
#add_executable(testReadFromCurl Test/testReadFromCurl.cpp)
#
#add_executable(testMmap Test/testMmap.cpp)
