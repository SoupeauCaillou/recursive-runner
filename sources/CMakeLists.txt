include_directories(${PROJECT_SOURCE_DIR}/sources)
include_directories(${PROJECT_SOURCE_DIR}/sac)

link_directories (${PROJECT_BINARY_DIR}/sac)

file(
        GLOB
        source_files
        *.cpp *.h api/linux/*.cpp systems/*.cpp systems/*.h menus/*.cpp menus/*.h states/*.cpp states/*.h
)

add_library(recursiveRunnercommon ${source_files})
