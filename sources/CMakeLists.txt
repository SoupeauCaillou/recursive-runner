include_directories(${PROJECT_SOURCE_DIR}/sources)
include_directories(${PROJECT_SOURCE_DIR}/sac)

link_directories (${PROJECT_BINARY_DIR}/sac)

file(
        GLOB
        source_files
        *.cpp *.h systems/*cpp systems/*.h menus/*.cpp menus/*.h
)

add_library(prototypecommon ${source_files})
