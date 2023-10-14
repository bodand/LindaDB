set(CMAKE_SYSTEM_PROCESSOR "x86_64")
cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)

if (DEFINED ENV{MS_VS_PATH})
    set(VS_ROOT "$ENV{MS_VS_PATH}")
else ()
    message(STATUS "MSVC: MS_VS_PATH not set, falling back on auto-detection")
    if (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Community")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/MSVC")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Professional")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/MSVC")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Enterprise")
    elseif (IS_DIRECTORY "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC")
        set(VS_ROOT "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/BuildTools")
    endif ()
endif ()
set(msvc_root "${VS_ROOT}/VC/Tools/MSVC")

file(GLOB msvc_versioned_roots
        "${msvc_root}/*"
        LIST_DIRECTORIES true)

list(LENGTH msvc_versioned_roots msvc_cnt)
if (msvc_cnt GREATER 1)
    message(WARNING "Multiple MSVC installations were found. Picking the one CMake picked up first.
I have no idea if this is deterministic even on a stable set of compilers.")
elseif (msvc_cnt LESS 1)
    message(FATAL_ERROR "Could not find MSVC installation. Is the MS_VS_PATH environment variable correctly set?
Currently using the following path: \"${VS_ROOT}\"
MS_VS_PATH: \"$ENV{MS_VS_PATH}\"")
endif ()

list(GET msvc_versioned_roots 0 msvc_versioned_root)

set(CMAKE_C_COMPILER "${msvc_versioned_root}/bin/Hostx64/x64/cl.exe")
set(CMAKE_CXX_COMPILER "${msvc_versioned_root}/bin/Hostx64/x64/cl.exe")

# i cannot comment on this...
set(project_root_path "${CMAKE_CURRENT_SOURCE_DIR}")
if (CMAKE_CURRENT_SOURCE_DIR MATCHES [[(.+)/_build-]])
    set(project_root_path "${CMAKE_MATCH_1}")
endif ()

cmake_policy(POP)

include("${project_root_path}/vcpkg/scripts/buildsystems/vcpkg.cmake")
