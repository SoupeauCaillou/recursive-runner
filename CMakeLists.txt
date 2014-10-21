cmake_minimum_required(VERSION 2.6)

#set the name of the project...
project(RecursiveRunner)

option(BENCHMARK_MODE "Build benchmark-mode" OFF)
if (BENCHMARK_MODE STREQUAL "ON")
    message("Benchmark mode enabled")
    add_definitions(-DSAC_BENCHMARK_MODE=1)
endif()

add_definitions(
    -DDISABLE_SCROLLING_SYSTEM=1
    -DDISABLE_AUTONOMOUS_SYSTEM=1
    -DDISABLE_COLLISION_SYSTEM=1
    -DDISABLE_CONTAINER_SYSTEM=1
    -DDISABLE_GRAPH_SYSTEM=1
    -DDISABLE_GRID_SYSTEM=1
    -DDISABLE_MORPHING_SYSTEM=1
    -DDISABLE_ZSQD_SYSTEM=1
    -DDISABLE_SPOT_SYSTEM=1
    -DDISABLE_SWYPE_SYSTEM=1
    -DDISABLE_BLINK_SYSTEM=1
    -DDISABLE_DEBUGGING_SYSTEM=1
    -DDISABLE_NETWORK_SYSTEM=1
)

#and let the magic begin :-)
include(sac/CMakeLists.txt)


