cmake_minimum_required(VERSION 3.0)
project(Bryanttxc)

# add path
include_directories(.)
include_directories(./util)
include_directories(./log)
include_directories(./fiberLibrary)

# find library
find_library(PTHREAD pthread)

set(LIBS_SRC
    ./log/logger.cc
    ./util/util.cc
    ./fiberLibrary/thread.cc
    ./fiberLibrary/fiber.cc)

# set library
add_library(bryant SHARED ${LIBS_SRC})

set(LIBS
    bryant
    pthread)

# put binary file into bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
add_executable(test_locker test/test_locker.cc)
target_link_libraries(test_locker ${LIBS})

add_executable(test_logger test/test_logger.cc)
target_link_libraries(test_logger ${LIBS})

add_executable(test_thread test/test_thread.cc)
target_link_libraries(test_thread ${LIBS})

add_executable(test_fiber test/test_fiber.cc)
target_link_libraries(test_fiber ${LIBS})