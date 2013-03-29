cmake_minimum_required(VERSION 2.6)

project(RecursiveRunner)

#list of the supported OS
SET(SUPPORTED_TARGETS linux android emscripten windows mac)

#check that target has been set
if (NOT TARGET)
    message(SEND_ERROR "No target defined! Aborting. Targets available: ${SUPPORTED_TARGETS}.
    Example: cmake . -DTARGET=linux")
    return()
endif()


string (TOLOWER "${TARGET}" varplatform)
string (TOUPPER "${TARGET}" VARPLATFORM)

LIST(FIND SUPPORTED_TARGETS ${varplatform} target_in_list)
#check it's supported yet
if (target_in_list EQUAL -1)
    message(SEND_ERROR "${varplatform} not in supported targets: ${SUPPORTED_TARGETS}")
    return()
endif()

message ("Building for ${varplatform}")
add_definitions(-DSAC_${VARPLATFORM})


include(sac/build/cmake/toolchains/${varplatform}.cmake)
include(sac/build/cmake/CMakeLists.txt)
