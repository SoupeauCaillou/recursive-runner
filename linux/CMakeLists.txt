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
	add_executable(prototype.bc ${source_files})
	target_link_libraries(prototype.bc prototypecommon sac GLEW glfw openal sqlite3  vorbisfile vorbis z png)
	add_custom_command(TARGET prototype.bc
		POST_BUILD
		COMMAND EMCC_DEBUG=1 emcc -s WARN_ON_UNDEFINED_SYMBOLS=1 -O2 --llvm-lto 1 ${CMAKE_BINARY_DIR}/linux/prototype.bc -o /tmp/prototype.html --preload-file assets
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
		COMMENT "Building emscripten HTML")	
else ()
	add_executable(prototype ${source_files})
	target_link_libraries(prototype prototypecommon sac GLEW glfw openal sqlite3  vorbisfile vorbis z)
endif ()
