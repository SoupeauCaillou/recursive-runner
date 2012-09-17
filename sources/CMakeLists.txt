include_directories(${PROJECT_SOURCE_DIR}/sources)
include_directories(${PROJECT_SOURCE_DIR}/sac)

link_directories (${PROJECT_BINARY_DIR}/sac)

file(
        GLOB
        source_files
        *.cpp *.h
)

add_library(prototypecommon ${source_files})
