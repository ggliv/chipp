cmake_minimum_required(VERSION 3.27)
project(chipp CXX)

set(CMAKE_CXX_STANDARD 11)

add_executable(${CMAKE_PROJECT_NAME} main.cpp chip_eight.cpp)

add_subdirectory(sdl)
target_link_libraries(${CMAKE_PROJECT_NAME} SDL2-static)

if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
  set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g3")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-g -Og")
  set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")
endif()

if (EMSCRIPTEN)
  add_link_options("-o ${CMAKE_PROJECT_NAME}.html")
endif()
