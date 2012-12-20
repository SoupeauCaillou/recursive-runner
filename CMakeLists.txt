cmake_minimum_required(VERSION 2.8)

project(TILEMATCH)

SET(CMAKE_C_FLAGS "-O0 -g -W -Wall")
if (EMSCRIPTEN_BUILD)
SET(CMAKE_CXX_FLAGS "-O2 -W -Wall")
else ()
SET(CMAKE_CXX_FLAGS "-O0 -g -W -Wall -DDEBUG -DENABLE_PROFILING -DINGAME_EDITORS -DDATADIR=\\\"${CMAKE_SOURCE_DIR}/assets/\\\"  -fvisibility=hidden")
endif()

add_subdirectory(sources)

add_subdirectory(linux)
if (EMSCRIPTEN_BUILD)
else ()
endif()
add_subdirectory(sac/build/cmake)
