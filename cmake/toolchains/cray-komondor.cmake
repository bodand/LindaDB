set(CMAKE_SYSTEM_PROCESSOR "x86_64")
cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)

if (DEFINED ENV{PROJECT_CC_EXE})
    set(CMAKE_C_COMPILER "$ENV{PROJECT_CC_EXE}")
else ()
    set(CMAKE_C_COMPILER "cc")
endif ()

if (DEFINED ENV{PROJECT_CXX_EXE})
    set(CMAKE_CXX_COMPILER "$ENV{PROJECT_CXX_EXE}")
else ()
    set(CMAKE_CXX_COMPILER "CC")
endif ()

set(CMAKE_C_FLAGS_INIT "-m64")
set(CMAKE_CXX_FLAGS_INIT "-m64")

set(project_root_path "${CMAKE_CURRENT_SOURCE_DIR}")
if (CMAKE_CURRENT_SOURCE_DIR MATCHES [[(.+)/_build-]])
    set(project_root_path "${CMAKE_MATCH_1}")
endif ()

cmake_policy(POP)

if (NOT EXISTS "${project_root_path}/vcpkg.json")
    file(COPY_FILE "${project_root_path}/vcpkg.komondor.json"
            "${project_root_path}/vcpkg.json")
endif ()

include("${project_root_path}/vcpkg/scripts/buildsystems/vcpkg.cmake")
