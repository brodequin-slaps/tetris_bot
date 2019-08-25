cmake_minimum_required(VERSION 3.10.2)
project (tetris_bot)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Boost
find_package(Boost REQUIRED fiber)

#bring the headers into the project
include_directories(include ${BOOST_INCLUDE_DIRS})
message("boost lib: ${Boost_LIBRARIES}")
message("boost inc:${Boost_INCLUDE_DIR}")

#add cpp files into the project
file(GLOB SOURCES "src/*.cpp")

add_executable(tetris_bot ${SOURCES})

target_compile_options(tetris_bot PRIVATE -Wall -Wextra)
target_link_libraries(tetris_bot ${Boost_LIBRARIES})