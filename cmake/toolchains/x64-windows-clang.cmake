set(CMAKE_SYSTEM_PROCESSOR "x86_64")
cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)

if (DEFINED ENV{MS_VS_PATH})
    set(VS_ROOT "$ENV{MS_VS_PATH}")
else ()
    message(STATUS "clang-cl: MS_VS_PATH not set, falling back on auto-detection")
    if (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/Llvm")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Community")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Professional/VC/Tools/Llvm")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Professional")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/Enterprise/VC/Tools/Llvm")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/Enterprise")
    elseif (IS_DIRECTORY "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm")
        set(VS_ROOT "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools")
    elseif (IS_DIRECTORY "C:/Program Files/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm")
        set(VS_ROOT "C:/Program Files/Microsoft Visual Studio/2022/BuildTools")
    endif ()
endif ()

set(clang_cl "${VS_ROOT}/VC/Tools/Llvm/x64/bin/clang-cl.exe")
if (NOT EXISTS "${clang_cl}")
    message(FATAL_ERROR "Could not find clang-cl. Is the MS_VS_PATH environment variable correctly set?
Currently using the following path: \"${VS_ROOT}\"
MS_VS_PATH: \"$ENV{MS_VS_PATH}\"")
endif ()

set(CMAKE_C_COMPILER "${clang_cl}")
set(CMAKE_CXX_COMPILER "${clang_cl}")
set(CMAKE_C_FLAGS_Debug_INIT "-m64 /MDd")
set(CMAKE_C_FLAGS_Release_INIT "-m64 /MD")
set(CMAKE_CXX_FLAGS_Debug_INIT "-m64 /MDd")
set(CMAKE_CXX_FLAGS_Release_INIT "-m64 /MD")

# i cannot comment on this...
set(project_root_path "${CMAKE_CURRENT_SOURCE_DIR}")
if (CMAKE_CURRENT_SOURCE_DIR MATCHES [[(.+)/_build-]])
    set(project_root_path "${CMAKE_MATCH_1}")
endif ()

cmake_policy(POP)

include("${project_root_path}/vcpkg/scripts/buildsystems/vcpkg.cmake")
