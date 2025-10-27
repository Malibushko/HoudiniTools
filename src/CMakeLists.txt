include(src/geo/CMakeLists.txt)
include(src/algorithms/CMakeLists.txt)

set(HTOOLS_HEADERS
        ${GEO_HEADERS}
        ${ALGO_HEADERS}
)
set(HTOOLS_SOURCES
        src/Library.cpp
        ${GEO_SOURCES}
        ${ALGO_SOURCES}
)