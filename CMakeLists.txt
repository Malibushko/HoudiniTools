cmake_minimum_required (VERSION 3.20)

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("HTools")

list(APPEND CMAKE_PREFIX_PATH "$ENV{HFS}/toolkit/cmake")

find_package(Houdini REQUIRED)

set(PROJECT_NAME "HTools")

include(src/CMakeLists.txt)

add_library(${PROJECT_NAME} SHARED ${HTOOLS_HEADERS} ${HTOOLS_SOURCES}
        src/algorithms/IndexedPoint.h)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

include_directories(src)
target_link_libraries(${PROJECT_NAME} Houdini)

if (DEFINED ENV{HOUDINI_DSO_PATH}) # Somewhy this command gets stuck on execute_process in it's internals so this is the workaround
    houdini_configure_target(${PROJECT_NAME} INSTDIR $ENV{HOUDINI_DSO_PATH})
else()
    houdini_configure_target(${PROJECT_NAME})
endif()