include_directories(${PROJECT_SOURCE_DIR}/sources)
include_directories(${PROJECT_SOURCE_DIR}/sac)
include_directories(${PROJECT_SOURCE_DIR}/sac/libs/squirrel/include)
include_directories(${PROJECT_SOURCE_DIR}/sac/libs/sqrat/include)
link_directories (${PROJECT_BINARY_DIR}/sac)
link_directories (${PROJECT_BINARY_DIR}/sac/libs/squirrel/lib)

file(
        GLOB
        source_files
        *.cpp *.h
)

if (EMSCRIPTEN_BUILD)
	add_executable(recursiveRunner.bc ${source_files})
	target_link_libraries(recursiveRunner.bc recursiveRunnercommon sac GLEW glfw openal sqlite3  vorbisfile vorbis z png)
	add_custom_command(TARGET recursiveRunner.bc
		POST_BUILD
		COMMAND emcc -s ALLOW_MEMORY_GROWTH=1 -O2 ${CMAKE_BINARY_DIR}/linux/recursiveRunner.bc -o /tmp/recursiveRunner.html --preload-file assets
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
		COMMENT "Building emscripten HTML")	
else ()
	add_executable(recursiveRunner ${source_files})
	target_link_libraries(recursiveRunner recursiveRunnercommon sac GLEW glfw openal sqlite3  vorbisfile vorbis z vpx pthread)
endif ()
