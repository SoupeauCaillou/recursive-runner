cmake_minimum_required(VERSION 2.6)

#set the name of the project...
project(RecursiveRunner)

option(USE_RESTRICTIVE_PLUGINS "Use restrictive plugins (Google Play Game Services)" ON)

if (USE_RESTRICTIVE_PLUGINS) 
	add_definitions(-DSAC_RESTRICTIVE_PLUGINS=1)
endif()

#and let the magic begin :-)
include(sac/build/cmake/CMakeLists.txt)


